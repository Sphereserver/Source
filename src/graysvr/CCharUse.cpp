//  CChar is either an NPC or a Player.
#include "graysvr.h"	// predef header.

static const int MASK_RETURN_FOLLOW_LINKS = 0x02;

bool CChar::Use_MultiLockDown( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_MultiLockDown");
	ASSERT(pItemTarg);
	ASSERT(m_pArea);

	if ( pItemTarg->IsType(IT_KEY) || !pItemTarg->IsMovableType() || !pItemTarg->IsTopLevel() )
		return false;

	if ( !pItemTarg->m_uidLink.IsValidUID() )
	{
		// If we are in a house then lock down the item.
		pItemTarg->m_uidLink.SetPrivateUID(m_pArea->GetResourceID());
		SysMessageDefault(DEFMSG_MULTI_LOCKDOWN);
		return true;
	}
	if ( pItemTarg->m_uidLink == m_pArea->GetResourceID() )
	{
		pItemTarg->m_uidLink.InitUID();
		SysMessageDefault(DEFMSG_MULTI_LOCKUP);
		return true;
	}

	return false;
}

bool CChar::Use_CarveCorpse( CItemCorpse * pCorpse )
{
	ADDTOCALLSTACK("CChar::Use_CarveCorpse");
	CREID_TYPE CorpseID = pCorpse->m_itCorpse.m_BaseID;
	CCharBase *pCorpseDef = CCharBase::FindCharBase(CorpseID);
	if ( !pCorpseDef || pCorpse->m_itCorpse.m_carved )
	{
		SysMessageDefault(DEFMSG_CARVE_CORPSE_NOTHING);
		return false;
	}

	CChar *pChar = pCorpse->m_uidLink.CharFind();
	CPointMap pnt = pCorpse->GetTopLevelObj()->GetTopPoint();

	UpdateAnimate(ANIM_BOW);
	if ( pCorpse->m_TagDefs.GetKeyNum("BLOOD") )
	{
		CItem *pBlood = CItem::CreateBase(ITEMID_BLOOD4);
		ASSERT(pBlood);
		pBlood->SetHue(pCorpseDef->m_wBloodHue);
		pBlood->MoveToDecay(pnt, 5 * TICK_PER_SEC);
	}

	size_t iItems = 0;
	for ( size_t i = 0; i < pCorpseDef->m_BaseResources.GetCount(); i++ )
	{
		long long iQty = pCorpseDef->m_BaseResources[i].GetResQty();
		RESOURCE_ID rid = pCorpseDef->m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( id == ITEMID_NOTHING )
			break;

		iItems++;
		CItem *pPart = CItem::CreateTemplate(id, NULL, this);
		ASSERT(pPart);
		switch ( pPart->GetType() )
		{
			case IT_FOOD:
			case IT_FOOD_RAW:
			case IT_MEAT_RAW:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_MEAT);
				break;
			case IT_HIDE:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_HIDES);
				if ( (g_Cfg.m_iRacialFlags & RACIALF_HUMAN_WORKHORSE) && IsHuman() )	// humans always find 10% bonus when gathering hides, ores and logs (Workhorse racial trait)
					iQty = iQty * 110 / 100;
				break;
			case IT_FEATHER:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_FEATHERS);
				break;
			case IT_WOOL:
				SysMessageDefault(DEFMSG_CARVE_CORPSE_WOOL);
				break;
			/*case IT_DRAGON_SCALE:			// TO-DO (typedef IT_DRAGON_SCALE doesn't exist yet)
				SysMessageDefault(DEFMSG_CARVE_CORPSE_SCALES);
				break;*/
			default:
				break;
		}

		if ( iQty > 1 )
			pPart->SetAmount(static_cast<WORD>(iQty));

		if ( pChar && pChar->m_pPlayer )
		{
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_CORPSE_NAME), pPart->GetName(), pChar->GetName());
			pPart->SetName(pszMsg);
			pPart->m_uidLink = pChar->GetUID();
			pPart->MoveToDecay(pnt, pPart->GetDecayTime());
			continue;
		}
		pCorpse->ContentAdd(pPart);
	}

	if ( iItems < 1 )
		SysMessageDefault(DEFMSG_CARVE_CORPSE_NOTHING);

	CheckCorpseCrime(pCorpse, false, false);
	pCorpse->m_itCorpse.m_carved = 1;			// mark as been carved
	pCorpse->m_itCorpse.m_uidKiller = GetUID();	// by you

	if ( pChar && pChar->m_pPlayer )
		pCorpse->SetTimeout(0);		// reset corpse timer to make it turn bones
	return true;
}

bool CChar::Use_MoonGate( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_MoonGate");
	ASSERT(pItem);

	CPointBase pt = pItem->m_itTelepad.m_pntMark;

	if ( pItem->IsType(IT_MOONGATE) )
	{
		// RES_MOONGATES
		// What gate are we at ?
		size_t i = 0;
		size_t iCount = g_Cfg.m_MoonGates.GetCount();
		for ( ; i < iCount; i++ )
		{
			if ( GetTopPoint().GetDist(g_Cfg.m_MoonGates[i]) <= UO_MAP_VIEW_SIZE )
				break;
		}

		// Set it's current destination based on the moon phases.
		// ensure iTrammelPhrase isn't smaller than iFeluccaPhase, to avoid uint underflow in next calculation
		unsigned int iTrammelPhase = g_World.GetMoonPhase(false) % iCount;
		unsigned int iFeluccaPhase = g_World.GetMoonPhase(true) % iCount;
		if ( iTrammelPhase < iFeluccaPhase )
			iTrammelPhase += iCount;

		size_t iMoongateIndex = (i + (iTrammelPhase - iFeluccaPhase)) % iCount;
		ASSERT(g_Cfg.m_MoonGates.IsValidIndex(iMoongateIndex));
		pt = g_Cfg.m_MoonGates[iMoongateIndex];
	}

	if ( m_pNPC )
	{
		if ( pItem->m_itTelepad.m_fPlayerOnly )
			return false;
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )	// guards won't leave the guarded region
		{
			CRegionBase *pArea = pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
			if ( !pArea || !pArea->IsGuarded() )
				return false;
		}
		if ( Noto_IsCriminal() )	// criminals won't enter on guarded regions
		{
			CRegionBase *pArea = pt.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA);
			if ( !pArea || pArea->IsGuarded() )
				return false;
		}
	}

	bool bCheckAntiMagic = pItem->IsAttr(ATTR_DECAY);
	bool bDisplayEffect = !pItem->m_itTelepad.m_fQuiet;
	return Spell_Teleport(pt, true, bCheckAntiMagic, bDisplayEffect);
}

bool CChar::Use_Kindling( CItem * pKindling )
{
	ADDTOCALLSTACK("CChar::Use_Kindling");
	ASSERT(pKindling);
	if ( !pKindling->IsTopLevel() )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_KINDLING_CONT);
		return false;
	}

	if ( !Skill_UseQuick(SKILL_CAMPING, Calc_GetRandVal(30)) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_KINDLING_FAIL);
		return false;
	}

	pKindling->SetID(ITEMID_CAMPFIRE);
	pKindling->SetAttr(ATTR_MOVE_NEVER|ATTR_CAN_DECAY);
	pKindling->SetTimeout((4 + pKindling->GetAmount()) * 60 * TICK_PER_SEC);
	pKindling->SetAmount(1);	// all kindling is set to one fire
	pKindling->m_itLight.m_pattern = LIGHT_LARGE;
	pKindling->Update();
	return true;
}

bool CChar::Use_Cannon_Feed( CItem * pCannon, CItem * pFeed )
{
	ADDTOCALLSTACK("CChar::Use_Cannon_Feed");
	if ( pFeed && pCannon && pCannon->IsType(IT_CANNON_MUZZLE) )
	{
		if ( !CanUse(pCannon, false) )
			return false;
		if ( !CanUse(pFeed, true) )
			return false;

		if ( pFeed->GetID() == ITEMID_REAG_SA )
		{
			if ( pCannon->m_itCannon.m_Load & 1 )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_CANNON_HPOWDER);
				return false;
			}
			pCannon->m_itCannon.m_Load |= 1;
			SysMessageDefault(DEFMSG_ITEMUSE_CANNON_LPOWDER);
			return true;
		}

		if ( pFeed->IsType(IT_CANNON_BALL) )
		{
			if ( pCannon->m_itCannon.m_Load & 2 )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_CANNON_HSHOT);
				return false;
			}
			pCannon->m_itCannon.m_Load |= 2;
			SysMessageDefault(DEFMSG_ITEMUSE_CANNON_LSHOT);
			return true;
		}
	}

	SysMessageDefault(DEFMSG_ITEMUSE_CANNON_EMPTY);
	return false;
}

bool CChar::Use_Train_Dummy( CItem * pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_Dummy");
	// IT_TRAIN_DUMMY
	// Dummy animation timer prevents over dclicking.
	ASSERT(pItem);

	if ( !pItem->IsTopLevel() || (GetDist(pItem) > 1) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_TOOFAR);
		return false;
	}
	else if ( IsStatFlag(STATF_OnHorse) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_MOUNT);
		return false;
	}

	SKILL_TYPE skill = Fight_GetWeaponSkill();
	if ( g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_RANGED);
		return false;
	}

	// Start action
	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
			return true;

		char skilltag[32];
		sprintf(skilltag, "OVERRIDE.PracticeMax.SKILL_%d", skill & ~0xD2000000);
		CVarDefCont *pSkillTag = pItem->GetKey(skilltag, true);
		WORD iMaxSkill = pSkillTag ? static_cast<WORD>(pSkillTag->GetValNum()) : g_Cfg.m_iSkillPracticeMax;
		if ( Skill_GetBase(skill) > iMaxSkill )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_TRAININGDUMMY_SKILL);
			return false;
		}

		UpdateAnimate(ANIM_ATTACK_WEAPON);
		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pItem->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	static const SOUND_TYPE sm_TrainingDummySounds[] = { 0x3A4, 0x3A6, 0x3A9, 0x3AE, 0x3B4, 0x3B6 };
	pItem->Sound(sm_TrainingDummySounds[Calc_GetRandVal(COUNTOF(sm_TrainingDummySounds))]);
	pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetDispID() + 1), 3 * TICK_PER_SEC);
	Skill_Experience(skill, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Train_PickPocketDip( CItem * pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_PickPocketDip");
	// IT_TRAIN_PICKPOCKET
	// Train dummy.
	ASSERT(pItem);

	if ( !pItem->IsTopLevel() || (GetDist(pItem) > 1) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_TOOFAR);
		return true;
	}
	else if ( IsStatFlag(STATF_OnHorse) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_MOUNT);
		return false;
	}

	// Start action
	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
			return true;

		if ( Skill_GetBase(SKILL_STEALING) > g_Cfg.m_iSkillPracticeMax )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_SKILL);
			return true;
		}

		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pItem->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	pItem->Sound(SOUND_RUSTLE);
	if ( Skill_UseQuick(SKILL_STEALING, Calc_GetRandVal(40)) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_SUCCESS);
		pItem->SetAnim(pItem->GetDispID(), 3 * TICK_PER_SEC);
	}
	else
	{
		SysMessageDefault(DEFMSG_ITEMUSE_PICKPOCKET_FAIL);
		pItem->Sound(SOUND_GLASS_BREAK4);
		pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetDispID() + 1), 3 * TICK_PER_SEC);
	}
	Skill_Experience(SKILL_STEALING, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Train_ArcheryButte( CItem * pButte, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_ArcheryButte");
	// IT_ARCHERY_BUTTE
	ASSERT(pButte);

	// If standing right next to the butte, gather the arrows/bolts
	int iDist = GetDist(pButte);
	if ( (iDist < 2) && pButte->m_itArcheryButte.m_AmmoCount )
	{
		CItem *pRemovedAmmo = CItem::CreateBase(pButte->m_itArcheryButte.m_AmmoType);
		ASSERT(pRemovedAmmo);
		pRemovedAmmo->SetAmount(pButte->m_itArcheryButte.m_AmmoCount);
		ItemBounce(pRemovedAmmo, false);
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_GATHER);

		pButte->m_itArcheryButte.m_AmmoType = ITEMID_NOTHING;
		pButte->m_itArcheryButte.m_AmmoCount = 0;
		return true;
	}

	CItem *pWeapon = m_uidWeapon.ItemFind();
	SKILL_TYPE skill = pWeapon ? pWeapon->Weapon_GetSkill() : SKILL_NONE;
	if ( !g_Cfg.IsSkillFlag(skill, SKF_RANGED) )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_RANGED);
		return true;
	}

	// Check distance
	if ( iDist < 5 )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_TOOCLOSE);
		return false;
	}
	else if ( iDist > 6 )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_TOOFAR);
		return false;
	}

	// Check position alignment
	CPointMap ptChar = GetTopPoint();
	CPointMap ptButte = pButte->GetTopPoint();
	if ( pButte->GetID() == ITEMID_ARCHERYBUTTE_S )
	{
		if ( ptChar.m_x != ptButte.m_x )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGALIGN);
			return false;
		}
		else if ( ptChar.m_y > ptButte.m_y )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGPOS);
			return false;
		}
	}
	else
	{
		if ( ptChar.m_y != ptButte.m_y )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGALIGN);
			return false;
		}
		else if ( ptChar.m_x < ptButte.m_x )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_WRONGPOS);
			return false;
		}
	}

	// Start action
	if ( fSetup )
	{
		if ( (Skill_GetActive() == NPCACT_TRAINING) || (m_atFight.m_Swing_NextAction > CServTime::GetCurrentTime()) )
			return true;

		char skilltag[32];
		sprintf(skilltag, "OVERRIDE.PracticeMax.SKILL_%d", skill & ~0xD2000000);
		CVarDefCont *pSkillTag = pButte->GetKey(skilltag, true);
		WORD iMaxSkill = pSkillTag ? static_cast<WORD>(pSkillTag->GetValNum()) : g_Cfg.m_iSkillPracticeMax;
		if ( Skill_GetBase(skill) > iMaxSkill )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_SKILL);
			return false;
		}

		UpdateAnimate(ANIM_ATTACK_WEAPON);
		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pButte->GetUID();
		Skill_Start(NPCACT_TRAINING);
		return true;
	}

	// Finish action
	if ( Skill_GetActive() != NPCACT_TRAINING )
		return false;

	CItem *pAmmo = NULL;
	if ( skill != SKILL_THROWING )		// throwing weapons doesn't need ammo
	{
		pAmmo = pWeapon->Weapon_FindRangedAmmo();
		if ( !pAmmo )
		{
			SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
			return false;
		}
	}

	// If there is a different ammo type on the butte, it must be removed first
	LPCTSTR pszButteAmmo = g_Cfg.ResourceGetName(RESOURCE_ID(RES_ITEMDEF, pButte->m_itArcheryButte.m_AmmoType));
	RESOURCE_ID rid = g_Cfg.ResourceGetID(RES_ITEMDEF, pszButteAmmo);
	ITEMID_TYPE ButteAmmoID = static_cast<ITEMID_TYPE>(rid.GetResIndex());
	ITEMID_TYPE WeaponAmmoID = ITEMID_NOTHING;
	if ( pAmmo )
	{
		WeaponAmmoID = pAmmo->GetID();
		if ( ButteAmmoID && (ButteAmmoID != WeaponAmmoID) )
		{
			SysMessageDefault(DEFMSG_ITEMUSE_ARCHBUTTE_CLEAN);
			return false;
		}
	}

	ITEMID_TYPE AnimID = ITEMID_NOTHING;
	DWORD AnimHue = 0, AnimRender = 0;
	pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
	pButte->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

	// TO-DO: Find a way to make this code work here. It inherits from CChar::Fight_Hit() and works using
	// TIMERF function to call TRYSRC with the EFFECT to send, but TRYSRC can't be used with items as SRC
	/*if ( skill == SKILL_THROWING )		// throwing weapons also have anim of the weapon returning after throw it
	{
		TCHAR *anim = Str_GetTemp();
		sprintf(anim, "TRYSRC %d EFFECT %d,%d,%d,%d,%d,%d,%d", static_cast<int>(pButte->GetUID()), EFFECT_BOLT, AnimID, 18, 1, 0, static_cast<int>(AnimHue), static_cast<int>(AnimRender));
		g_World.m_TimedFunctions.Add(GetUID(), 1, anim);	// TIMERF function
	}*/

	m_atFight.m_Swing_NextAction = CServTime::GetCurrentTime() + (2 * TICK_PER_SEC);

	if ( Skill_UseQuick(skill, Calc_GetRandVal(40)) )
	{
		static LPCTSTR const sm_Txt_ArcheryButte_Success[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT1),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT2),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT3),
			g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_HIT4)
		};
		Emote(sm_Txt_ArcheryButte_Success[Calc_GetRandVal(COUNTOF(sm_Txt_ArcheryButte_Success))]);
		Sound(pWeapon->Weapon_GetSoundHit());

		if ( WeaponAmmoID )
		{
			pButte->m_itArcheryButte.m_AmmoType = WeaponAmmoID;
			pButte->m_itArcheryButte.m_AmmoCount++;
		}
	}
	else
	{
		Emote(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_ARCHBUTTE_MISS));
		Sound(pWeapon->Weapon_GetSoundMiss());
	}
	pAmmo->ConsumeAmount();
	Skill_Experience(skill, Calc_GetRandVal(40));
	return true;
}

bool CChar::Use_Item_Web( CItem * pItemWeb )
{
	ADDTOCALLSTACK("CChar::Use_Item_Web");
	// IT_WEB
	// IT_EQ_STUCK
	// Try to break out of the web.
	// Or just try to damage it.
	//
	// RETURN: true = held in place.
	//  false = walk thru it.

	if ( GetDispID() == CREID_GIANT_SPIDER || !pItemWeb || !pItemWeb->IsTopLevel() || IsStatFlag(STATF_DEAD|STATF_Insubstantial) || IsPriv(PRIV_GM) )
		return false;	// just walk through it

	// Try to break it.
	int iStr = pItemWeb->m_itWeb.m_Hits_Cur;
	if ( iStr == 0 )
		iStr = pItemWeb->m_itWeb.m_Hits_Cur = 60 + Calc_GetRandVal(250);

	// Since broken webs become spider silk, we should get out of here now if we aren't in a web.
	CItem *pFlag = LayerFind(LAYER_FLAG_Stuck);
	if ( CanMove(pItemWeb, false) )
	{
		if ( pFlag )
			pFlag->Delete();
		return false;
	}

	if ( pFlag )
	{
		if ( pFlag->IsTimerSet() )	// don't allow me to try to damage it too often
			return true;
	}

	int iDmg = pItemWeb->OnTakeDamage(Stat_GetAdjusted(STAT_STR), this);
	switch ( iDmg )
	{
		case 0:			// damage blocked
		case 1:			// web survived
		default:		// unknown
			if ( GetTopPoint() == pItemWeb->GetTopPoint() )		// is character still stuck on the web?
				break;

		case 2:			// web turned into silk
		case INT_MAX:	// web destroyed
			if ( pFlag )
				pFlag->Delete();
			return false;
	}

	// Stuck in it still.
	if ( !pFlag )
	{
		if ( iDmg < 0 )
			return false;

		// First time message.
		pFlag = CItem::CreateBase(ITEMID_WEB1_1);
		ASSERT(pFlag);
		pFlag->SetAttr(ATTR_DECAY);
		pFlag->SetType(IT_EQ_STUCK);
		pFlag->m_uidLink = pItemWeb->GetUID();
		pFlag->SetTimeout(pItemWeb->GetTimerDAdjusted());
		LayerAdd(pFlag, LAYER_FLAG_Stuck);
	}
	else
	{
		if ( iDmg < 0 )
		{
			pFlag->Delete();
			return false;
		}
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SWEB_STUCK), pItemWeb->GetName());
	}

	return true;
}

int CChar::Use_PlayMusic( CItem * pInstrument, int iDifficultyToPlay )
{
	ADDTOCALLSTACK("CChar::Use_PlayMusic");
	// SKILL_ENTICEMENT, SKILL_MUSICIANSHIP,
	// ARGS:
	//	iDifficultyToPlay = 0-100
	// RETURN:
	//  >=0 = success
	//  -1 = too hard for u.
	//  -2 = can't play. no instrument.

	if ( !pInstrument )
	{
		pInstrument = ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_MUSICAL), 0, 1);
		if ( !pInstrument )
		{
			SysMessageDefault(DEFMSG_MUSICANSHIP_NOTOOL);
			return -2;
		}
	}

	bool fSuccess = Skill_UseQuick(SKILL_MUSICIANSHIP, iDifficultyToPlay, (Skill_GetActive() != SKILL_MUSICIANSHIP));
	Sound(pInstrument->Use_Music(fSuccess));
	if ( fSuccess )
		return iDifficultyToPlay;	// success

	// Skill gain for SKILL_MUSICIANSHIP failure will need to be triggered
	// manually, since Skill_UseQuick isn't going to do it for us in this case
	if ( Skill_GetActive() == SKILL_MUSICIANSHIP )
		Skill_Experience(SKILL_MUSICIANSHIP, -iDifficultyToPlay);

	SysMessageDefault(DEFMSG_MUSICANSHIP_POOR);
	return -1;		// fail
}

bool CChar::Use_Repair( CItem * pItemArmor )
{
	ADDTOCALLSTACK("CChar::Use_Repair");
	// Attempt to repair the item.
	// If it is repairable.

	if ( !pItemArmor || !pItemArmor->Armor_IsRepairable() )
	{
		SysMessageDefault(DEFMSG_REPAIR_NOT);
		return false;
	}

	if ( pItemArmor->IsItemEquipped() )
	{
		SysMessageDefault(DEFMSG_REPAIR_WORN);
		return false;
	}
	if ( !CanUse(pItemArmor, true) )
	{
		SysMessageDefault(DEFMSG_REPAIR_REACH);
		return false;
	}

	// Quickly use arms lore skill, but don't gain any skill until later on
	int iArmsLoreDiff = Calc_GetRandVal(30);
	if ( !Skill_UseQuick(SKILL_ARMSLORE, iArmsLoreDiff, false) )
	{
		// apply arms lore skillgain for failure
		Skill_Experience(SKILL_ARMSLORE, -iArmsLoreDiff);
		SysMessageDefault(DEFMSG_REPAIR_UNK);
		return false;
	}

	if ( pItemArmor->m_itArmor.m_Hits_Cur >= pItemArmor->m_itArmor.m_Hits_Max )
	{
		SysMessageDefault(DEFMSG_REPAIR_FULL);
		return false;
	}

	m_Act_p = g_World.FindItemTypeNearby(GetTopPoint(), IT_ANVIL, 2, false, true);
	if ( !m_Act_p.IsValidPoint() )
	{
		SysMessageDefault(DEFMSG_REPAIR_ANVIL);
		return false;
	}

	CItemBase *pItemDef = pItemArmor->Item_GetDef();
	ASSERT(pItemDef);

	// Use up some raw materials to repair.
	int iTotalHits = pItemArmor->m_itArmor.m_Hits_Max;
	int iDamageHits = pItemArmor->m_itArmor.m_Hits_Max - pItemArmor->m_itArmor.m_Hits_Cur;
	int iDamagePercent = IMULDIV(100, iDamageHits, iTotalHits);

	size_t iMissing = ResourceConsumePart(&(pItemDef->m_BaseResources), 1, iDamagePercent / 2, true);
	if ( iMissing != pItemDef->m_BaseResources.BadIndex() )
	{
		// Need this to repair.
		const CResourceDef *pCompDef = g_Cfg.ResourceGetDef(pItemDef->m_BaseResources.GetAt(iMissing).GetResourceID());
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_LACK_1), pCompDef ? pCompDef->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_LACK_2));
		return false;
	}

	UpdateDir(m_Act_p);
	UpdateAnimate(ANIM_ATTACK_1H_SLASH);

	// quarter the skill to make it.
	// + more damaged items should be harder to repair.
	// higher the percentage damage the closer to the skills to make it.

	size_t iRes = pItemDef->m_SkillMake.FindResourceType(RES_SKILL);
	if ( iRes == pItemDef->m_SkillMake.BadIndex() )
		return false;

	CResourceQty RetMainSkill = pItemDef->m_SkillMake[iRes];
	int iSkillLevel = static_cast<int>(RetMainSkill.GetResQty()) / 10;
	int iDifficulty = IMULDIV(iSkillLevel, iDamagePercent, 100);
	if ( iDifficulty < iSkillLevel / 4 )
		iDifficulty = iSkillLevel / 4;

	// apply arms lore skillgain now
	LPCTSTR pszText;
	Skill_Experience(SKILL_ARMSLORE, iArmsLoreDiff);
	bool fSuccess = Skill_UseQuick(static_cast<SKILL_TYPE>(RetMainSkill.GetResIndex()), iDifficulty);
	if ( fSuccess )
	{
		pItemArmor->m_itArmor.m_Hits_Cur = static_cast<WORD>(iTotalHits);
		pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_1);
	}
	else
	{
		/*****************************
		// not sure if this is working!
		******************************/
		// Failure
		if ( !Calc_GetRandVal(6) )
		{
			pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_2);
			pItemArmor->m_itArmor.m_Hits_Max--;
			pItemArmor->m_itArmor.m_Hits_Cur--;
		}
		else if ( !Calc_GetRandVal(3) )
		{
			pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_3);
			pItemArmor->m_itArmor.m_Hits_Cur--;
		}
		else
			pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_4 );

		iDamagePercent = Calc_GetRandVal(iDamagePercent);	// some random amount
	}

	ResourceConsumePart(&(pItemDef->m_BaseResources), 1, iDamagePercent / 2, false);
	if ( pItemArmor->m_itArmor.m_Hits_Cur <= 0 )
		pszText = g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_5);

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_REPAIR_MSG), pszText, pItemArmor->GetName());
	Emote(pszMsg);

	if ( pItemArmor->m_itArmor.m_Hits_Cur <= 0 )
		pItemArmor->Delete();
	else
		pItemArmor->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_DURABILITY);

	return fSuccess;
}

bool CChar::Use_EatQty( CItem * pFood, WORD iQty )
{
	ADDTOCALLSTACK("CChar::Use_EatQty");
	// low level eat
	ASSERT(pFood);
	if ( iQty <= 0 )
		return false;

	if ( iQty > pFood->GetAmount() )
		iQty = pFood->GetAmount();

	WORD iRestore = 0;
	if ( pFood->m_itFood.m_foodval )
		iRestore = static_cast<WORD>(pFood->m_itFood.m_foodval);
	else
		iRestore = pFood->Item_GetDef()->GetVolume();	// some food should have more value than other !

	if ( iRestore < 1 )
		iRestore = 1;

	WORD iSpace = static_cast<WORD>(Stat_GetMax(STAT_FOOD) - Stat_GetVal(STAT_FOOD));
	if ( iSpace <= 0 )
		return false;

	if ( (iQty > 1) && (iRestore * iQty > iSpace) )
		iQty = maximum(1, iSpace / iRestore);

	switch ( pFood->GetType() )
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			if ( pFood->m_itFood.m_poison_skill )	// was the food poisoned?
				SetPoison(pFood->m_itFood.m_poison_skill * 10, 1 + (pFood->m_itFood.m_poison_skill / 50), this);
		default:
			break;
	}

	UpdateDir(pFood);
	EatAnim(pFood->GetName(), iRestore * iQty);
	pFood->ConsumeAmount(iQty);
	return true;
}

bool CChar::Use_Eat( CItem * pItemFood, WORD iQty )
{
	ADDTOCALLSTACK("CChar::Use_Eat");
	// What we can eat should depend on body type.
	// How much we can eat should depend on body size and current fullness.
	//
	// ??? monsters should be able to eat corpses / raw meat
	// IT_FOOD or IT_FOOD_RAW
	// NOTE: Some foods like apples are stackable !

	if ( !CanMove(pItemFood) )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTMOVE);
		return false;
	}

	if ( Stat_GetMax(STAT_FOOD) == 0 )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTEAT);
		return false;
	}

	// Is this edible by me ?
	if ( !Food_CanEat(pItemFood) )
	{
		SysMessageDefault(DEFMSG_FOOD_RCANTEAT);
		return false;
	}

	if ( Stat_GetVal(STAT_FOOD) >= Stat_GetMax(STAT_FOOD) )
	{
		SysMessageDefault(DEFMSG_FOOD_CANTEATF);
		return false;
	}

	Use_EatQty(pItemFood, iQty);

	LPCTSTR pMsg;
	int index = IMULDIV(Stat_GetVal(STAT_FOOD), 5, Stat_GetMax(STAT_FOOD));
	switch ( index )
	{
		case 0:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_1);
			break;
		case 1:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_2);
			break;
		case 2:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_3);
			break;
		case 3:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_4);
			break;
		case 4:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_5);
			break;
		case 5:
		default:
			pMsg = g_Cfg.GetDefaultMsg(DEFMSG_FOOD_FULL_6);
			break;
	}
	SysMessage(pMsg);
	return true;
}

bool CChar::Use_Drink( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_Drink");
	// IT_POTION:
	// IT_DRINK:
	// IT_PITCHER:
	// IT_WATER_WASH:
	// IT_BOOZE:

	if ( !CanMove(pItem) )
	{
		SysMessageDefault(DEFMSG_DRINK_CANTMOVE);
		return false;
	}

	const CItemBase *pItemDef = pItem->Item_GetDef();
	ITEMID_TYPE idbottle = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttDrink.m_idEmpty));

	if ( pItem->IsType(IT_BOOZE) )
	{
		// Beer wine and liquor. vary strength of effect.
		int iAlcohol = Calc_GetRandVal(4);

		CItem *pDrunkLayer = LayerFind(LAYER_FLAG_Drunk);
		if ( !pDrunkLayer )
			pDrunkLayer = Spell_Effect_Create(SPELL_Liquor, LAYER_FLAG_Drunk, 0, 5 * TICK_PER_SEC, this);

		pDrunkLayer->m_itSpell.m_spellcharges += iAlcohol;
		if ( pDrunkLayer->m_itSpell.m_spellcharges > 60 )
			pDrunkLayer->m_itSpell.m_spellcharges = 60;
	}

	if ( pItem->IsType(IT_POTION) )
	{
		// Time limit on using potions.
		if ( LayerFind(LAYER_FLAG_PotionUsed) )
		{
			SysMessageDefault(DEFMSG_DRINK_POTION_DELAY);
			return false;
		}

		// Convey the effect of the potion.
		int iSkillQuality = pItem->m_itPotion.m_skillquality;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			int iEnhance = static_cast<int>(GetDefNum("EnhancePotions"));
			if ( iEnhance )
				iSkillQuality += IMULDIV(iSkillQuality, iEnhance, 100);
		}

		OnSpellEffect(static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itPotion.m_Type)), this, iSkillQuality, pItem);

		// Give me the marker that i've used a potion.
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_PotionUsed, iSkillQuality, 15 * TICK_PER_SEC, this);
	}

	if ( pItem->IsType(IT_DRINK) && IsSetOF(OF_DrinkIsFood) )
	{
		short iRestore = 0;
		if ( pItem->m_itDrink.m_foodval )
			iRestore = static_cast<short>(pItem->m_itDrink.m_foodval);
		else
			iRestore = static_cast<short>(pItem->Item_GetDef()->GetVolume());

		if ( iRestore < 1 )
			iRestore = 1;

		if ( Stat_GetVal(STAT_FOOD) >= Stat_GetMax(STAT_FOOD) )
		{
			SysMessageDefault(DEFMSG_DRINK_FULL);
			return false;
		}

		Stat_SetVal(STAT_FOOD, Stat_GetVal(STAT_FOOD) + iRestore);
		if ( pItem->m_itFood.m_poison_skill )
			SetPoison(pItem->m_itFood.m_poison_skill * 10, 1 + (pItem->m_itFood.m_poison_skill / 50), this);
	}

	//Sound(sm_DrinkSounds[Calc_GetRandVal(COUNTOF(sm_DrinkSounds))]);
	UpdateAnimate(ANIM_EAT);
	pItem->ConsumeAmount();

	// Create the empty bottle ?
	if ( idbottle != ITEMID_NOTHING )
		ItemBounce(CItem::CreateScript(idbottle, this), false);
	return true;
}

CChar * CChar::Use_Figurine( CItem * pItem, bool bCheckFollowerSlots )
{
	ADDTOCALLSTACK("CChar::Use_Figurine");
	// NOTE: The figurine is NOT destroyed.
	bool bCreatedNewNpc = false;
	if ( !pItem )
		return NULL;

	if ( pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsChar() && pItem->m_uidLink != GetUID() && !IsPriv(PRIV_GM) )
	{
		SysMessageDefault(DEFMSG_MSG_FIGURINE_NOTYOURS);
		return NULL;
	}

	// Create a new NPC if there's no one linked to this figurine 
	CChar *pPet = pItem->m_itFigurine.m_UID.CharFind();
	if ( !pPet )
	{
		CREID_TYPE id = pItem->m_itFigurine.m_ID;
		if ( !id )
		{
			id = CItemBase::FindCharTrack(pItem->GetID());
			if ( !id )
			{
				DEBUG_ERR(("FIGURINE id 0%x, no creature\n", pItem->GetDispID()));
				return NULL;
			}
		}
		bCreatedNewNpc = true;
		pPet = CreateNPC(id);
		ASSERT(pPet);
		pPet->SetName(pItem->GetName());
		if ( pItem->GetHue() )
		{
			pPet->m_prev_Hue = pItem->GetHue();
			pPet->SetHue(pItem->GetHue());
		}
	}

	if ( bCheckFollowerSlots && IsSetOF(OF_PetSlots) )
	{
		if ( !FollowersUpdate(pPet, static_cast<short>(maximum(1, pPet->GetDefNum("FOLLOWERSLOTS", true))), true) )
		{
			SysMessageDefault(DEFMSG_PETSLOTS_TRY_CONTROL);
			if ( bCreatedNewNpc )
				pPet->Delete();
			return NULL;
		}
	}

	if ( pPet->IsDisconnected() )
		pPet->StatFlag_Clear(STATF_Ridden);		// pull the creature out of IDLE space

	pItem->m_itFigurine.m_UID.InitUID();
	pPet->m_dirFace = m_dirFace;
	pPet->NPC_PetSetOwner(this, false);
	pPet->MoveToChar(pItem->GetTopLevelObj()->GetTopPoint());
	pPet->Update();
	pPet->Skill_Start(SKILL_NONE);
	pPet->SoundChar(CRESND_RAND1);
	return pPet;
}

bool CChar::FollowersUpdate( CChar * pChar, short iFollowerSlots, bool bCheckOnly )
{
	ADDTOCALLSTACK("CChar::FollowersUpdate");
	// Attemp to update followers on this character based on pChar
	// bSustract = true for pet's release, shrink, etc ...
	// This is supossed to be called only when OF_PetSlots is enabled, so no need to check it here.

	if ( !bCheckOnly && IsTrigUsed(TRIGGER_FOLLOWERSUPDATE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = (iFollowerSlots > 0) ? 0 : 1;
		Args.m_iN2 = abs(iFollowerSlots);
		if ( OnTrigger(CTRIG_FollowersUpdate, pChar, &Args) == TRIGRET_RET_TRUE )
			return false;

		iFollowerSlots = static_cast<short>(Args.m_iN2);
	}

	short iCurFollower = static_cast<short>(GetDefNum("CURFOLLOWER", true));
	short iMaxFollower = static_cast<short>(GetDefNum("MAXFOLLOWER", true));
	short iSetFollower = iCurFollower + iFollowerSlots;

	if ( (iSetFollower > iMaxFollower) && !IsPriv(PRIV_GM) )
		return false;

	if ( !bCheckOnly )
	{
		SetDefNum("CURFOLLOWER", maximum(iSetFollower, 0));
		UpdateStatsFlag();
	}
	return true;
}

bool CChar::Use_Key( CItem * pKey, CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_Key");
	ASSERT(pKey);
	ASSERT(pKey->IsType(IT_KEY));
	if ( !pItemTarg )
	{
		SysMessageDefault(DEFMSG_MSG_KEY_TARG);
		return false;
	}

	if ( pKey != pItemTarg && pItemTarg->IsType(IT_KEY) )
	{
		// We are trying to copy a key ?
		if ( !CanUse(pItemTarg, true) )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_REACH);
			return false;
		}

		if ( !pKey->m_itKey.m_lockUID && !pItemTarg->m_itKey.m_lockUID )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_BLANKS);
			return false;
		}
		if ( pItemTarg->m_itKey.m_lockUID && pKey->m_itKey.m_lockUID )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_NOTBLANKS);
			return false;
		}

		// Need tinkering tools ???
		if ( !Skill_UseQuick(SKILL_TINKERING, 30 + Calc_GetRandVal(40)) )
		{
			SysMessageDefault(DEFMSG_MSG_KEY_FAILC);
			return false;
		}
		if ( pItemTarg->m_itKey.m_lockUID )
			pKey->m_itKey.m_lockUID = pItemTarg->m_itKey.m_lockUID;
		else
			pItemTarg->m_itKey.m_lockUID = pKey->m_itKey.m_lockUID;
		return true;
	}

	if ( !pKey->m_itKey.m_lockUID )
	{
		SysMessageDefault(DEFMSG_MSG_KEY_ISBLANK);
		return false;
	}
	if ( pKey == pItemTarg )	// rename the key
	{
		if ( m_pClient )
			m_pClient->addPromptConsole(CLIMODE_PROMPT_NAME_KEY, g_Cfg.GetDefaultMsg(DEFMSG_MSG_KEY_SETNAME), pKey->GetUID());
		return false;
	}

	if ( !CanUse(pItemTarg, false) )
	{
		SysMessageDefault(DEFMSG_MSG_KEY_CANTREACH);
		return false;
	}

	if ( m_pArea->GetResourceID() == pKey->m_itKey.m_lockUID )
	{
		if ( Use_MultiLockDown(pItemTarg) )
			return true;
	}

	if ( !pItemTarg->m_itContainer.m_lockUID )	// or m_itContainer.m_lockUID
	{
		SysMessageDefault(DEFMSG_MSG_KEY_NOLOCK);
		return false;
	}
	if ( !pKey->IsKeyLockFit(pItemTarg->m_itContainer.m_lockUID) )	// or m_itKey
	{
		SysMessageDefault(DEFMSG_MSG_KEY_WRONGLOCK);
		return false;
	}

	return Use_KeyChange(pItemTarg);
}

bool CChar::Use_KeyChange( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_KeyChange");
	// Lock or unlock the item.
	switch ( pItemTarg->GetType() )
	{
		case IT_SIGN_GUMP:
			// We may rename the sign.
			if ( m_pClient )
				m_pClient->addPromptConsole(CLIMODE_PROMPT_NAME_SIGN, g_Cfg.GetDefaultMsg(DEFMSG_MSG_KEY_TARG_SIGN), pItemTarg->GetUID());
			return true;
		case IT_CONTAINER:
			pItemTarg->SetType(IT_CONTAINER_LOCKED);
			pItemTarg->ResendTooltip();
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_CONT_LOCK);
			break;
		case IT_CONTAINER_LOCKED:
			pItemTarg->SetType(IT_CONTAINER);
			pItemTarg->ResendTooltip();
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_CONT_ULOCK);
			break;
		case IT_SHIP_HOLD:
			pItemTarg->SetType(IT_SHIP_HOLD_LOCK);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_HOLD_LOCK);
			break;
		case IT_SHIP_HOLD_LOCK:
			pItemTarg->SetType(IT_SHIP_HOLD);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_HOLD_ULOCK);
			break;
		case IT_DOOR:
		case IT_DOOR_OPEN:
			pItemTarg->SetType(IT_DOOR_LOCKED);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_DOOR_LOCK);
			break;
		case IT_DOOR_LOCKED:
			pItemTarg->SetType(IT_DOOR);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_DOOR_ULOCK);
			break;
		case IT_SHIP_TILLER:
			if ( m_pClient )
				m_pClient->addPromptConsole(CLIMODE_PROMPT_NAME_SHIP, g_Cfg.GetDefaultMsg(DEFMSG_MSG_SHIPNAME_PROMT), pItemTarg->GetUID());
			return true;
		case IT_SHIP_PLANK:
			pItemTarg->Ship_Plank(false);	// just close it.
			if ( pItemTarg->GetType() == IT_SHIP_SIDE_LOCKED )
			{
				pItemTarg->SetType(IT_SHIP_SIDE);
				SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_ULOCK);
				break;
			}
			// Then fall thru and lock it.
		case IT_SHIP_SIDE:
			pItemTarg->SetType(IT_SHIP_SIDE_LOCKED);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_LOCK);
			break;
		case IT_SHIP_SIDE_LOCKED:
			pItemTarg->SetType(IT_SHIP_SIDE);
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_SHIP_ULOCK);
			break;
		default:
			SysMessageDefault(DEFMSG_MSG_KEY_TARG_NOLOCK);
			return false;
	}
	return true;
}

bool CChar::Use_Seed( CItem * pSeed, CPointMap * pPoint )
{
	ADDTOCALLSTACK("CChar::Use_Seed");
	// Use the seed at the current point on the ground or some new point that i can touch.
	// IT_SEED from IT_FRUIT

	ASSERT(pSeed);
	CPointMap pt;
	if ( pPoint )
		pt = *pPoint;
	else if ( pSeed->IsTopLevel() )
		pt = pSeed->GetTopPoint();
	else
		pt = GetTopPoint();

	if ( !CanTouch(pt) )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_REACH);
		return false;
	}

	// is there soil here ? IT_DIRT
	if ( !IsPriv(PRIV_GM) && !g_World.IsItemTypeNear(pt, IT_DIRT, 0, false, true) )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_TARGSOIL);
		return(false);
	}

	const CItemBase *pItemDef = pSeed->Item_GetDef();
	ITEMID_TYPE idReset = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttFruit.m_idReset));
	if ( idReset == 0 )
	{
		SysMessageDefault(DEFMSG_MSG_SEED_NOGOOD);
		return false;
	}

	// Already a plant here ?
	CWorldSearch AreaItems(pt);
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( pItem->IsType(IT_TREE) || pItem->IsType(IT_FOLIAGE) )		// there's already a tree here
		{
			SysMessageDefault(DEFMSG_MSG_SEED_ATREE);
			return false;
		}
		if ( pItem->IsType(IT_CROPS) )		// there's already a plant here
			pItem->Delete();
	}

	// plant it and consume the seed.

	CItem *pPlant = CItem::CreateScript(idReset, this);
	ASSERT(pPlant);

	pPlant->MoveToUpdate(pt);
	if ( pPlant->IsType(IT_CROPS) || pPlant->IsType(IT_FOLIAGE) )
	{
		pPlant->m_itCrop.m_ReapFruitID = pSeed->GetID();
		pPlant->Plant_CropReset();
	}
	else
	{
		pPlant->SetDecayTime(10 * g_Cfg.m_iDecay_Item);
	}

	pSeed->ConsumeAmount();
	return true;
}

bool CChar::Use_BedRoll( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_BedRoll");
	// IT_BEDROLL

	ASSERT(pItem);
	switch ( pItem->GetDispID() )
	{
		case ITEMID_BEDROLL_C:
			if ( !pItem->IsTopLevel() )
			{
			putonground:
				SysMessageDefault(DEFMSG_ITEMUSE_BEDROLL);
				return true;
			}
			pItem->SetID(Calc_GetRandVal(2) ? ITEMID_BEDROLL_O_EW : ITEMID_BEDROLL_O_NS);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_C_NS:
			if ( !pItem->IsTopLevel() )
				goto putonground;
			pItem->SetID(ITEMID_BEDROLL_O_NS);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_C_EW:
			if ( !pItem->IsTopLevel() )
				goto putonground;
			pItem->SetID(ITEMID_BEDROLL_O_EW);
			pItem->Update();
			return true;
		case ITEMID_BEDROLL_O_EW:
		case ITEMID_BEDROLL_O_NS:
			pItem->SetID(ITEMID_BEDROLL_C);
			pItem->Update();
			return true;
		default:
			return false;
	}
}

int CChar::Do_Use_Item(CItem *pItem, bool fLink)
{
	ADDTOCALLSTACK("CChar::Do_Use_Item");
	if ( !pItem )
		return false;

	if ( m_pNPC && (IsTrigUsed(TRIGGER_DCLICK) || IsTrigUsed(TRIGGER_ITEMDCLICK)) )	// for players, DClick was called before this function
	{
		if ( pItem->OnTrigger(ITRIG_DCLICK, this) == TRIGRET_RET_TRUE )
			return false;
	}

	CItemSpawn *pSpawn = static_cast<CItemSpawn *>(pItem->m_uidSpawnItem.ItemFind());
	if ( pSpawn )
		pSpawn->DelObj(pItem->GetUID());	// remove this item from it's spawn when DClicks it

	int fAction = true;
	switch ( pItem->GetType() )
	{
		case IT_ITEM_STONE:
		{
			// Give them this item
			if ( pItem->m_itItemStone.m_wAmount == USHRT_MAX )
			{
				SysMessageDefault(DEFMSG_MSG_IT_IS_DEAD);
				return true;
			}
			if ( pItem->m_itItemStone.m_wRegenTime )
			{
				if ( pItem->IsTimerSet() )
				{
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_STONEREG_TIME), pItem->GetTimerDiff() / TICK_PER_SEC);
					return true;
				}
				pItem->SetTimeout(pItem->m_itItemStone.m_wRegenTime * TICK_PER_SEC);
			}
			ItemBounce(CItem::CreateTemplate(pItem->m_itItemStone.m_ItemID, GetContainerCreate(LAYER_PACK), this));
			if ( pItem->m_itItemStone.m_wAmount != 0 )
			{
				pItem->m_itItemStone.m_wAmount--;
				if ( pItem->m_itItemStone.m_wAmount == 0 )
					pItem->m_itItemStone.m_wAmount = USHRT_MAX;
			}
			return true;
		}

		case IT_SEED:
			return Use_Seed(pItem, NULL);

		case IT_BEDROLL:
			return Use_BedRoll(pItem);

		case IT_KINDLING:
			return Use_Kindling(pItem);

		case IT_SPINWHEEL:
		{
			if ( fLink )
				return false;

			// Just make them spin
			pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetID() + 1), 2 * TICK_PER_SEC);
			SysMessageDefault(DEFMSG_ITEMUSE_SPINWHEEL);
			return true;
		}

		case IT_TRAIN_DUMMY:
		{
			if ( fLink )
				return false;

			Use_Train_Dummy(pItem, true);
			return true;
		}

		case IT_TRAIN_PICKPOCKET:
		{
			if ( fLink )
				return false;

			Use_Train_PickPocketDip(pItem, true);
			return true;
		}

		case IT_ARCHERY_BUTTE:
		{
			if ( fLink )
				return false;

			Use_Train_ArcheryButte(pItem, true);
			return true;
		}

		case IT_LOOM:
		{
			if ( fLink )
				return false;

			SysMessageDefault(DEFMSG_ITEMUSE_LOOM);
			return true;
		}

		case IT_BEE_HIVE:
		{
			if ( fLink )
				return false;

			// Get honey from it
			ITEMID_TYPE id = ITEMID_NOTHING;
			if ( !pItem->m_itBeeHive.m_honeycount )
				SysMessageDefault(DEFMSG_ITEMUSE_BEEHIVE);
			else
			{
				switch ( Calc_GetRandVal(3) )
				{
					case 1:
						id = ITEMID_JAR_HONEY;
						break;
					case 2:
						id = ITEMID_BEE_WAX;
						break;
					default:
						break;
				}
			}
			if ( id )
			{
				ItemBounce(CItem::CreateScript(id, this));
				pItem->m_itBeeHive.m_honeycount--;
			}
			else
			{
				SysMessageDefault(DEFMSG_ITEMUSE_BEEHIVE_STING);
				OnTakeDamage(Calc_GetRandVal(5), this, DAMAGE_POISON|DAMAGE_GENERAL);
			}
			pItem->SetTimeout(15 * 60 * TICK_PER_SEC);
			return true;
		}

		case IT_MUSICAL:
		{
			if ( !Skill_Wait(SKILL_MUSICIANSHIP) )
			{
				m_Act_Targ = pItem->GetUID();
				Skill_Start(SKILL_MUSICIANSHIP);
			}
			break;
		}

		case IT_CROPS:
		case IT_FOLIAGE:
		{
			// Pick cotton/hay/etc
			fAction = pItem->Plant_Use(this);
			break;
		}

		case IT_FIGURINE:
		{
			// Create the creature here
			if ( Use_Figurine(pItem) )
				pItem->Delete();
			return true;
		}

		case IT_TRAP:
		case IT_TRAP_ACTIVE:
		{
			// Activate the trap (plus any linked traps)
			int iDmg = pItem->Use_Trap();
			if ( CanTouch(pItem->GetTopLevelObj()->GetTopPoint()) )
				OnTakeDamage(iDmg, NULL, DAMAGE_HIT_BLUNT|DAMAGE_GENERAL);
			break;
		}

		case IT_SWITCH:
		{
			// Switches can be linked to gates and doors and such.
			// Flip the switch graphic.
			pItem->SetSwitchState();
			break;
		}

		case IT_PORT_LOCKED:
			if ( !fLink && !IsPriv(PRIV_GM) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_PORT_LOCKED);
				return true;
			}
		case IT_PORTCULIS:
			// Open a metal gate vertically
			pItem->Use_Portculis();
			break;

		case IT_DOOR_LOCKED:
			if ( !ContentFindKeyFor(pItem) )
			{
				SysMessageDefault(DEFMSG_MSG_KEY_DOORLOCKED);
				if ( !pItem->IsTopLevel() )
					return false;
				if ( pItem->IsAttr(ATTR_MAGIC) )	// show it's magic face
				{
					ITEMID_TYPE id = (GetDispID() & DOOR_NORTHSOUTH) ? ITEMID_DOOR_MAGIC_SI_NS : ITEMID_DOOR_MAGIC_SI_EW;
					CItem *pFace = CItem::CreateBase(id);
					ASSERT(pFace);
					pFace->MoveToDecay(pItem->GetTopPoint(), 4 * TICK_PER_SEC);
				}
				if ( !IsPriv(PRIV_GM) )
					return true;
			}
		case IT_DOOR_OPEN:
		case IT_DOOR:
		{
			bool fOpen = pItem->Use_DoorNew(fLink);
			if ( fLink || !fOpen )	// don't link if we are just closing the door
				return true;
		}
		break;

		case IT_SHIP_PLANK:
		{
			// Close the plank if I'm inside the ship
			if ( m_pArea->IsFlag(REGION_FLAG_SHIP) && m_pArea->GetResourceID() == pItem->m_uidLink )
			{
				if ( pItem->m_itShipPlank.m_itSideType == IT_SHIP_SIDE_LOCKED && !ContentFindKeyFor(pItem) )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_SHIPSIDE);
					return true;
				}
				return pItem->Ship_Plank(false);
			}
			else if ( pItem->IsTopLevel() )
			{
				// Teleport to plank if I'm outside the ship
				CPointMap pntTarg = pItem->GetTopPoint();
				pntTarg.m_z++;
				Spell_Teleport(pntTarg, true, false, false);
			}
			return true;
		}

		case IT_SHIP_SIDE_LOCKED:
			if ( !ContentFindKeyFor(pItem) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_SHIPSIDE);
				return true;
			}
		case IT_SHIP_SIDE:
			// Open the plank
			pItem->Ship_Plank(true);
			return true;

		case IT_GRAIN:
		case IT_GRASS:
		case IT_GARBAGE:
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
		{
			if ( fLink )
				return false;

			Use_Eat(pItem);
			return true;
		}

		case IT_POTION:
		case IT_DRINK:
		case IT_PITCHER:
		case IT_WATER_WASH:
		case IT_BOOZE:
		{
			if ( fLink )
				return false;

			Use_Drink(pItem);
			return true;
		}

		case IT_LIGHT_OUT:		// can the light be lit?
		case IT_LIGHT_LIT:		// can the light be doused?
			fAction = pItem->Use_Light();
			break;

		case IT_CLOTHING:
		case IT_ARMOR:
		case IT_ARMOR_LEATHER:
		case IT_SHIELD:
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
		case IT_WEAPON_AXE:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_MACE_STAFF:
		case IT_JEWELRY:
		case IT_WEAPON_THROWING:
		{
			if ( fLink )
				return false;

			return ItemEquip(pItem);
		}

		case IT_WEB:
		{
			if ( fLink )
				return false;

			Use_Item_Web(pItem);
			return true;
		}

		case IT_SPY_GLASS:
		{
			if ( fLink )
				return false;

			// Spyglass will tell you the moon phases
			static LPCTSTR const sm_sPhases[8] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M1),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M2),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M3),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M4),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M5),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M6),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M7),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_M8)
			};
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_TR), sm_sPhases[g_World.GetMoonPhase(false)]);
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SPYGLASS_FE), sm_sPhases[g_World.GetMoonPhase(true)]);

			if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_SHIP) )
				ObjMessage(pItem->Use_SpyGlass(this), this);
			return true;
		}

		case IT_SEXTANT:
		{
			if ( fLink )
				return false;

			if ( (GetTopPoint().m_map <= 1) && (GetTopPoint().m_x > UO_SIZE_X_REAL) )		// dungeons and T2A lands
				ObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEXTANT_T2A), this);
			else
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_SEXTANT), m_pArea->GetName(), pItem->Use_Sextant(GetTopPoint()));
				ObjMessage(pszMsg, this);
			}
			return true;
		}

		default:
			fAction = false;
	}
	return fAction | MASK_RETURN_FOLLOW_LINKS;
}

bool CChar::Use_Item(CItem *pItem, bool fLink)
{
	ADDTOCALLSTACK("CChar::Use_Item");
	int result = Do_Use_Item(pItem, fLink);
	if ( (result & MASK_RETURN_FOLLOW_LINKS) == MASK_RETURN_FOLLOW_LINKS )
	{
		CItem *pLinkItem = pItem;
		for ( int i = 0; i < 64; i++ )	// dumb protection for endless loop
		{
			pLinkItem = pLinkItem->m_uidLink.ItemFind();
			if ( !pLinkItem || pLinkItem == pItem )
				break;

			result |= Do_Use_Item(pLinkItem, true);
		}
	}
	return (result & ~MASK_RETURN_FOLLOW_LINKS) ? true : false;
}

bool CChar::Use_Obj( CObjBase * pObj, bool fTestTouch, bool fScript )
{
	ADDTOCALLSTACK("CChar::Use_Obj");
	if ( !pObj )
		return false;
	if ( m_pClient )
		return m_pClient->Event_DoubleClick(pObj->GetUID(), false, fTestTouch, fScript);
	else
		return Use_Item(dynamic_cast<CItem *>(pObj), fTestTouch);
}

bool CChar::ItemEquipArmor( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipArmor");
	// Equip ourselves as best as possible.

	CCharBase *pCharDef = Char_GetDef();
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( !pPack || !pCharDef || !pCharDef->Can(CAN_C_EQUIP) )
		return false;

	int iBestScore[LAYER_HORSE];
	memset(iBestScore, 0, sizeof(iBestScore));
	CItem *pBestArmor[LAYER_HORSE];
	memset(pBestArmor, 0, sizeof(pBestArmor));

	if ( !fForce )
	{
		// Block those layers that are already used
		for ( size_t i = 0; i < COUNTOF(iBestScore); i++ )
		{
			pBestArmor[i] = LayerFind(static_cast<LAYER_TYPE>(i));
			if ( pBestArmor[i] )
				iBestScore[i] = INT_MAX;
		}
	}

	for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		int iScore = pItem->Armor_GetDefense();
		if ( !iScore )	// might not be armor
			continue;

		// Can I even equip this?
		LAYER_TYPE layer = CanEquipLayer(pItem, LAYER_QTY, NULL, true);
		if ( layer == LAYER_NONE )
			continue;

		if ( iScore > iBestScore[layer] )
		{
			iBestScore[layer] = iScore;
			pBestArmor[layer] = pItem;
		}
	}

	// Equip all the stuff we found
	for ( size_t i = 0; i < COUNTOF(iBestScore); i++ )
	{
		if ( pBestArmor[i] )
			ItemEquip(pBestArmor[i], this);
	}

	return true;
}

bool CChar::ItemEquipWeapon( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipWeapon");
	// Find my best weapon and equip it
	if ( !fForce && m_uidWeapon.IsValidUID() )	// we already have a weapon equipped
		return true;

	CCharBase *pCharDef = Char_GetDef();
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( !pPack || !pCharDef || !pCharDef->Can(CAN_C_USEHANDS) )
		return false;

	// Loop through all my weapons and come up with a score for it's usefulness

	CItem *pBestWeapon = NULL;
	int iWeaponScoreMax = NPC_GetWeaponUseScore(NULL);	// wrestling

	for ( CItem *pItem = pPack->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		int iWeaponScore = NPC_GetWeaponUseScore(pItem);
		if ( iWeaponScore > iWeaponScoreMax )
		{
			iWeaponScoreMax = iWeaponScore;
			pBestWeapon = pItem;
		}
	}

	if ( pBestWeapon )
		return ItemEquip(pBestWeapon);

	return true;
}
