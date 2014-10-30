//
// CCharUse.cpp
// Copyright Menace Software (www.menasoft.com).
//  CChar is either an NPC or a Player.
//

#include "graysvr.h"	// predef header.

static const WORD sm_Item_Blood[] =
{
	0x1cf1,
	ITEMID_BLOOD1,
	ITEMID_BLOOD6
};

bool CChar::Use_MultiLockDown( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_MultiLockDown");
	ASSERT(pItemTarg);
	ASSERT(m_pArea);

	if ( ! pItemTarg->IsMovableType())
		return( false );
	if ( pItemTarg->IsType(IT_KEY))
		return( false );
	if ( !pItemTarg->IsTopLevel() )
		return( false );

	// I am in my house ?
	if ( ! pItemTarg->m_uidLink.IsValidUID())
	{
		// If we are in a house then lock down the item.
		pItemTarg->m_uidLink.SetPrivateUID( m_pArea->GetResourceID());
		SysMessageDefault( DEFMSG_MULTI_LOCKDOWN );
		return( true );
	}
	if ( pItemTarg->m_uidLink == m_pArea->GetResourceID() )
	{
		pItemTarg->m_uidLink.InitUID();
		SysMessageDefault( DEFMSG_MULTI_LOCKUP );
		return( true );
	}

	return( false );
}

void CChar::Use_CarveCorpse( CItemCorpse * pCorpse )
{
	ADDTOCALLSTACK("CChar::Use_CarveCorpse");
	if ( !pCorpse )
		return;

	UpdateAnimate( ANIM_BOW );

	CChar * pChar = pCorpse->m_uidLink.CharFind();
	if ( pChar && pChar->IsStatFlag( STATF_Sleeping ))
	{
		// They are violently awakened.
		SysMessageDefault( DEFMSG_SLEEP_AWAKE_2 );
		pChar->SysMessageDefault( DEFMSG_SLEEP_AWAKE_1 );
		pChar->OnTakeDamage( 1 + Calc_GetRandVal(10), this, DAMAGE_HIT_SLASH|DAMAGE_GENERAL );
		return;
	}

	CPointMap pnt = pCorpse->GetTopLevelObj()->GetTopPoint();

	bool fHumanCorpse = CCharBase::IsPlayableID( pCorpse->GetCorpseType());
	if ( fHumanCorpse )
	{
		// If it's a player corpse, put the stuff in one of these and (mabey)
		// flag them a criminal

		if ( pChar && pChar->m_pPlayer )
		{
			// Until the corse inherits NPC criminal status, only do
			// other players
			if ( CheckCorpseCrime( pCorpse, false, false ))
			{
				SysMessageDefault( DEFMSG_CARVE_CORPSE_1 );
			}
		}

		// Dump any stuff on corpse.
		pCorpse->ContentsDump( pnt );
	}

	CREID_TYPE CorpseID = static_cast<CREID_TYPE>(pCorpse->m_itCorpse.m_BaseID);
	const CCharBase * pCorpseDef = CCharBase::FindCharBase( CorpseID );
	if ( pCorpseDef != NULL )
	{
		if ( pCorpseDef->m_wBloodHue != static_cast<HUE_TYPE>(-1) )
		{
			int iBloodLeft = static_cast<int>(pCorpse->m_TagDefs.GetKeyNum("BLOOD", true));
			if ( iBloodLeft )
			{
				pCorpse->m_TagDefs.SetNum("BLOOD", iBloodLeft-1, true);
				CItem * pBlood = CItem::CreateBase( (ITEMID_TYPE) sm_Item_Blood[ Calc_GetRandVal( COUNTOF( sm_Item_Blood )) ] );
				ASSERT(pBlood);
				pBlood->SetHue( pCorpseDef->m_wBloodHue );
				pBlood->MoveToDecay(pCorpse->GetTopPoint(), 60*TICK_PER_SEC);
			}
		}
	}

	pCorpse->m_itCorpse.m_uidKiller = GetUID();	// by you.

	// based on corpse type.
	if ( pCorpseDef == NULL ||
		! pCorpse->GetTimeStamp().IsTimeValid() )
	{
		SysMessageDefault( DEFMSG_CARVE_CORPSE_2 );
		pCorpse->SetTimeStamp(0);
		return;
	}

	pCorpse->SetTimeStamp(0);	// been carved.

	LPCTSTR pszMsg = NULL;
	size_t iItems = 0;
	for ( size_t i = 0; i < pCorpseDef->m_BaseResources.GetCount(); i++ )
	{
		int iQty = static_cast<int>(pCorpseDef->m_BaseResources[i].GetResQty());
		RESOURCE_ID rid = pCorpseDef->m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
		{
			continue;
		}

		ITEMID_TYPE id = static_cast<ITEMID_TYPE>(rid.GetResIndex());
		if ( id == ITEMID_NOTHING )
			break;

		CItem * pPart = CItem::CreateTemplate( id, NULL, this );
		ASSERT(pPart);

		LPCTSTR pszMsgNew = NULL;
		switch ( pPart->GetType())
		{
			case IT_FOOD:
			case IT_FOOD_RAW:	// meat
			case IT_MEAT_RAW:
				pszMsgNew = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_MEAT );
				pPart->m_itFood.m_MeatType = CorpseID;
				break;
			case IT_HIDE:	// hides
			case IT_LEATHER:	// hides
				pszMsgNew = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_HIDES );
				pPart->m_itSkin.m_creid = CorpseID;
				break;
			case IT_FEATHER:	// feathers.
				pszMsgNew = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_FEATHERS );
				pPart->m_itSkin.m_creid = CorpseID;
				break;
			case IT_FUR:	// fur
				pszMsgNew = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_FUR );
				pPart->m_itSkin.m_creid = CorpseID;
				break;
			case IT_WOOL:	// wool
				pszMsgNew = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_WOOL );
				pPart->m_itSkin.m_creid = CorpseID;
				break;
			case IT_BLOOD:
				if ( pCorpseDef->m_wBloodHue != static_cast<HUE_TYPE>(-1) )
				{
					pPart->SetHue( pCorpseDef->m_wBloodHue );
				}
				pPart->m_itSkin.m_creid = CorpseID;
				break;
			default:
				break;
		}

		if ( pszMsg == NULL )
			pszMsg = pszMsgNew;
		if ( iQty > 1 )
			pPart->SetAmount( iQty );

		if ( !fHumanCorpse )
		{
			pCorpse->ContentAdd( pPart );
		}
		else
		{
			if ( pChar )
			{
				TCHAR * pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_CORPSE_NAME ), static_cast<LPCTSTR>(pPart->GetName()), static_cast<LPCTSTR>(pChar->GetName()));
				pPart->SetName(pszMsg);
				pPart->m_uidLink = pChar->GetUID();
			}
			pPart->MoveToDecay( pnt, pPart->GetDecayTime() );
		}

		iItems++;
	}

	if ( iItems <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_CARVE_CORPSE_2 );
	}
	if ( pszMsg != NULL )
	{
		SysMessage( pszMsg );
	}

	if ( pCorpseDef->m_wBloodHue != static_cast<HUE_TYPE>(-1) )
	{
		int iBloodLeft = static_cast<int>(pCorpse->m_TagDefs.GetKeyNum("BLOOD", true));
		if ( iBloodLeft )
		{
			pCorpse->m_TagDefs.SetNum("BLOOD", iBloodLeft-1, true);

			CItem * pBlood = CItem::CreateBase( ITEMID_BLOOD1 );
			ASSERT(pBlood);
			pBlood->SetHue( pCorpseDef->m_wBloodHue );
			pBlood->MoveToDecay( pnt, 3*60*TICK_PER_SEC );
		}
	}

	if ( fHumanCorpse )
	{
		pCorpse->Delete();
	}
}

void CChar::Use_MoonGate( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_MoonGate");
	ASSERT(pItem);

	bool fQuiet = ( pItem->m_itTelepad.m_fQuiet != 0);
	CPointMap ptTeleport = pItem->m_itTelepad.m_pntMark;
	if ( pItem->IsType(IT_MOONGATE))
	{
		// RES_MOONGATES
		// What gate are we at ?
		size_t iCount = g_Cfg.m_MoonGates.GetCount();
		size_t i = 0;
		for ( ; ; i++ )
		{
			if ( i >= iCount)
				return;
			int iDist = GetTopPoint().GetDist( g_Cfg.m_MoonGates[i] );
			if ( iDist <= UO_MAP_VIEW_SIZE )
				break;
		}

		// Set it's current destination based on the moon phases.
		// ensure iTrammelPhrase isn't smaller than iFeluccaPhase, to avoid uint underflow in next calculation
		unsigned int iTrammelPhase = g_World.GetMoonPhase( false ) % iCount;
		unsigned int iFeluccaPhase = g_World.GetMoonPhase( true ) % iCount;
		if (iTrammelPhase < iFeluccaPhase)
			iTrammelPhase += iCount;

		size_t iMoongateIndex = (i + (iTrammelPhase - iFeluccaPhase)) % iCount;
		ASSERT(g_Cfg.m_MoonGates.IsValidIndex(iMoongateIndex));

		ptTeleport = g_Cfg.m_MoonGates[iMoongateIndex];
	}

	if ( ! m_pPlayer )
	{
		// NPCs and gates.
		ASSERT(m_pNPC);
		if ( pItem->m_itTelepad.m_fPlayerOnly )
		{
			return;
		}
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
		{
			// Guards won't gate into unguarded areas.
			CRegionWorld * pArea = dynamic_cast <CRegionWorld *> ( ptTeleport.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( pArea == NULL || ! pArea->IsGuarded())
				return;
		}
		if ( Noto_IsCriminal())
		{
			// wont teleport to guarded areas.
			CRegionWorld * pArea = dynamic_cast <CRegionWorld *> ( ptTeleport.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA));
			if ( pArea == NULL || pArea->IsGuarded())
				return;
		}
	}

	// Teleport me. and take a step.
	Spell_Teleport( ptTeleport, true,
		(pItem->IsAttr(ATTR_DECAY))?true:false,
		(fQuiet) ? ITEMID_NOTHING : ITEMID_FX_TELE_VANISH );
}

bool CChar::Use_Kindling( CItem * pKindling )
{
	ADDTOCALLSTACK("CChar::Use_Kindling");
	ASSERT(pKindling);
	if ( !pKindling->IsTopLevel() )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_KINDLING_CONT );
		return false ;
	}

	if ( ! Skill_UseQuick( SKILL_CAMPING, Calc_GetRandLLVal(30)))
	{
		SysMessageDefault( DEFMSG_ITEMUSE_KINDLING_FAIL );
		return false;
	}

	pKindling->SetID( ITEMID_CAMPFIRE );
	pKindling->SetAttr(ATTR_MOVE_NEVER | ATTR_CAN_DECAY);
	pKindling->SetTimeout( (4+pKindling->GetAmount())*60*TICK_PER_SEC );
	pKindling->SetAmount(1);	// All kindling is set to one fire.
	pKindling->m_itLight.m_pattern = LIGHT_LARGE;
	pKindling->Update();
	pKindling->Sound( 0x226 );
	return( true );
}

bool CChar::Use_Cannon_Feed( CItem * pCannon, CItem * pFeed )
{
	ADDTOCALLSTACK("CChar::Use_Cannon_Feed");
	if ( pCannon != NULL &&
		pCannon->IsType(IT_CANNON_MUZZLE) &&
		pFeed != NULL )
	{
		if ( ! CanUse( pCannon, false ))
			return( false );
		if ( ! CanUse( pFeed, true ))
			return( false );

		if ( pFeed->GetID() == ITEMID_REAG_SA )
		{
			if ( pCannon->m_itCannon.m_Load & 1 )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_CANNON_HPOWDER );
				return( false );
			}
			pCannon->m_itCannon.m_Load |= 1;
			SysMessageDefault( DEFMSG_ITEMUSE_CANNON_LPOWDER );
			return( true );
		}

		if ( pFeed->IsType(IT_CANNON_BALL))
		{
			if ( pCannon->m_itCannon.m_Load & 2 )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_CANNON_HSHOT );
				return( false );
			}
			pCannon->m_itCannon.m_Load |= 2;
			SysMessageDefault( DEFMSG_ITEMUSE_CANNON_LSHOT );
			return( true );
		}
	}

	SysMessageDefault( DEFMSG_ITEMUSE_CANNON_EMPTY );
	return( false );
}

bool CChar::Use_Train_Dummy( CItem * pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_Dummy");
	// IT_TRAIN_DUMMY
	// Dummy animation timer prevents over dclicking.

	ASSERT(pItem);
	SKILL_TYPE skill = Fight_GetWeaponSkill();
	char skilltag[32];
	int skillcheck = 0;
	sprintf( skilltag, "OVERRIDE.PracticeMax.SKILL_%d", skill &~ 0xD2000000 );

	//SysMessagef("SKILLTAG=%s\n",skilltag);

	if ( g_Cfg.IsSkillRanged(skill) ) // We do not allow archerytraining on dummys.
	{
		SysMessageDefault( DEFMSG_ITEMUSE_TDUMMY_ARCH );
		return( false );
	}

	CVarDefCont * pSkillTag = pItem->GetKey(skilltag, true);
	if ( pSkillTag )
	{
		//SysMessagef("skillcheck from TAG.%s=%d",skilltag, pSkillTag->GetValNum() );
		skillcheck = static_cast<int>(pSkillTag->GetValNum());
	} else {
		skillcheck = g_Cfg.m_iSkillPracticeMax;
		//SysMessagef("skillcheck from INI=%d", skillcheck );
	}

	if ( Skill_GetBase(skill) > skillcheck )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_TDUMMY_SKILL );
		return( false );
	}
	if ( ! pItem->IsTopLevel())
	{
baddumy:
		SysMessageDefault( DEFMSG_ITEMUSE_TDUMMY_P );
		return( false );
	}

	// Check location
	int dx = GetTopPoint().m_x - pItem->GetTopPoint().m_x;
	int dy = GetTopPoint().m_y - pItem->GetTopPoint().m_y;

	if ( pItem->GetDispID() == ITEMID_DUMMY1 )
	{
		if ( ! ( ! dx && abs(dy) < 2 ))
			goto baddumy;
	}
	else
	{
		if ( ! ( ! dy && abs(dx) < 2 ))
			goto baddumy;
	}

	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
		{
			return true;
		}
		UpdateAnimate(ANIM_ATTACK_WEAPON);
		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pItem->GetUID();
		Skill_Start( NPCACT_TRAINING );
	}
	else
	{
		pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetID() + 1), 3 * TICK_PER_SEC);
		pItem->Sound(0x033);
		Skill_UseQuick(skill, Calc_GetRandLLVal(40));
	}
	return( true );
}

bool CChar::Use_Train_PickPocketDip( CItem *pItem, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_PickPocketDip");
	// IT_TRAIN_PICKPOCKET
	// Train dummy.

	ASSERT(pItem);
	if ( Skill_GetBase(SKILL_STEALING) > g_Cfg.m_iSkillPracticeMax )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_PDUMMY_SKILL );
		return( true );
	}
	if ( !pItem->IsTopLevel())
	{
badpickpocket:
		SysMessageDefault( DEFMSG_ITEMUSE_PDUMMY_P );
		return( true );
	}

	int dx = GetTopPoint().m_x - pItem->GetTopPoint().m_x;
	int dy = GetTopPoint().m_y - pItem->GetTopPoint().m_y;

	bool fNS = ( pItem->GetDispID() == ITEMID_PICKPOCKET_NS ||
		pItem->GetDispID() == ITEMID_PICKPOCKET_NS2 );
	if ( fNS )
	{
		if ( ! ( ! dx && abs(dy) < 2 ))
			goto badpickpocket;
	}
	else
	{
		if ( ! ( ! dy && abs(dx) < 2 ))
			goto badpickpocket;
	}

	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
		{
			return true;
		}
		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pItem->GetUID();
		Skill_Start( NPCACT_TRAINING );
	}
	else
	{
		if ( ! Skill_UseQuick( SKILL_STEALING, Calc_GetRandLLVal(40)))
		{
			pItem->Sound( 0x041 );
			pItem->SetAnim( fNS ? ITEMID_PICKPOCKET_NS_FX : ITEMID_PICKPOCKET_EW_FX, 3*TICK_PER_SEC );
			UpdateAnimate(ANIM_ATTACK_WEAPON);
		}
		else
		{
			SysMessageDefault( DEFMSG_ITEMUSE_PDUMMY_OK );
		// pItem->Sound( 0x033 );
		}
	}

	return( true );
}

bool CChar::Use_Train_ArcheryButte( CItem * pButte, bool fSetup )
{
	ADDTOCALLSTACK("CChar::Use_Train_ArcheryButte");
	// IT_ARCHERY_BUTTE
	// If we are standing right next to the butte,
	// retrieve the arrows and bolts
	ASSERT(pButte);

	ITEMID_TYPE AmmoID;

	if ( GetDist( pButte ) < 2 )
	{
		if ( pButte->m_itArcheryButte.m_AmmoCount == 0 )
		{
			SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_EMPTY );
			return ( true );
		}

		AmmoID = pButte->m_itArcheryButte.m_AmmoType;
		const CItemBase * pAmmoDef = CItemBase::FindItemBase(AmmoID);
		if ( pAmmoDef != NULL )
		{
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_REM ),
				static_cast<LPCTSTR>(pAmmoDef->GetName()),
				(pButte->m_itArcheryButte.m_AmmoCount == 1 ) ? "" : g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_REMS ) );
			Emote(pszMsg);

			CItem *pRemovedAmmo =  CItem::CreateBase( AmmoID );
			ASSERT(pRemovedAmmo);
			pRemovedAmmo->SetAmount( pButte->m_itArcheryButte.m_AmmoCount );
			ItemBounce( pRemovedAmmo );
		}

		// Clear the target
		pButte->m_itArcheryButte.m_AmmoType = ITEMID_NOTHING;
		pButte->m_itArcheryButte.m_AmmoCount = 0;
		return ( true );
	}

	// We are not standing right next to the target...

	// We have to be using a ranged skill (aka archery or throwing) on this
	SKILL_TYPE skill = Fight_GetWeaponSkill();
	if ( !g_Cfg.IsSkillRanged(skill) )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_WS );
		return ( true );
	}
	if ( Skill_GetBase(skill) > g_Cfg.m_iSkillPracticeMax )
	{
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_SKILL );
		return ( true );
	}

	// Make sure we have some ammo

	CItem * pWeapon = m_uidWeapon.ItemFind();
	ASSERT(pWeapon);
	const CItemBase * pWeaponDef = pWeapon->Item_GetDef();

	// determine ammo type
	CVarDefCont * pVarAmmoType = pWeapon->GetDefKey("AMMOTYPE", true);
	RESOURCE_ID_BASE rid;
	LPCTSTR t_Str;

	if ( pVarAmmoType )
	{
		t_Str = pVarAmmoType->GetValStr();
		rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str));
	}
	else
		rid = pWeaponDef->m_ttWeaponBow.m_idAmmo;

	AmmoID = static_cast<ITEMID_TYPE>(rid.GetResIndex());

	// If there is a different ammo type on the butte currently,
	// tell us to remove the current type first.
	if ( (pButte->m_itArcheryButte.m_AmmoType != ITEMID_NOTHING) &&
		(pButte->m_itArcheryButte.m_AmmoType != AmmoID))
	{
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_X );
		return ( true );
	}

	// We need to be correctly aligned with the target before we can use it
	// For the south facing butte, we need to have the same x value and a y value greater than 2
	// For the east facing butte, we need to have the same y value and an x value greater than 2
	if ( ! pButte->IsTopLevel())
	{
badalign:
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_P );
		return ( true );
	}

	int targDistX = GetTopPoint().m_x - pButte->GetTopPoint().m_x;
	int targDistY = GetTopPoint().m_y - pButte->GetTopPoint().m_y;

	if ((pButte->GetID() == ITEMID_ARCHERYBUTTE_S) ||
		(pButte->GetID() == ITEMID_MONGBATTARGET_S))
	{
		if ( ! ( targDistX == 0 && targDistY > 2 ))
			goto badalign;
	}
	else
	{
		if ( ! ( targDistY == 0 && targDistX > 2 ))
			goto badalign;
	}

	// Is anything in the way?
	if ( !CanSeeLOS(pButte, LOS_NB_WINDOWS)) //we should be able to shoot through a window
	{
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_BLOCK );
		return ( true );
	}

	if ( fSetup )
	{
		if ( Skill_GetActive() == NPCACT_TRAINING )
		{
			return true;
		}
		UpdateAnimate(ANIM_ATTACK_WEAPON);
		m_Act_TargPrv = m_uidWeapon;
		m_Act_Targ = pButte->GetUID();
		Skill_Start( NPCACT_TRAINING );
		return true;
	}

	if ( m_pPlayer &&
		AmmoID &&
		ContentConsume( RESOURCE_ID( RES_ITEMDEF, AmmoID )))
	{
		SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_NOAMMO );
		return( true );
	}

	CVarDefCont * pCont = pWeapon->GetDefKey("AMMOCONT",true);

	if (m_pPlayer && AmmoID )
		{
			int iFound = 0;
			if ( pCont )
			{
				//check for UID
				CGrayUID uidCont = static_cast<DWORD>(pCont->GetValNum());
				CItemContainer *pNewCont = dynamic_cast <CItemContainer*> (uidCont.ItemFind());
				if (!pNewCont) //if no UID, check for ITEMID_TYPE
				{
					t_Str = pCont->GetValStr();
					RESOURCE_ID_BASE rContid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
					ITEMID_TYPE ContID = static_cast<ITEMID_TYPE>(rContid.GetResIndex());
					if (ContID)
						pNewCont = dynamic_cast <CItemContainer*> (ContentFind( rContid ));
				}

				if (pNewCont)
					iFound = pNewCont->ContentConsume( RESOURCE_ID( RES_ITEMDEF, AmmoID ));
				else
					iFound = ContentConsume( RESOURCE_ID( RES_ITEMDEF, AmmoID ));
			}
			else
				iFound = ContentConsume( RESOURCE_ID( RES_ITEMDEF, AmmoID ));
			if ( !iFound )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_ARCHB_NOAMMO );
				return( true );
			}
		}

	// OK...go ahead and fire at the target
	// Check the skill
	bool fSuccess = Skill_UseQuick( skill, Calc_GetRandLLVal(40));

	// determine animation parameters
	CVarDefCont * pVarAnim = pWeapon->GetDefKey("AMMOANIM", true);
	CVarDefCont * pVarAnimColor = pWeapon->GetDefKey("AMMOANIMHUE", true);
	CVarDefCont * pVarAnimRender = pWeapon->GetDefKey("AMMOANIMRENDER", true);
	ITEMID_TYPE AmmoAnim;
	DWORD AmmoHue;
	DWORD AmmoRender;

	if ( pVarAnim )
	{
		t_Str = pVarAnim->GetValStr();
		rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
		AmmoAnim = static_cast<ITEMID_TYPE>(rid.GetResIndex());
	}
	else
	{
		AmmoAnim = static_cast<ITEMID_TYPE>(pWeaponDef->m_ttWeaponBow.m_idAmmoX.GetResIndex());
	}

	if ( pVarAnimColor )
		AmmoHue = static_cast<unsigned long>(pVarAnimColor->GetValNum());
	else
		AmmoHue = 0;

	if ( pVarAnimRender )
		AmmoRender = static_cast<unsigned long>(pVarAnimRender->GetValNum());
	else
		AmmoRender = 0;
	
	// show firing effect
	pButte->Effect( EFFECT_BOLT, AmmoAnim, this, 16, 0, false, AmmoHue, AmmoRender );
	pButte->Sound( 0x224 );

static LPCTSTR const sm_Txt_ArcheryButte_Failure[] =
{
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_MISS_1 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_MISS_2 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_MISS_3 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_MISS_4 )
};

static LPCTSTR const sm_Txt_ArcheryButte_Success[] =
{
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_HIT_1 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_HIT_2 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_HIT_3 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_HIT_4 )
};

	// Did we destroy the ammo?
	const CItemBase * pAmmoDef = NULL;
	if ( AmmoID )
		pAmmoDef = CItemBase::FindItemBase(AmmoID);

	if (!fSuccess)
	{
		// Small chance of destroying the ammo
		if ( pAmmoDef != NULL && !Calc_GetRandVal(10))
		{
			TCHAR * pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_DEST ), static_cast<LPCTSTR>(pAmmoDef->GetName()));
			Emote(pszMsg, NULL, true );
			return true;
		}

		Emote( sm_Txt_ArcheryButte_Failure[ Calc_GetRandVal(COUNTOF(sm_Txt_ArcheryButte_Failure)) ]);
	}
	else
	{
		// Very small chance of destroying another arrow
		if ( pAmmoDef != NULL && !Calc_GetRandVal(50) && pButte->m_itArcheryButte.m_AmmoCount )
		{
			TCHAR * pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_ARCHB_SPLIT ), static_cast<LPCTSTR>(pAmmoDef->GetName()));
			Emote(pszMsg, NULL, true);
			return true;
		}

		Emote( sm_Txt_ArcheryButte_Success[ Calc_GetRandVal(COUNTOF(sm_Txt_ArcheryButte_Success)) ]);
	}

	// Update the target
	if ( AmmoID )
	{
		pButte->m_itArcheryButte.m_AmmoType = AmmoID;
		pButte->m_itArcheryButte.m_AmmoCount++;
		// if (pButte->GetID() == ITEMID_MONGBATTARGET_S)
		// {
		//     animate the target getting hit
		// }
	}
	return( true );
}

bool CChar::Use_Item_Web( CItem * pItemWeb )
{
	ADDTOCALLSTACK("CChar::Use_Item_Web");
	// IT_WEB
	// IT_EQ_STUCK
	// Try to break out of the web.
	// Or just try to damage it.

	// RETURN: true = held in place.
	//  false = walk thru it.
	ASSERT( pItemWeb );

	if ( GetDispID() == CREID_GIANT_SPIDER ||
		IsPriv(PRIV_GM) ||
		! pItemWeb->IsTopLevel() ||
		IsStatFlag(STATF_DEAD|STATF_Insubstantial))
	{
		return( false );	// just walks thru it.
	}

	// Try to break it.
	int iStr = pItemWeb->m_itWeb.m_Hits_Cur;
	if ( iStr == 0 )
	{
		iStr = pItemWeb->m_itWeb.m_Hits_Cur = 60 + Calc_GetRandVal( 250 );
	}

	CItem * pFlag = LayerFind( LAYER_FLAG_Stuck );

	// Since broken webs become spider silk, we should get out of here now if we aren't in a web.
	if ( CanMove( pItemWeb, false ))
	{
		if (pFlag)
			pFlag->Delete();
		return( false );
	}

	if ( pFlag )
	{
		// don't allow me to try to damage it too often.
		if ( pFlag->IsTimerSet())
			return( true );
	}

	int iDmg = pItemWeb->OnTakeDamage( Stat_GetAdjusted(STAT_STR), this );

	switch (iDmg)
	{
		case 0:			// damage blocked
		case 1:			// web survived
		default:		// unknown
			// is character still stuck on the web?
			if ( GetTopPoint() == pItemWeb->GetTopPoint() )
				break;

		case 2:			// web turned into silk
		case INT_MAX:	// web destroyed
			if (pFlag)
				pFlag->Delete();

			return( false );
	}

	// Stuck in it still.
	if ( pFlag == NULL )
	{
		if ( iDmg < 0 )
			return( false );

		// First time message.
		pFlag = CItem::CreateBase( ITEMID_WEB1_1 );
		ASSERT(pFlag);
		pFlag->SetType(IT_EQ_STUCK);
		pFlag->m_uidLink = pItemWeb->GetUID();
		LayerAdd( pFlag, LAYER_FLAG_Stuck );
	}
	else
	{
		if ( iDmg < 0 )
		{
			pFlag->Delete();
			return( false );
		}
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SWEB_STUCK ), static_cast<LPCTSTR>(pItemWeb->GetName()));
	}

	pFlag->SetTimeout( TICK_PER_SEC );	// Don't check it too often.
	return( true );
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
	//

	if ( pInstrument == NULL )
	{
		pInstrument = ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_MUSICAL), 0, 1 );
		if ( pInstrument == NULL )
		{
			SysMessageDefault( DEFMSG_MUSICANSHIP_NOTOOL );
			return( -2 );
		}
	}

	bool fSuccess = Skill_UseQuick( SKILL_MUSICIANSHIP, iDifficultyToPlay, ( Skill_GetActive() != SKILL_MUSICIANSHIP ) );
	Sound( pInstrument->Use_Music( fSuccess ));
	if ( ! fSuccess )
	{
		// Skill gain for SKILL_MUSICIANSHIP failure will need to be triggered
		// manually, since Skill_UseQuick isn't going to do it for us in this case
		if ( Skill_GetActive() == SKILL_MUSICIANSHIP )
			Skill_Experience( SKILL_MUSICIANSHIP, -iDifficultyToPlay );

		SysMessageDefault( DEFMSG_MUSICANSHIP_POOR );
		return( -1 );	// Impossible.
	}
	return( iDifficultyToPlay );	// success
}

bool CChar::Use_Repair( CItem * pItemArmor )
{
	ADDTOCALLSTACK("CChar::Use_Repair");
	// Attempt to repair the item.
	// If it is repairable.

	if ( pItemArmor == NULL || ! pItemArmor->Armor_IsRepairable())
	{
		SysMessageDefault( DEFMSG_REPAIR_NOT );
		return( false );
	}

	if ( pItemArmor->IsItemEquipped())
	{
		SysMessageDefault( DEFMSG_REPAIR_WORN );
		return( false );
	}
	if ( ! CanUse( pItemArmor, true ))
	{
		SysMessageDefault( DEFMSG_REPAIR_REACH );
		return( false );
	}

	// quickly use arms lore skill, but don't gain any skill until later on
	int iArmsLoreDiff = Calc_GetRandVal(30);
	if ( ! Skill_UseQuick( SKILL_ARMSLORE, iArmsLoreDiff, false ) )
	{
		// apply arms lore skillgain for failure
		Skill_Experience( SKILL_ARMSLORE, -iArmsLoreDiff );
		SysMessageDefault( DEFMSG_REPAIR_UNK );
		return( false );
	}

	if ( pItemArmor->m_itArmor.m_Hits_Cur >= pItemArmor->m_itArmor.m_Hits_Max )
	{
		SysMessageDefault( DEFMSG_REPAIR_FULL );
		return( false );
	}

	m_Act_p = g_World.FindItemTypeNearby( GetTopPoint(), IT_ANVIL, 3, false );
	if ( ! m_Act_p.IsValidPoint())
	{
		SysMessageDefault( DEFMSG_REPAIR_ANVIL );
		return( false );
	}

	CItemBase * pItemDef = pItemArmor->Item_GetDef();
	ASSERT(pItemDef);

	// Use up some raw materials to repair.
	int iTotalHits = pItemArmor->m_itArmor.m_Hits_Max;
	int iDamageHits = pItemArmor->m_itArmor.m_Hits_Max - pItemArmor->m_itArmor.m_Hits_Cur;
	int iDamagePercent = IMULDIV( 100, iDamageHits, iTotalHits );

	size_t iMissing = ResourceConsumePart( &(pItemDef->m_BaseResources), 1, iDamagePercent/2, true );
	if ( iMissing != pItemDef->m_BaseResources.BadIndex() )
	{
		// Need this to repair.
		const CResourceDef * pCompDef = g_Cfg.ResourceGetDef( pItemDef->m_BaseResources.GetAt(iMissing).GetResourceID() );
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_LACK_1 ), pCompDef ? static_cast<LPCTSTR>(pCompDef->GetName()) : g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_LACK_2 ) );
		return( false );
	}

	UpdateDir( m_Act_p );
	UpdateAnimate(ANIM_ATTACK_1H_WIDE);

	// quarter the skill to make it.
	// + more damaged items should be harder to repair.
	// higher the percentage damage the closer to the skills to make it.
	//

	size_t iRes = pItemDef->m_SkillMake.FindResourceType( RES_SKILL );
	if ( iRes == pItemDef->m_SkillMake.BadIndex() ) // no skill ?
	{
		return( false );
	}

	CResourceQty RetMainSkill = pItemDef->m_SkillMake[iRes];

	int iSkillLevel = static_cast<int>(RetMainSkill.GetResQty()) / 10;
	int iDifficulty = IMULDIV( iSkillLevel, iDamagePercent, 100 );
	if ( iDifficulty < iSkillLevel/4 )
	{
		iDifficulty = iSkillLevel/4;
	}

	LPCTSTR pszText;

	// apply arms lore skillgain now
	Skill_Experience( SKILL_ARMSLORE, iArmsLoreDiff );
	bool fSuccess = Skill_UseQuick(static_cast<SKILL_TYPE>(RetMainSkill.GetResIndex()), iDifficulty );
	if ( fSuccess )
	{
		pItemArmor->m_itArmor.m_Hits_Cur = iTotalHits;
		pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_1 );
	}
	else
	{
		/*****************************
		// not sure if this is working!
		******************************/
		// Failure
		if ( ! Calc_GetRandVal(6))
		{
			pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_2 );
			pItemArmor->m_itArmor.m_Hits_Max --;
			// pItemArmor->OnTakeDamage()
			pItemArmor->m_itArmor.m_Hits_Cur --;
		}
		else if ( ! Calc_GetRandVal(3))
		{
			pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_3 );
			// pItemArmor->OnTakeDamage()
			pItemArmor->m_itArmor.m_Hits_Cur --;
		}
		else
		{
			pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_4 );
		}

		// Some random amount.
		iDamagePercent = Calc_GetRandVal(iDamagePercent);
	}

	ResourceConsumePart( &(pItemDef->m_BaseResources), 1, iDamagePercent/2, false );

	if ( pItemArmor->m_itArmor.m_Hits_Cur <= 0 )
	{
		pszText = g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_5 );
	}

	TCHAR * pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_REPAIR_MSG ), pszText, static_cast<LPCTSTR>(pItemArmor->GetName()));
	Emote(pszMsg);

	if ( pItemArmor->m_itArmor.m_Hits_Cur <= 0 )
	{
		pItemArmor->Delete();
	}
	else
	{
		pItemArmor->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_DURABILITY);
	}

	return( fSuccess );
}

void CChar::Use_EatQty( CItem * pFood, int iQty )
{
	ADDTOCALLSTACK("CChar::Use_EatQty");
	// low level eat
	ASSERT(pFood);
	if ( iQty <= 0 )
		return;

	if ( iQty > pFood->GetAmount())
		iQty = pFood->GetAmount();

	int iValue;
	if ( pFood->m_itFood.m_foodval != 0)
	{
		iValue = static_cast<int>(pFood->m_itFood.m_foodval);
		//DEBUG_WARN(("retrieved food value %s from MOREM = %d\n", pFood->GetResourceName(), iValue )); 
	}
	else
	{
        iValue = pFood->Item_GetDef()->GetVolume(); // some food should have more value than other !
	}

	if ( ! iValue )
		iValue = 1;

	int iSpace = Stat_GetMax(STAT_FOOD) - Stat_GetVal(STAT_FOOD);
	if ( iSpace <= 0 )	// no room
		return;
	int iValueTotal = iValue * iQty;
	if ( iValueTotal > iSpace && iQty > 1 )
	{
		iQty = iSpace / iValue;
		if ( iQty <= 0 )
			iQty = 1;
	}

	// Was the food poisoned ?
	// Did it have special (magical?) properties ?
	UpdateDir( pFood );

	// Poisoned ?
	switch ( pFood->GetType())
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			if ( pFood->m_itFood.m_poison_skill )
			{
				SetPoison( pFood->m_itFood.m_poison_skill*10, 1+(pFood->m_itFood.m_poison_skill/50), this );
			}
			break;

		default:
			break;
	}

	pFood->ConsumeAmount( iQty );

	EatAnim( pFood->GetName(), iValue*iQty );
}

bool CChar::Use_Eat( CItem * pItemFood, int iQty )
{
	ADDTOCALLSTACK("CChar::Use_Eat");
	// What we can eat should depend on body type.
	// How much we can eat should depend on body size and current fullness.
	//
	// ??? monsters should be able to eat corpses / raw meat
	// IT_FOOD or IT_FOOD_RAW
	// NOTE: Some foods like apples are stackable !

	if ( ! CanMove( pItemFood ))
	{
		SysMessageDefault( DEFMSG_FOOD_CANTMOVE );
		return false;
	}

	if ( Stat_GetMax(STAT_FOOD) == 0 )
	{
		SysMessageDefault( DEFMSG_FOOD_CANTEAT );
		return false;
	}

	// Is this edible by me ?
	if ( ! Food_CanEat( pItemFood ))
	{
		SysMessageDefault( DEFMSG_FOOD_RCANTEAT );
		return( false );
	}

	if ( Stat_GetVal(STAT_FOOD) >= Stat_GetMax(STAT_FOOD) )
	{
		SysMessageDefault( DEFMSG_FOOD_CANTEATF );
		return false;
	}

	Use_EatQty( pItemFood, iQty );

	LPCTSTR pMsg;
	int index = IMULDIV( Stat_GetVal(STAT_FOOD), 6, Stat_GetMax(STAT_FOOD) );

	switch ( index )
	{
	case 0:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_1 );
		break;
	case 1:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_2 );
		break;
	case 2:
	case 3:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_3 );
		break;
	case 4:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_4 );
		break;
	case 5:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_5 );
		break;
	case 6:
	default:
		pMsg = g_Cfg.GetDefaultMsg( DEFMSG_FOOD_FULL_6 );
		break;
	}
	SysMessage(pMsg);
	return( true );
}

void CChar::Use_Drink( CItem * pItem )
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
		return;
	}

	static const SOUND_TYPE sm_DrinkSounds[] = { 0x030, 0x031 };

	const CItemBase * pItemDef = pItem->Item_GetDef();
	ITEMID_TYPE idbottle = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttDrink.m_idEmpty));

	if ( pItem->IsType(IT_BOOZE))
	{
		// Beer wine and liquor. vary strength of effect. m_itBooze.m_EffectStr
		int iStrength = Calc_GetRandVal(300) + 10;

		// Create ITEMID_PITCHER if drink a pitcher.
		// GLASS or MUG or Bottle ?

		// Get you Drunk, but check to see if we already are drunk
		CItem *pDrunkLayer = LayerFind( LAYER_FLAG_Drunk );
		if ( pDrunkLayer == NULL )
		{
			CItem * pSpell = Spell_Effect_Create( SPELL_Liquor, LAYER_FLAG_Drunk, iStrength, 15*TICK_PER_SEC, this );
			pSpell->m_itSpell.m_spellcharges = 10;	// how long to last.
		}
		else
		{
			// lengthen/strengthen the effect
			Spell_Effect_Remove(pDrunkLayer);
			pDrunkLayer->m_itSpell.m_spellcharges += 10;
			if ( pDrunkLayer->m_itSpell.m_spelllevel < 500 )
			{
				pDrunkLayer->m_itSpell.m_spelllevel += iStrength;
			}
			else
			{
				// dead drunk ?
			}
			Spell_Effect_Add( pDrunkLayer );
		}
	}

	if ( pItem->IsType(IT_POTION))
	{
		// Time limit on using potions.
		if ( LayerFind( LAYER_FLAG_PotionUsed ))
		{
			SysMessageDefault(DEFMSG_DRINK_POTION_DELAY);
			return;
		}

		// Convey the effect of the potion.
		int iSkillQuality = pItem->m_itPotion.m_skillquality;

		OnSpellEffect(static_cast<SPELL_TYPE>(RES_GET_INDEX(pItem->m_itPotion.m_Type)), this, iSkillQuality, pItem );

		// Give me the marker that i've used a potion.
		Spell_Effect_Create( SPELL_NONE, LAYER_FLAG_PotionUsed, iSkillQuality, 15*TICK_PER_SEC, this );
	}

	if ( pItem->IsType(IT_DRINK) && IsSetOF(OF_DrinkIsFood))
	{
		if ( pItem->m_itFood.m_poison_skill )
		{
			SetPoison( pItem->m_itFood.m_poison_skill*10, 1+(pItem->m_itFood.m_poison_skill/50), this );
		}

		if ( Stat_GetVal(STAT_FOOD) < Stat_GetMax(STAT_FOOD) )
		{
			int fValue;
			if ( pItem->m_itDrink.m_foodval != 0)
			{
				fValue = static_cast<int>(pItem->m_itDrink.m_foodval);
				//DEBUG_WARN(("retrieved food value %s from MOREM = %d\n", pItem->GetResourceName(), fValue )); 
			}
			else
			{
				fValue = pItem->Item_GetDef()->GetVolume(); 
			}


			if ( ! fValue )
				fValue = 1;

			int iSpace = Stat_GetMax(STAT_FOOD) - Stat_GetVal(STAT_FOOD);
			if ( iSpace <= 0 )	// no room
			{
				SysMessageDefault(DEFMSG_DRINK_FULL);
				return;
			}
			Stat_SetVal(STAT_FOOD, (Stat_GetVal(STAT_FOOD) + fValue));
		}
	}

	Sound( sm_DrinkSounds[ Calc_GetRandVal( COUNTOF(sm_DrinkSounds)) ] );
	UpdateAnimate( ANIM_EAT );

	pItem->ConsumeAmount();

	// Create the empty bottle ?
	if ( idbottle != ITEMID_NOTHING )
	{
		ItemBounce( CItem::CreateScript( idbottle, this ));
	}
}

CChar * CChar::Use_Figurine( CItem * pItem, int iPaces )
{
	ADDTOCALLSTACK("CChar::Use_Figurine");
	// NOTE: The figurine is NOT destroyed.
	int iCreateNewNpc = 0;
	if ( pItem == NULL )
		return NULL;

	if ( pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsChar() && pItem->m_uidLink != GetUID() && ! IsPriv( PRIV_GM ))
	{
		SysMessageDefault( DEFMSG_FIGURINE_NOTYOURS );
		return( NULL );
	}

	// Create a new NPC if there's no one linked to this figurine 
	CChar * pPet = pItem->m_itFigurine.m_UID.CharFind();
	if ( pPet == NULL )
	{
		CREID_TYPE id = pItem->m_itFigurine.m_ID;
		if ( ! id )
		{
			id = CItemBase::FindCharTrack( pItem->GetID());
			if ( ! id )
			{
				DEBUG_ERR(( "FIGURINE id 0%x, no creature\n", pItem->GetDispID()));
				return NULL;
			}
		}
		iCreateNewNpc = 1;
		pPet = CreateNPC( id );
		ASSERT(pPet);
		pPet->SetName( pItem->GetName());
		if ( pItem->GetHue())
		{
			pPet->m_prev_Hue = pItem->GetHue();
			pPet->SetHue( pItem->GetHue());
		}
	}

	if ( ! iPaces )
	{
		pPet->m_dirFace = m_dirFace;	// getting off ridden horse.
	}
	else
	{
		if ( IsSetEF(EF_PetSlots) )
		{
			short int iFollowerSlotsNeeded = maximum(pPet->GetDefNum("FOLLOWERSLOTS", true),1);
			short int iCurFollower = GetDefNum("CURFOLLOWER", true);
			short int iMaxFollower = GetDefNum("MAXFOLLOWER", true);
			short int iSetFollower = iCurFollower + iFollowerSlotsNeeded;
			if ( iSetFollower > 255 )
				iSetFollower = 255;

			if ((iCurFollower + iFollowerSlotsNeeded) > iMaxFollower && !IsPriv(PRIV_GM) )
			{
				SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_PETSLOTS_TRY_CONTROL) );
				if ( iCreateNewNpc )
					pPet->Delete();
				return false;
			}

			// Send an update packet for the stats
			SetDefNum("CURFOLLOWER", iSetFollower);
			CClient * pClient = GetClient();
			if (pClient)
				pClient->addCharStatWindow( GetUID() );
		}
	}

	if ( pPet->IsDisconnected())
	{
		// Pull the creature out of idle space.
		pPet->StatFlag_Clear( STATF_Ridden );
	}

	pItem->m_itFigurine.m_UID.InitUID();
	pPet->SetUnkPoint( pItem->GetTopLevelObj()->GetTopPoint() );
	pPet->MoveToChar( pItem->GetTopLevelObj()->GetTopPoint() );
	pPet->NPC_PetSetOwner( this );
	pPet->Update();
	pPet->Skill_Start( SKILL_NONE );	// was NPCACT_RIDDEN

	if ( pItem->IsAttr( ATTR_MAGIC ))
	{
		pPet->UpdateAnimate( ANIM_CAST_DIR );
		pPet->SoundChar( CRESND_GETHIT );
	}
	else
	{
		pPet->SoundChar( CRESND_RAND1 );	// Horse winny
	}

	return pPet;
}

bool CChar::Use_Key( CItem * pKey, CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_Key");
	ASSERT(pKey);
	ASSERT(pKey->IsType(IT_KEY));
	if ( pItemTarg == NULL )
	{
		SysMessageDefault( DEFMSG_KEY_TARG );
		return false;
	}

	if ( pKey != pItemTarg && pItemTarg->IsType(IT_KEY) )
	{
		// We are trying to copy a key ?
		if ( ! CanUse( pItemTarg, true ))
		{
			SysMessageDefault( DEFMSG_KEY_TARG_REACH );
			return false;
		}

		if ( ! pKey->m_itKey.m_lockUID && ! pItemTarg->m_itKey.m_lockUID )
		{
			SysMessageDefault( DEFMSG_KEY_BLANKS );
			return false;
		}
		if ( pItemTarg->m_itKey.m_lockUID && pKey->m_itKey.m_lockUID )
		{
			SysMessageDefault( DEFMSG_KEY_NOTBLANKS );
			return false;
		}

		// Need tinkering tools ???

		if ( ! Skill_UseQuick( SKILL_TINKERING, 30+Calc_GetRandLLVal(40)))
		{
			SysMessageDefault( DEFMSG_KEY_FAILC );
			return( false );
		}
		if ( pItemTarg->m_itKey.m_lockUID )
		{
			pKey->m_itKey.m_lockUID = pItemTarg->m_itKey.m_lockUID;
		}
		else
		{
			pItemTarg->m_itKey.m_lockUID = pKey->m_itKey.m_lockUID;
		}
		return( true );
	}

	if ( ! pKey->m_itKey.m_lockUID )
	{
		SysMessageDefault( DEFMSG_KEY_ISBLANK );
		return false;
	}
	if ( pKey == pItemTarg )	// Rename the key.
	{
		// We may rename the key.
		if ( IsClient())
		{
			GetClient()->addPromptConsole( CLIMODE_PROMPT_NAME_KEY, g_Cfg.GetDefaultMsg( DEFMSG_KEY_SETNAME ), pKey->GetUID() );
		}
		return false;
	}

	if ( ! CanUse( pItemTarg, false ))
	{
		SysMessageDefault( DEFMSG_KEY_CANTREACH );
		return false;
	}

	if ( m_pArea->GetResourceID() == pKey->m_itKey.m_lockUID )
	{
		if ( Use_MultiLockDown( pItemTarg ))
		{
			return( true );
		}
	}

	if ( ! pItemTarg->m_itContainer.m_lockUID )	// or m_itContainer.m_lockUID
	{
		SysMessageDefault( DEFMSG_KEY_NOLOCK );
		return false;
	}
	if ( ! pKey->IsKeyLockFit( pItemTarg->m_itContainer.m_lockUID )) // or m_itKey
	{
		SysMessageDefault( DEFMSG_KEY_WRONGLOCK );
		return false;
	}

	return( Use_KeyChange( pItemTarg ));
}

bool CChar::Use_KeyChange( CItem * pItemTarg )
{
	ADDTOCALLSTACK("CChar::Use_KeyChange");
	// Lock or unlock the item.
	switch ( pItemTarg->GetType() )
	{
	case IT_SIGN_GUMP:
		// We may rename the sign.
		if ( IsClient())
		{
			GetClient()->addPromptConsole( CLIMODE_PROMPT_NAME_SIGN, g_Cfg.GetDefaultMsg( DEFMSG_KEY_TARG_SIGN ), pItemTarg->GetUID() );
		}
		return true;
	case IT_CONTAINER:
		pItemTarg->SetType(IT_CONTAINER_LOCKED);
		SysMessageDefault( DEFMSG_KEY_TARG_CONT_LOCK );
		break;
	case IT_CONTAINER_LOCKED:
		pItemTarg->SetType(IT_CONTAINER);
		SysMessageDefault( DEFMSG_KEY_TARG_CONT_ULOCK );
		break;
	case IT_SHIP_HOLD:
		pItemTarg->SetType(IT_SHIP_HOLD_LOCK);
		SysMessageDefault( DEFMSG_KEY_TARG_HOLD_LOCK );
		break;
	case IT_SHIP_HOLD_LOCK:
		pItemTarg->SetType(IT_SHIP_HOLD);
		SysMessageDefault( DEFMSG_KEY_TARG_HOLD_ULOCK );
		break;
	case IT_DOOR_OPEN:
	case IT_DOOR:
		pItemTarg->SetType(IT_DOOR_LOCKED);
		SysMessageDefault( DEFMSG_KEY_TARG_DOOR_LOCK );
		break;
	case IT_DOOR_LOCKED:
		pItemTarg->SetType(IT_DOOR);
		SysMessageDefault( DEFMSG_KEY_TARG_DOOR_ULOCK );
		break;
	case IT_SHIP_TILLER:
		if ( IsClient())
		{
			GetClient()->addPromptConsole( CLIMODE_PROMPT_NAME_SHIP, g_Cfg.GetDefaultMsg( DEFMSG_SHIPNAME_PROMT ), pItemTarg->GetUID() );
		}
		return true;
	case IT_SHIP_PLANK:
		pItemTarg->Ship_Plank( false );	// just close it.
		if ( pItemTarg->GetType() == IT_SHIP_SIDE_LOCKED )
		{
			pItemTarg->SetType(IT_SHIP_SIDE);
			SysMessageDefault( DEFMSG_KEY_TARG_SHIP_ULOCK );
			break;
		}
		// Then fall thru and lock it.
	case IT_SHIP_SIDE:
		pItemTarg->SetType(IT_SHIP_SIDE_LOCKED);
		SysMessageDefault( DEFMSG_KEY_TARG_SHIP_LOCK );
		break;
	case IT_SHIP_SIDE_LOCKED:
		pItemTarg->SetType(IT_SHIP_SIDE);
		SysMessageDefault( DEFMSG_KEY_TARG_SHIP_ULOCK );
		break;
	default:
		SysMessageDefault( DEFMSG_KEY_TARG_NOLOCK );
		return false;
	}

	pItemTarg->Sound( 0x049 );
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
	{
		pt = *pPoint;
	}
	else
	{
		// NPC is using this .
		if ( ! pSeed->IsTopLevel())
			pt = GetTopPoint();
		else
			pt = pSeed->GetTopPoint();
	}

	if ( ! CanTouch(pt))
	{
		SysMessageDefault( DEFMSG_SEED_REACH );
		return( false );
	}

	// is there soil here ? IT_DIRT
	if ( ! IsPriv(PRIV_GM) && ! g_World.IsItemTypeNear( pt, IT_DIRT, 0, false ))
	{
		SysMessageDefault( DEFMSG_SEED_TARGSOIL );
		return( false );
	}

	const CItemBase * pItemDef = pSeed->Item_GetDef();
	ITEMID_TYPE idReset = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttFruit.m_idReset));
	if ( idReset == 0 )
	{
		SysMessageDefault( DEFMSG_SEED_NOGOOD );
		return( false );
	}

	// Already a plant here ?
	CWorldSearch AreaItems( pt );
	for (;;)
	{
		CItem * pItem = AreaItems.GetItem();
		if (pItem == NULL)
			break;
		if ( pItem->IsType(IT_TREE) ||
			pItem->IsType(IT_FOLIAGE))
		{
			// already a tree here .
			SysMessageDefault( DEFMSG_SEED_ATREE );
			return( false );
		}
		if ( pItem->IsType(IT_CROPS))
		{
			// already a plant here !
			pItem->Delete();
		}
	}

	// plant it and consume the seed.

	CItem * pPlant = CItem::CreateScript( idReset, this );
	ASSERT( pPlant );

	pPlant->MoveTo(pt);
	if ( pPlant->IsType(IT_CROPS) || pPlant->IsType(IT_FOLIAGE))
	{
		pPlant->m_itCrop.m_ReapFruitID = pSeed->GetID();
		pPlant->Plant_CropReset();
	}
	else
	{
		pPlant->SetDecayTime(10*g_Cfg.m_iDecay_Item );
	}

	pSeed->ConsumeAmount();
	return( true );
}

bool CChar::Use_BedRoll( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::Use_BedRoll");
	// IT_BEDROLL

	ASSERT(pItem);
	switch ( pItem->GetDispID())
	{
	case ITEMID_BEDROLL_C:
		if ( ! pItem->IsTopLevel())
		{
putonground:
			SysMessageDefault( DEFMSG_ITEMUSE_BEDROLL );
			return( true );
		}
		pItem->SetID( Calc_GetRandVal(2) ? ITEMID_BEDROLL_O_EW : ITEMID_BEDROLL_O_NS );
		pItem->Update();
		return true;
	case ITEMID_BEDROLL_C_NS:
		if ( ! pItem->IsTopLevel())
			goto putonground;
		pItem->SetID( ITEMID_BEDROLL_O_NS );
		pItem->Update();
		return true;
	case ITEMID_BEDROLL_C_EW:
		if ( ! pItem->IsTopLevel())
			goto putonground;
		pItem->SetID( ITEMID_BEDROLL_O_EW );
		pItem->Update();
		return true;
	case ITEMID_BEDROLL_O_EW:
	case ITEMID_BEDROLL_O_NS:
		pItem->SetID( ITEMID_BEDROLL_C );
		pItem->Update();
		return true;
	default:
		return false;
	}
}

bool CChar::Use_Item( CItem * pItem, bool fLink )
{
	ADDTOCALLSTACK("CChar::Use_Item");
	// An NPC could use these as well.
	// don't check distance here.
	// Could be a switch or something.
	// ARGS:
	//   fLink = this is the result of a linked action.
	// RETURN:
	//   true = it activated.

	if ( pItem == NULL )
		return( false );

	if ((( m_pNPC != NULL ) && ( m_pPlayer == NULL )) && (( IsTrigUsed(TRIGGER_DCLICK) ) || ( IsTrigUsed(TRIGGER_ITEMDCLICK) ))) //for players, dclick was called before this function
	{		
		if ( pItem->OnTrigger( ITRIG_DCLICK, this ) == TRIGRET_RET_TRUE )
			return false;
	}

	bool fAction = true;

	switch ( pItem->GetType() )
	{
	case IT_ITEM_STONE:
		// Give them this item
		if ( pItem->m_itItemStone.m_wAmount == USHRT_MAX )
		{
			// used to be sysmessagef
			SysMessageDefault( DEFMSG_IT_IS_DEAD );
			return true;
		}
		if ( pItem->m_itItemStone.m_wRegenTime )
		{
			if ( pItem->IsTimerSet())
			{
				SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_STONEREG_TIME ), pItem->GetTimerDiff() / TICK_PER_SEC );
				return true;
			}
			pItem->SetTimeout( pItem->m_itItemStone.m_wRegenTime * TICK_PER_SEC );
		}
		ItemBounce( CItem::CreateTemplate( pItem->m_itItemStone.m_ItemID, GetPackSafe(), this ));
		if ( pItem->m_itItemStone.m_wAmount != 0 )
		{
			pItem->m_itItemStone.m_wAmount--;
			if ( pItem->m_itItemStone.m_wAmount == 0 )
			{
				pItem->m_itItemStone.m_wAmount = USHRT_MAX;
			}
		}
		return true;

	case IT_SEED:
		return Use_Seed( pItem, NULL );

	case IT_BEDROLL:
		return Use_BedRoll(pItem);

	case IT_BELLOWS:
		// This is supposed to make the nearby fire pit hotter.
		pItem->Sound( 0x02b );
		pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetID() + 1), 2 * TICK_PER_SEC);
		return true;

	case IT_KINDLING:
		return Use_Kindling( pItem );

	case IT_SPINWHEEL:
		// Just make them spin.
		if ( !fLink )
		{
			pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetID() + 1), 2 * TICK_PER_SEC);
			SysMessageDefault( DEFMSG_ITEMUSE_SPINWHEEL );
			return true;
		}
		else
			return false;

	case IT_TRAIN_DUMMY:	// Train dummy.
		if ( !fLink )
		{
			Use_Train_Dummy(pItem, true );
			return true;
		}
		else
			return false;
	case IT_TRAIN_PICKPOCKET:
		if ( !fLink )
		{
			Use_Train_PickPocketDip(pItem, true);
			return true;
		}
		else
			return false;
	case IT_ARCHERY_BUTTE:	// Archery Butte
		if ( !fLink )
		{
			Use_Train_ArcheryButte(pItem, true);
			return true;
		}
		else
			return false;

	case IT_LOOM:
		//pItem->SetAnim(static_cast<ITEMID_TYPE>(pItem->GetID() + 1), 2 * TICK_PER_SEC);
		if ( !fLink )
		{
			SysMessageDefault( DEFMSG_ITEMUSE_LOOM );
			return true;
		}
		else
			return false;

	case IT_BEE_HIVE:
		// Get honey from it.
		if ( !fLink )
		{
			ITEMID_TYPE id = ITEMID_NOTHING;
			if ( ! pItem->m_itBeeHive.m_honeycount )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_BEEHIVE );
			}
			else switch ( Calc_GetRandVal(3))
			{
			case 1:
				id = ITEMID_JAR_HONEY;
				break;
			case 2:
				id = ITEMID_BEE_WAX;
				break;
			}
			if ( id )
			{
				ItemBounce( CItem::CreateScript( id, this ));
				pItem->m_itBeeHive.m_honeycount --;
			}
			else
			{
				// ouch got stung. Poison bee sting ?
				SysMessageDefault( DEFMSG_ITEMUSE_BEEHIVE_STING );
				OnTakeDamage( Calc_GetRandVal(5), this, DAMAGE_POISON|DAMAGE_GENERAL );
			}
			pItem->SetTimeout( 15*60*TICK_PER_SEC );
			return true;
		}
		else
			return false;

	case IT_MUSICAL:
		if ( ! Skill_Wait(SKILL_MUSICIANSHIP))
		{
			m_Act_Targ = pItem->GetUID();
			Skill_Start( SKILL_MUSICIANSHIP );
		}
		break;

	case IT_CROPS:
	case IT_FOLIAGE:
		// Pick cotton/hay/etc...
		fAction = pItem->Plant_Use(this);
		break;

	case IT_FIGURINE:
		// Create the creature here.
		fAction = ( Use_Figurine( pItem, 0 ) != NULL );
		if ( fAction )
		{
			pItem->Delete();
		}
		break;

	case IT_TRAP:
	case IT_TRAP_ACTIVE:
		// Explode or active the trap. (plus any linked traps.)
		if ( CanTouch( pItem->GetTopLevelObj()->GetTopPoint()))
		{
			OnTakeDamage( pItem->Use_Trap(), NULL, DAMAGE_HIT_BLUNT | DAMAGE_GENERAL );
		}
		else
		{
			pItem->Use_Trap();
		}
		break;

	case IT_SWITCH:
		// Switches can be linked to gates and doors and such.
		// Flip the switch graphic.
		pItem->SetSwitchState();
		break;

	case IT_PORT_LOCKED:	// Can only be trigered.
		if ( ! fLink && ! IsPriv(PRIV_GM))
		{
			SysMessageDefault( DEFMSG_ITEMUSE_PORT_LOCKED );
			return( true );
		}

	case IT_PORTCULIS:
		// Open a metal gate via a trigger of some sort.
		pItem->Use_Portculis();
		break;

	case IT_DOOR_LOCKED:
		if ( ! ContentFindKeyFor( pItem ))
		{
			SysMessageDefault( DEFMSG_KEY_DOORLOCKED );
			if ( ! pItem->IsTopLevel())
				return( false );
			if ( pItem->IsAttr(ATTR_MAGIC))
			{
				// Show it's magic face.
				ITEMID_TYPE id = ( GetDispID() & DOOR_NORTHSOUTH ) ? ITEMID_DOOR_MAGIC_SI_NS : ITEMID_DOOR_MAGIC_SI_EW;
				CItem * pFace = CItem::CreateBase( id );
				ASSERT(pFace);
				pFace->MoveToDecay( pItem->GetTopPoint(), 4*TICK_PER_SEC );
			}
			if ( ! IsPriv( PRIV_GM ))
				return true;
		}

	case IT_DOOR_OPEN:
	case IT_DOOR:
		{
			//bool fOpen = pItem->Use_Door( fLink );
			bool fOpen = pItem->Use_DoorNew( fLink );
			if ( fLink || !fOpen ) // Don't link if we are just closing the door.
				return true;
		}
		break;

	case IT_SHIP_PLANK:
		// If on the ship close it. if not teleport to it.
		if ( m_pArea->IsFlag( REGION_FLAG_SHIP ) &&
			m_pArea->GetResourceID() == pItem->m_uidLink )
		{
			// Close it. Check for a key if the plank used to be a locked ship side
			if ( pItem->m_itShipPlank.m_itSideType == IT_SHIP_SIDE_LOCKED && !ContentFindKeyFor( pItem ) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_SHIPSIDE );
				return true;
			}
			return( pItem->Ship_Plank( false ));
		}
		else if ( pItem->IsTopLevel())
		{
			// Move to it.
			CPointMap pntTarg = pItem->GetTopPoint();
			pntTarg.m_z ++;
			Spell_Teleport( pntTarg, true, false, ITEMID_NOTHING );
		}
		return true;

	case IT_SHIP_SIDE_LOCKED:
		if ( ! ContentFindKeyFor( pItem ))
		{
			SysMessageDefault( DEFMSG_ITEMUSE_SHIPSIDE );
			return true;
		}

	case IT_SHIP_SIDE:
		// Open it if we have the key or are already on the ship
		pItem->Ship_Plank( true );
		break;

	case IT_GRAIN:
	case IT_GRASS:
	case IT_GARBAGE:
	case IT_FRUIT:
	case IT_FOOD:
	case IT_FOOD_RAW:
	case IT_MEAT_RAW:
		if ( !fLink )
			Use_Eat(pItem);
		else
			return false;
		return true;

	case IT_POTION:
	case IT_DRINK:
	case IT_PITCHER:
	case IT_WATER_WASH:
	case IT_BOOZE:
		if ( !fLink )
			Use_Drink(pItem);
		else
			return false;
		return true;

	case IT_LIGHT_OUT: // Can the light be lit ?
	case IT_LIGHT_LIT: // Can the light be doused ?
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
		if ( !fLink )
			return ItemEquip( pItem );
		else
			return false;

	case IT_WEB:
		// try to get out of web.
		if ( !fLink )
		{
			Use_Item_Web( pItem );
			return true;
		}
		else
			return false;

	case IT_SPY_GLASS:
		if ( !fLink )
		{
			// Spyglass will now tell you the moon phases...
			static LPCTSTR const sm_sPhases[8] =
			{
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M1 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M2 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M3 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M4 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M5 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M6 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M7 ),
				g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_M8 )
			};
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_TR ), sm_sPhases[ g_World.GetMoonPhase(false) ]);
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SPYGLASS_FE ), sm_sPhases[ g_World.GetMoonPhase(true) ]);

			if ( m_pArea && m_pArea->IsFlag( REGION_FLAG_SHIP ))
			{
				ObjMessage( pItem->Use_SpyGlass(this), this );
			}
			return true;
		}
		return false;
	case IT_SEXTANT:
		if ( !fLink )
		{
			if (( GetTopPoint().m_map <= 1 ) && ( GetTopPoint().m_x > UO_SIZE_X_REAL ))	// in dungeons and t2a lands
			{
				ObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SEXTANT_T2A ), this );
			}
			else
			{
				TCHAR * pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_SEXTANT ), static_cast<LPCTSTR>(m_pArea->GetName()), static_cast<LPCTSTR>(pItem->Use_Sextant(GetTopPoint())));
				ObjMessage(pszMsg, this);
			}
			return true;
		}
		else return false;
	default:
		fAction = false;
	}

	// Try to follow the link as well. (if it has one)
	CItem * pLinkItem = pItem->m_uidLink.ItemFind();
	if ( pLinkItem != NULL && pLinkItem != pItem )
	{
		static CItem * sm_pItemFirst = NULL;	// watch out for loops.
		static int sm_iCount = 0;
		if ( ! fLink )
		{
			sm_pItemFirst = pItem;
			sm_iCount = 0;
		}
		else
		{
			if ( sm_pItemFirst == pItem )
				return true;	// kill the loop.
			if ( ++sm_iCount > 64 )
			{
				return( true );
			}
		}
		fAction |= Use_Item( pLinkItem, true );
	}

	return( fAction );
}

bool CChar::Use_Obj( CObjBase * pObj, bool fTestTouch, bool fScript  )
{
	ADDTOCALLSTACK("CChar::Use_Obj");
	if ( pObj == NULL )
		return( false );
	if ( IsClient())
	{
		return GetClient()->Event_DoubleClick( pObj->GetUID(), false, fTestTouch, fScript );
	}
	else
	{
		return Use_Item( dynamic_cast <CItem*>(pObj), fTestTouch );
	}
}

bool CChar::ItemEquipArmor( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipArmor");
	// equip ourselves as best as possible.

	CCharBase		*pCharDef = Char_GetDef();
	CItemContainer	*pPack = GetPack();

	if ( !pCharDef || !pPack || !pCharDef->Can(CAN_C_EQUIP) )
		return false;

	int iBestScore[LAYER_HORSE];
	memset(iBestScore, 0, sizeof(iBestScore));
	CItem	*pBestArmor[LAYER_HORSE];
	memset(pBestArmor, 0, sizeof(pBestArmor));

	if ( !fForce )
	{
		// Block those layers that are already used.
		for ( size_t i = 0; i < COUNTOF(iBestScore); i++ )
		{
			pBestArmor[i] = LayerFind(static_cast<LAYER_TYPE>(i));
			if ( pBestArmor[i] != NULL )
				iBestScore[i] = INT_MAX;
		}
	}

	CItem	*pItem = pPack->GetContentHead();
	for ( ; pItem != NULL; pItem = pItem->GetNext() )
	{
		int iScore = pItem->Armor_GetDefense();
		if ( !iScore )	// might not be armor.
			continue;

		// Can i even equip this ?
		LAYER_TYPE layer = CanEquipLayer(pItem, LAYER_QTY, NULL, true);
		if ( layer == LAYER_NONE )
			continue;

		if ( iScore > iBestScore[layer] )
		{
			iBestScore[layer] = iScore;
			pBestArmor[layer] = pItem;
		}
	}

	// now equip all the stuff we found.
	for ( size_t i = 0; i <COUNTOF(iBestScore); i++ )
	{
		if ( pBestArmor[i] == NULL )
			continue;
		ItemEquip(pBestArmor[i], this);
	}

	return true;
}

bool CChar::ItemEquipWeapon( bool fForce )
{
	ADDTOCALLSTACK("CChar::ItemEquipWeapon");
	// find my best weapon and equip it.
	if ( ! fForce && m_uidWeapon.IsValidUID())	// we are ok.
		return( true );

	CCharBase		*pCharDef = Char_GetDef();
	CItemContainer	*pPack = GetPack();

	if ( !pCharDef || !pPack || !pCharDef->Can(CAN_C_USEHANDS) )
		return( false );

	// Go through all my weapons and come up with a score for it's usefulness.

	CItem * pBestWeapon = NULL;
	int iWeaponScoreMax = NPC_GetWeaponUseScore( NULL );	// just wrestling.

	CItem* pItem = pPack->GetContentHead();
	for ( ; pItem!=NULL; pItem = pItem->GetNext())
	{
		int iWeaponScore = NPC_GetWeaponUseScore( pItem );
		if ( iWeaponScore > iWeaponScoreMax )
		{
			iWeaponScoreMax = iWeaponScore;
			pBestWeapon = pItem;
		}
	}

	if ( pBestWeapon )
	{
		return ItemEquip(pBestWeapon);
	}

	return( true );
}

