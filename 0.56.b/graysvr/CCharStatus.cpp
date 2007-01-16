//
// CChar.cpp
// Copyright Menace Software (www.menasoft.com).
//  CChar is either an NPC or a Player.
//

#include <math.h>
#include "graysvr.h"	// predef header.

bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	return IsResourceMatch( rid, dwAmount, 0 );
}


bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount, DWORD dwArgResearch )
{
	ADDTOCALLSTACK("CChar::IsResourceMatch");
	// Is the char a match for this test ?
	switch ( rid.GetResType())
	{
	case RES_SKILL:
		// Do i have this skill level at least ?
		// A min skill is required.
		if ( Skill_GetBase((SKILL_TYPE) rid.GetResIndex()) < dwAmount )
			return( false );
		return( true );

	case RES_CHARDEF:
		// Am i this type of char ?
		{
			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			if ( pCharDef->GetResourceID() == rid )
				return( true );
		}
		break;
	case RES_SPEECH:	// do i have this speech ?
		if ( m_pNPC != NULL )
		{
			if ( m_pNPC->m_Speech.FindResourceID( rid ) >= 0 )
				return( true );
			CCharBase * pCharDef = Char_GetDef();
			ASSERT(pCharDef);
			if ( pCharDef->m_Speech.FindResourceID( rid ) >= 0 )
				return( true );
		}
		break;
	case RES_EVENTS:	// do i have these events ?
		{
			if ( m_OEvents.FindResourceID( rid ) >= 0 )
				return( true );
			if ( m_pNPC != NULL )
			{
				CCharBase * pCharDef = Char_GetDef();
				ASSERT(pCharDef);
				if ( pCharDef->m_TEvents.FindResourceID( rid ) >= 0 )
					return( true );
			}
		}
		break;
	case RES_TYPEDEF:
		// Do i have these in my posession ?
		if ( ! ContentConsume( rid, dwAmount, true, dwArgResearch ) )
		{
			return( true );
		}
		return( false );
	case RES_ITEMDEF:
		// Do i have these in my posession ?
		if ( ! ContentConsume( rid, dwAmount, true ) )
		{
			return( true );
		}
		return( false );

	default:
		// No idea.
		return( false );
	}
	return( false );
}

CItemContainer * CChar::GetBank( LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::GetBank");
	// Get our bank box or vendor box.
	// If we dont have 1 then create it.

	ITEMID_TYPE id;
	switch ( layer )
	{
	case LAYER_PACK:
		id = ITEMID_BACKPACK;
		break;

	case LAYER_VENDOR_STOCK:
	case LAYER_VENDOR_EXTRA:
	case LAYER_VENDOR_BUYS:
		if ( !NPC_IsVendor() ) 
			return NULL;
 		id = ITEMID_VENDOR_BOX;
		break;

	case LAYER_BANKBOX:
	default:
		id = ITEMID_BANK_BOX;
		layer = LAYER_BANKBOX;
		break;
	}

	CItem * pItemTest = LayerFind( layer );
	CItemContainer * pBankBox = dynamic_cast <CItemContainer *>(pItemTest);
	if ( pBankBox == NULL && !g_Serv.IsLoading() )
	{
		if ( pItemTest )
		{
			DEBUG_ERR(( "Junk in bank box layer %d!\n", layer ));
			pItemTest->Delete();
		}

		// Give them a bank box if not already have one.
		pBankBox = dynamic_cast <CItemContainer *>( CItem::CreateScript(id, this));
		ASSERT(pBankBox);
		pBankBox->SetAttr(ATTR_NEWBIE | ATTR_MOVE_NEVER);
		LayerAdd( pBankBox, layer );
	}
	return( pBankBox );
}

CItem * CChar::LayerFind( LAYER_TYPE layer ) const
{
	ADDTOCALLSTACK("CChar::LayerFind");
	// Find an item i have equipped.
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItem->GetNext())
	{
		if ( pItem->GetEquipLayer() == layer )
			break;
	}
	return( pItem );
}

TRIGRET_TYPE CChar::OnCharTrigForLayerLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForLayerLoop");
	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	CItem * pItem=GetContentHead();
	CItem * pItemNext;

	for ( ; pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();

		if ( pItem->GetEquipLayer() == layer )
		{
			TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
			if ( iRet != TRIGRET_ENDIF )
			{
				return( iRet );
			}
			EndContext = s.GetContext();
			s.SeekContext( StartContext );
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
		{
			return( iRet );
		}
	}
	else
	{
		s.SeekContext( EndContext );
	}
	return( TRIGRET_ENDIF );
}

int CChar::GetWeightLoadPercent( int iWeight ) const
{
	ADDTOCALLSTACK("CChar::GetWeightLoadPercent");
	// Get a percent of load.
	if ( IsPriv(PRIV_GM))
		return( 1 );
	int	MaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if ( !MaxCarry ) return 1000;	// suppose self extra-overloaded
	return( IMULDIV( iWeight, 100, MaxCarry));
}

bool CChar::CanCarry( const CItem * pItem ) const
{
	ADDTOCALLSTACK("CChar::CanCarry");
	if ( IsPriv(PRIV_GM))
		return( true );

	int iMaxWeight = g_Cfg.Calc_MaxCarryWeight(this);

	// We are already carrying it ?
	const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	if ( this == pObjTop )
	{
		if ( GetTotalWeight() > iMaxWeight )
			return( false );
	}
	else
	{
		int iWeight = pItem->GetWeight();
		if ( GetTotalWeight() + iWeight > iMaxWeight )
			return( false );
	}

	return( true );
}

LAYER_TYPE CChar::CanEquipLayer( CItem * pItem, LAYER_TYPE layer, CChar * pCharMsg, bool fTest )
{
	ADDTOCALLSTACK("CChar::CanEquipLayer");
	// This takes care of any conflicting items in the slot !
	// NOTE: Do not check to see if i can pick this up or steal this etc.
	// LAYER_NONE = can't equip this .

	ASSERT(pItem);

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	const CItemBase * pItemDef = pItem->Item_GetDef();
	if ( layer >= LAYER_QTY )
	{
		layer = pItemDef->GetEquipLayer();
		if ( pItemDef->IsTypeEquippable() && CItemBase::IsVisibleLayer( layer ))
		{
			if ( ( m_pPlayer || fTest )	// test for PCs only or if requested
			&& pItemDef->m_ttEquippable.m_StrReq
			&& Stat_GetAdjusted(STAT_STR) < pItemDef->m_ttEquippable.m_StrReq )
			{
				if ( m_pPlayer )	// message only players
				{
					SysMessagef( "%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), (LPCTSTR) pItem->GetName());
					if ( pCharMsg != this )
					{
						pCharMsg->SysMessagef( "%s %s.", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_NOT_STRONG_ENOUGH), (LPCTSTR) pItem->GetName());
					}
				}
				return LAYER_NONE;	// can't equip stuff.
			}

			if ( pItem->GetParent() == this && pItem->GetEquipLayer() == layer )
			{
				return layer;
			}
		}
	}
	else
	{
		// Not a visible item LAYER_TYPE.
		if ( pItem->GetParent() == this && pItem->GetEquipLayer() == layer )
		{
			return layer;
		}
	}

	bool fCantEquip = false;

	CItem * pItemPrev = NULL;
	switch ( layer )
	{
	case LAYER_NONE:
	case LAYER_SPECIAL:
		switch ( pItem->GetType() )
		{
		case IT_EQ_TRADE_WINDOW:
		case IT_EQ_MEMORY_OBJ:
		case IT_EQ_SCRIPT:
			// We can have multiple items of these.
			return LAYER_SPECIAL;
		}
		return( LAYER_NONE );	// not legal !
	case LAYER_HAIR:
		if ( ! pItem->IsType(IT_HAIR))
		{
			fCantEquip = true;
			break;
		}
		break;
	case LAYER_BEARD:
		if ( ! pItem->IsType(IT_BEARD))
		{
			fCantEquip = true;
			break;
		}
		break;
	case LAYER_PACK:
		if ( ! pItem->IsType(IT_CONTAINER))
		{
			fCantEquip = true;
			break;
		}
		break;
	case LAYER_BANKBOX:
		if ( ! pItem->IsType(IT_EQ_BANK_BOX))
		{
			fCantEquip = true;
			break;
		}
		break;
	case LAYER_HORSE:
		// Only humans can ride horses !?
		if ( ! pItem->IsType(IT_EQ_HORSE) || ! IsHuman())
		{
			fCantEquip = true;
			break;
		}
		break;
	case LAYER_HAND1:
	case LAYER_HAND2:
		if ( ! pItemDef->IsTypeEquippable())
		{
			fCantEquip = true;
			break;
		}
		if ( ! pCharDef->Can( CAN_C_USEHANDS ))
		{
			fCantEquip = true;
			break;
		}
		if ( layer == LAYER_HAND2 )
		{
			// Is this a 2 handed weapon ? shields and lights are ok
			if ( pItem->IsTypeWeapon() || pItem->IsType(IT_FISH_POLE))
			{
				// Make sure the other hand is not full.
				pItemPrev = LayerFind( LAYER_HAND1 );
			}
		}
		else
		{
			// do i have a 2 handed weapon already equipped ? shields and lights are ok
			pItemPrev = LayerFind( LAYER_HAND2 );
			if ( pItemPrev != NULL )
			{
				if ( ! pItemPrev->IsTypeWeapon() && ! pItemPrev->IsType(IT_FISH_POLE))
				{
					pItemPrev = NULL;	// must be a shield or light.
				}
			}
		}
		break;


	case LAYER_NEWLIGHT:
		// DEBUG_ERR(( "ALERT: Weird layer %d used for '%s' check this\n", LAYER_HIDDEN, pItem->GetResourceName()));
	case LAYER_COLLAR:
	case LAYER_RING:
	case LAYER_EARS:	// anyone can use these !
	case LAYER_TALISMAN:
		break;

	default:
		// Can i equip this with my body type ?
		if ( CItemBase::IsVisibleLayer( layer ))
		{
			if ( !pCharDef->Can(CAN_C_EQUIP) )
			{
				if ( m_pNPC )	// auto-patch NPC adding CAN equip flags
					pCharDef->m_Can |= CAN_C_EQUIP;
				else
				{
					fCantEquip = true;

				}
			}
		}
		break;
	}

	if ( fCantEquip )// some creatures can equip certain special items ??? (orc lord?)
	{
		if ( pCharMsg)
			pCharMsg->SysMessagef("%s", g_Cfg.GetDefaultMsg(DEFMSG_EQUIP_CANNOT));
		return LAYER_NONE;	// can't equip stuff.
	}

	// Check for objects already in this slot. Deal with it.
	if ( pItemPrev == NULL )
	{
		pItemPrev = LayerFind( layer );
	}
	if ( pItemPrev != NULL )
	{
		switch ( layer )
		{
		case LAYER_PACK:
			// this should not happen.
			// Put it in my main pack.
			return LAYER_NONE;
		case LAYER_HORSE:
			// this should not happen.
			if ( ! fTest )
			{
				ItemBounce( pItemPrev );
			}
			break;
		case LAYER_DRAGGING:	// drop it.
			if ( ! fTest )
			{
				ItemBounce( pItemPrev );
			}
			break;
		case LAYER_BEARD:
		case LAYER_HAIR:
			if ( ! fTest )
			{
				pItemPrev->Delete();
			}
			break;
		default:
			if ( layer >= LAYER_SPELL_STATS )
			{
				// MAGICF_STACKSTATS flag allows multiple items equipped to
				// LAYER_SPELL_STATS, but not identical spells (?)
				if (IsSetMagicFlags( MAGICF_STACKSTATS ) && (layer == LAYER_SPELL_STATS))
				{
					for (pItemPrev = GetContentHead(); pItemPrev != NULL; pItemPrev = pItemPrev->GetNext())
					{
						// Don't interfere with other layers
						if (pItemPrev->GetEquipLayer() != layer)
							continue;

						// Don't interfere with other spells
						if (pItemPrev->m_itSpell.m_spell != pItem->m_itSpell.m_spell)
							continue;

						// Remove previous identical spells
						pItemPrev->Delete();
					}
					break;
				}

				// Magic spell slots just get bumped.
				pItemPrev->Delete();
				break;
			}
			// DEBUG_ERR(( "LayerAdd Layer %d already used\n", layer ));
			// Force the current layer item into the pack ?
			if ( ! CanMove( pItemPrev, true ))
			{
				return LAYER_NONE;
			}
			if ( ! fTest )
			{
				ItemBounce( pItemPrev );
			}
			break;
		}
	}

	return( layer );
}

bool CChar::CheckCorpseCrime( const CItemCorpse *pCorpse, bool fLooting, bool fTest )
{
	ADDTOCALLSTACK("CChar::CheckCorpseCrime");
	// fLooting = looting as apposed to carving.
	// RETURN: true = criminal act !

	if ( !pCorpse )
		return false;

	CChar * pCharGhost = pCorpse->m_uidLink.CharFind();
	if ( !pCharGhost )
		return false;
	if ( pCharGhost == this )	// ok but strange to carve up your own corpse.
		return false;
	if ( !g_Cfg.m_fLootingIsACrime )
		return false;

	// It's ok to loot a guild member !
	NOTO_TYPE noto = pCharGhost->Noto_GetFlag(this, false);
	switch (noto)
	{
	case NOTO_NEUTRAL:		// animals etc.
	case NOTO_GUILD_SAME:
	case NOTO_CRIMINAL:
	case NOTO_GUILD_WAR:
	case NOTO_EVIL:
		return false;	// not a crime.
	}

	if ( !fTest )
	{
		// Anyone see me do this ?
		CheckCrimeSeen(SKILL_NONE, pCharGhost, pCorpse, fLooting ? "looting" : NULL);
		Noto_Criminal();
	}

	return true; // got flagged
}

CItemCorpse *CChar::FindMyCorpse( int iRadius ) const
{
	ADDTOCALLSTACK("CChar::FindMyCorpse");
	// If they are standing on there own corpse then res the corpse !
	CWorldSearch Area(GetTopPoint(), iRadius);
	while ( true )
	{
		CItem *pItem = Area.GetItem();
		if ( !pItem )
			break;
		if ( !pItem->IsType(IT_CORPSE) )
			continue;
		CItemCorpse *pCorpse = dynamic_cast <CItemCorpse*> (pItem);
		if ( !pCorpse || ( pCorpse->m_uidLink != GetUID() ))
			continue;
		// not morphed type.
		if ( pCorpse->m_itCorpse.m_BaseID != m_prev_id )
			break;
		return pCorpse;
	}
	return NULL;
}

int CChar::GetHealthPercent() const
{
	ADDTOCALLSTACK("CChar::GetHealthPercent");
	int str = Stat_GetAdjusted(STAT_STR);
	if ( !str ) return 0;
	return IMULDIV(Stat_GetVal(STAT_STR), 100, str);
}

bool CChar::IsSwimming() const
{
	ADDTOCALLSTACK("CChar::IsSwimming");
	// Am i in/over/slightly under the water now ?
	// NOTE: This is a bit more complex because we need to test if we are slightly under water.

	CPointMap ptTop = GetTopPoint();

	CPointMap pt = g_World.FindItemTypeNearby( ptTop, IT_WATER );
	if ( ! pt.IsValidPoint())
		return( false );	// no water here.

	int iDistZ = ptTop.m_z - pt.m_z;
	if ( iDistZ < -PLAYER_HEIGHT )	// far under the water somehow
		return false;
	if ( iDistZ <= 0 )	// in or below the water
		return true;

	// Is there a solid surface under us ?
	WORD wBlockFlags = GetMoveBlockFlags();
	if ( IsSetEF( EF_WalkCheck ) )
	{
		if ( g_World.GetHeightPoint_New(ptTop, wBlockFlags, true) == pt.m_z )
			return true;
	}
	else
	{
		if ( g_World.GetHeightPoint(ptTop, wBlockFlags, true) == pt.m_z )
			return true;
	}

	return false;
}

NPCBRAIN_TYPE CChar::GetNPCBrain( bool fDefault ) const
{
	ADDTOCALLSTACK("CChar::GetNPCBrain");
	// return 1 for animal, 2 for monster, 3 for NPC humans and PCs
	// For tracking and other purposes.

	if ( m_pNPC && fDefault )
	{
		if ((( m_pNPC->m_Brain >= NPCBRAIN_HUMAN ) && ( m_pNPC->m_Brain <= NPCBRAIN_THIEF )) ||
			( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY ))
			return NPCBRAIN_HUMAN;

		return m_pNPC->m_Brain;
	}

	// handle the exceptions.
	CREID_TYPE id = GetDispID();
	if ( id >= CREID_MAN )
	{
		if ( id == CREID_BLADES || id == CREID_VORTEX )
			return NPCBRAIN_BERSERK;

		if ( id >= CREID_IRON_GOLEM && id <= CREID_MULTICOLORED_HORDE_DAEMON )
		{

			switch ( id )
			{
			case CREID_SERPENTINE_DRAGON:
			case CREID_SKELETAL_DRAGON:
			case CREID_REPTILE_LORD:
			case CREID_ANCIENT_WYRM:
			case CREID_SWAMP_DRAGON1:
			case CREID_SWAMP_DRAGON2:
				return NPCBRAIN_DRAGON;

			case CREID_SHADE:
			case CREID_MUMMY:
				return NPCBRAIN_UNDEAD;

			default:
				return NPCBRAIN_MONSTER;
			}
		}

		return NPCBRAIN_HUMAN;
	}
	if ( id >= CREID_HORSE1 )
		return( NPCBRAIN_ANIMAL );
	switch ( id )
	{
	case CREID_EAGLE:
	case CREID_BIRD:
	case CREID_GORILLA:
	case CREID_Snake:
	case CREID_Dolphin:
	case CREID_Giant_Toad:
	case CREID_Bull_Frog:
		return NPCBRAIN_ANIMAL;
	}
	return NPCBRAIN_MONSTER;
}

LPCTSTR CChar::GetPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPronoun");
	switch ( GetDispID())
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
			return( g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_HE) );
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
			return( g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_SHE) );
		default:
			return( g_Cfg.GetDefaultMsg(DEFMSG_PRONOUN_IT) );
	}
}

LPCTSTR CChar::GetPossessPronoun() const
{
	ADDTOCALLSTACK("CChar::GetPossessPronoun");
	switch ( GetDispID())
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
			return( g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HIS) );
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
			return( g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_HER) );
		default:
			return( g_Cfg.GetDefaultMsg(DEFMSG_POSSESSPRONOUN_ITS) );
	}
}

BYTE CChar::GetModeFlag( bool fTrueSight ) const
{
	ADDTOCALLSTACK("CChar::GetModeFlag");
	BYTE mode = 0;
	if ( IsStatFlag( STATF_Poisoned ))
		mode |= CHARMODE_POISON;
	if ( IsStatFlag( STATF_War ))
		mode |= CHARMODE_WAR;
	if ( IsStatFlag(STATF_Freeze|STATF_Sleeping|STATF_Hallucinating|STATF_Stone) )
		mode |= CHARMODE_YELLOW;
	if ( ! fTrueSight && IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden | STATF_Sleeping ))	// if not me, this will not show up !
		mode |= CHARMODE_INVIS;
	return( mode );
}

BYTE CChar::GetLightLevel() const
{
	ADDTOCALLSTACK("CChar::GetLightLevel");
	// Get personal light level.

	if ( IsStatFlag( STATF_DEAD ) || IsPriv(PRIV_DEBUG))	// dead don't need light.
		return( LIGHT_BRIGHT + 1 );
	if ( IsStatFlag( STATF_Sleeping ) && ! IsPriv( PRIV_GM ))	// eyes closed.
		return( LIGHT_DARK/2 );
	if ( IsStatFlag( STATF_NightSight ))
		return( LIGHT_BRIGHT );
	return( GetTopSector()->GetLight());
}

CItem * CChar::GetSpellbook( SPELL_TYPE iSpell ) const
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	CItem	*pBook = NULL;
	int		i = iSpell;

	//	search for suitable book in hands first
	pBook = GetContentHead();
	for ( ; pBook != NULL; pBook = pBook->GetNext() )
	{
		if ( pBook->IsTypeSpellbook() )
		{
			if (( i > pBook->m_itSpellbook.m_baseid ) && ( i - ( pBook->m_itSpellbook.m_baseid + 1 ) < 96 ))
				return pBook;
		}
	}

	//	then search in the top level of the pack
	CItemContainer *pPack = GetPack();
	if ( pPack )
	{
		pBook = pPack->GetContentHead();
		for ( ; pBook != NULL; pBook = pBook->GetNext() )
		{
			if ( pBook->IsTypeSpellbook() )
			{
				if (( i > pBook->m_itSpellbook.m_baseid ) && ( i - ( pBook->m_itSpellbook.m_baseid + 1 ) < 96 ))
					return pBook;
			}
		}
	}

	return NULL;
}

int CChar::Food_GetLevelPercent() const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelPercent");
	int	max	= Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return 100;
	else
		return IMULDIV( Stat_GetVal(STAT_FOOD), 100, max );
}

LPCTSTR CChar::Food_GetLevelMessage( bool fPet, bool fHappy ) const
{
	ADDTOCALLSTACK("CChar::Food_GetLevelMessage");
	int	max	= Stat_GetMax(STAT_FOOD);
	if ( max == 0)
		return g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_UNAFFECTED);
	int index = IMULDIV(Stat_GetVal(STAT_FOOD), 8, max);

	if ( fPet )
	{
		static LPCTSTR const sm_szPetHunger[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_1),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_2),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_3),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_4),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_5),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_6),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_7),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_HAPPY_8),
		};
		static LPCTSTR const sm_szPetHappy[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_1),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_2),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_3),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_4),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_5),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_6),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_7),
			g_Cfg.GetDefaultMsg(DEFMSG_PET_FOOD_8),
		};

		if ( index >= COUNTOF(sm_szPetHunger)-1 )
			index = COUNTOF(sm_szPetHunger)-1;

		return ( fHappy ? sm_szPetHappy[index] : sm_szPetHunger[index] );
	}

	static LPCTSTR const sm_szFoodLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_1),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_2),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_3),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_4),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_5),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_6),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_7),
		g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_8),
	};

	if ( index >= COUNTOF(sm_szFoodLevel)-1 )
		index = COUNTOF(sm_szFoodLevel)-1;

	return sm_szFoodLevel[index];
}

int CChar::Food_CanEat( CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CChar::Food_CanEat");
	// Would i want to eat this creature ? hehe
	// would i want to eat some of this item ?
	// 0 = not at all.
	// 10 = only if starving.
	// 20 = needs to be prepared.
	// 50 = not bad.
	// 75 = yummy.
	// 100 = my favorite (i will drop other things to get this).
	//

	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	int iRet = pCharDef->m_FoodType.FindResourceMatch( pObj );
	if ( iRet >= 0 )
	{
		return( pCharDef->m_FoodType[iRet].GetResQty()); // how bad do i want it ?
	}

	// ???
	return( 0 );
}

LPCTSTR CChar::GetTradeTitle() const // Paperdoll title for character p (2)
{
	ADDTOCALLSTACK("CChar::GetTradeTitle");
	static const CValStr sm_SkillTitles[] =
	{
		"", INT_MIN,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NEOPHYTE), 300,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_NOVICE), 400,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_APPRENTICE), 500,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_JOURNEYMAN), 600,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_EXPERT), 700,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_ADEPT), 800,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_MASTER), 900,
		g_Cfg.GetDefaultMsg(DEFMSG_SKILLTITLE_GRANDMASTER), 980,
		NULL, INT_MAX,
	};

	if ( ! m_sTitle.IsEmpty())
		return( m_sTitle );

	TCHAR * pTemp = Str_GetTemp();

	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// Incognito ?
	// If polymorphed then use the poly name.
	if ( IsStatFlag( STATF_Incognito ) ||
		! IsHuman() ||
		( m_pNPC && pCharDef->GetTypeName() != pCharDef->GetTradeName()))
	{
		if ( ! IsIndividualName())
			return( "" );	// same as type anyhow.
		sprintf( pTemp, "%s %s", (pCharDef->IsFemale()) ? g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_TRADETITLE_ARTICLE_MALE), pCharDef->GetTradeName());
		return( pTemp );
	}

	SKILL_TYPE skill = Skill_GetBest();
	int len = sprintf( pTemp, "%s ", sm_SkillTitles->FindName( Skill_GetBase(skill)));
	sprintf( pTemp+len, g_Cfg.GetSkillDef(skill)->m_sTitle, (pCharDef->IsFemale()) ? g_Cfg.GetDefaultMsg(DEFMSG_INCOGNITO_NAME_FEMALE) : g_Cfg.GetDefaultMsg(DEFMSG_INCOGNITO_NAME_MALE) );
	return( pTemp );
}

bool CChar::CanDisturb( CChar * pChar ) const
{
	ADDTOCALLSTACK("CChar::CanDisturb");
	// I can see/disturb only players with priv same/less than me.
	if ( !pChar )
		return false;
	if ( GetPrivLevel() < pChar->GetPrivLevel())
		return pChar->IsStatFlag(STATF_Insubstantial);
	return true;
}

bool CChar::CanSeeTrue( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CChar::CanSeeTrue");
	if ( pChar == NULL || pChar == this )
		return( false );
	// if ( pChar->IsStatFlag( STATF_Sleeping )) return( false );
	return( pChar->GetPrivLevel() < GetPrivLevel());
}

bool CChar::CanSeeInContainer( const CItemContainer * pContItem ) const
{
	ADDTOCALLSTACK("CChar::CanSeeInContainer");
	// This is a container of some sort. Can i see inside it ?

	if ( pContItem == NULL )	// must be a CChar ?
		return( true );

	if ( pContItem->IsSearchable())	// not a bank box or locked box.
		return( true );

	// Not normally searchable.
	// Make some special cases for searchable.

	const CChar * pChar = dynamic_cast <const CChar*> (pContItem->GetTopLevelObj());
	if ( pChar == NULL )
	{
		if ( IsPriv(PRIV_GM))
			return( true );
		return( false );
	}

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		if ( IsPriv(PRIV_GM) || pChar == this )
			return true;

		CItem * pItemTrade = pContItem->m_uidLink.ItemFind();
		if ( pItemTrade )
		{
			const CChar * pCharTrade = dynamic_cast <const CChar*> (pItemTrade->GetTopLevelObj());
			if ( pCharTrade == this )
				return true;
		}
		return false;
	}

	if ( ! pChar->NPC_IsOwnedBy( this ))	// player vendor boxes.
		return( false );

	if ( pContItem->IsType(IT_EQ_VENDOR_BOX) || pContItem->IsType(IT_EQ_BANK_BOX) )
	{
		// must be in the same position i opened it legitamitly.
		// see addBankOpen
		if ( !IsPriv(PRIV_GM) && ( pContItem->m_itEqBankBox.m_pntOpen != GetTopPoint() ))
			return false;
	}
	return true;
}

bool CChar::CanSee( const CObjBaseTemplate * pObj ) const
{
	ADDTOCALLSTACK("CChar::CanSee");
	// Can I see this object ( char or item ) ?
	if ( !pObj || /*pObj->*/IsDisconnected() )
		return false;

	//	first check the distance since if this will fail, we do not need to scan all
	//	subcontainers to find this result ;)
	if ( pObj->GetTopLevelObj()->GetTopPoint().GetDist(GetTopPoint()) > pObj->GetVisualRange() )
		return false;

	if ( pObj->IsItem() )
	{
		const CItem * pItem = STATIC_CAST <const CItem*>(pObj);
		if ( !pItem || !CanSeeItem(pItem) )
			return false;
		CObjBase * pObjCont = pItem->GetContainer();
		if ( pObjCont )
		{
			if ( !CanSeeInContainer( dynamic_cast <const CItemContainer*>(pObjCont) ))
				return( false );
			return( CanSee( pObjCont ));
		}
	}
	else
	{
		const CChar * pChar = STATIC_CAST <const CChar*>(pObj);
		if ( !pChar )
			return false;
		else if ( pChar == this )
			return true;
		else if ( IsPriv(PRIV_ALLSHOW) )
		{
			if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ) )
			{
				if ( pChar->GetPrivLevel() <= PLEVEL_Player )
				{
					if ( GetPrivLevel() <= pChar->GetPrivLevel())
						return( false );
				}
				else
				{
					if ( GetPrivLevel() < pChar->GetPrivLevel())
						return( false );
				}
			}
			return( true );
		}

		if ( pChar->IsStatFlag(STATF_DEAD) && m_pNPC )
		{
			if ( !(   m_pNPC->m_Brain == NPCBRAIN_HEALER || Skill_GetBase( SKILL_SPIRITSPEAK ) >= 1000 ) )
				return( false );
		}
		else if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ))
		{
			// Characters can be invisible, but not to GM's (true sight ?)
			// equal level can see each other
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( GetPrivLevel() <= pChar->GetPrivLevel())
					return( false );
			}
			else
			{
				if ( GetPrivLevel() < pChar->GetPrivLevel())
					return( false );
			}

		}
		if ( pChar->IsDisconnected() )
		{
			if ( pChar->IsStatFlag(STATF_Ridden) )
			{
				CChar *pCharRider = Horse_GetMountChar();
				if ( pCharRider )
					return CanSee(pCharRider);
				else
					return false;
			}

			// Check to see if the character is logged out. Only characters
			// with GM or ALLSHOW priv should be able to see these.
			if (!IsPriv( PRIV_GM ) )
				return false;
		}
	}

	if ( IsPriv(PRIV_ALLSHOW) && ( pObj->IsTopLevel() || pObj->IsDisconnected() ))
	{
		// don't exclude for logged out and diff maps.
		return (GetTopPoint().GetDistBase(pObj->GetTopPoint()) <= pObj->GetVisualRange());
	}
	return true;
}

bool CChar::CanSeeLOS( const CPointMap & ptDst, CPointMap * pptBlock, int iMaxDist, WORD wFlags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	if ( ( m_pPlayer || m_pNPC ) && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER) )
		return CanSeeLOS_New( ptDst, pptBlock, iMaxDist, wFlags );
	
	// Max distance of iMaxDist
	// Line of sight check
	// NOTE: if not blocked. pptBlock is undefined.
	// ToDo: move repeated blocks to a separate functions?
	// 3D LOS later - real LOS, i.e. we can't shoot through the floor, but can shoot through the hole in it

	if ( IsPriv( PRIV_GM ))
		return( true );

	CPointMap ptSrc = GetTopPoint();

	int iDist = ptSrc.GetDist( ptDst );
	if ( iDist > iMaxDist )
	{
blocked:
		if ( pptBlock != NULL )
			* pptBlock = ptSrc;
		return false; // blocked
	}
	// Walk towards the object. If any spot is too far above our heads
	// then we can not see what's behind it.
	int iDistTry = 0;
	while ( --iDist >= 0 )
	{
		DIR_TYPE dir = ptSrc.GetDir( ptDst );

		WORD wBlockFlags;
		if ( dir % 2 )		// test only diagonal dirs
		{
			CPointMap	ptTest;
			DIR_TYPE	dirTest1	= (DIR_TYPE) (dir-1);	// get 1st ortogonal
			DIR_TYPE	dirTest2	= (DIR_TYPE) (dir+1);	// get 2nd ortogonal
			if ( dirTest2 == DIR_QTY )		// roll over
				dirTest2	= (DIR_TYPE) 0;

			bool fBlocked = false;
			ptTest = ptSrc;
			ptTest.Move( dirTest1 );
			{
				wBlockFlags = CAN_C_SWIM | CAN_C_WALK | CAN_C_FLY;
				signed char z = g_World.GetHeightPoint( ptTest, wBlockFlags, true );;
				signed char zDiff	= abs( z - ptTest.m_z );
				if ( zDiff > PLAYER_HEIGHT ) fBlocked = true;
				else ptTest.m_z	= z;

				if ( wBlockFlags & ( CAN_I_BLOCK | CAN_I_DOOR ))
					fBlocked = true;
			}
			if ( fBlocked )
			{
				ptTest = ptSrc;
				ptTest.Move( dirTest2 );
				{
					wBlockFlags = CAN_C_SWIM | CAN_C_WALK | CAN_C_FLY;
					signed char z = g_World.GetHeightPoint( ptTest, wBlockFlags, true );;
					signed char zDiff	= abs( z - ptTest.m_z );
					if ( zDiff > PLAYER_HEIGHT ) goto blocked;
					else ptTest.m_z	= z;

					if ( wBlockFlags & ( CAN_I_BLOCK | CAN_I_DOOR ))
					{
						ptSrc = ptTest;
						goto blocked;
					}
				}
			}
		}
		if ( iDist )
		{
			ptSrc.Move( dir );	// NOTE: The dir is very coarse and can change slightly.

			wBlockFlags = CAN_C_SWIM | CAN_C_WALK | CAN_C_FLY;
			signed char z = g_World.GetHeightPoint( ptSrc, wBlockFlags, true );;
			signed char zDiff	= abs( z - ptSrc.m_z );

			if ( zDiff > PLAYER_HEIGHT ) goto blocked;
			else ptSrc.m_z	= z;

			if ( wBlockFlags & ( CAN_I_BLOCK | CAN_I_DOOR ))
				goto blocked;

			if ( iDistTry > iMaxDist )
			{
				// just went too far.
				goto blocked;
			}
			iDistTry ++;
		}
	}

	if ( abs( ptSrc.m_z - ptDst.m_z ) > PLAYER_HEIGHT ) return false;
	return true; // made it all the way to the object with no obstructions.
}

// a - gradient < x < b + gradient
#define BETWEENPOINT(coord, coordt, coords) ( ((coord) > (((double)minimum((coordt),(coords)))-0.5)) && ((coord) < (((double)maximum((coordt),(coords)))+0.5)) )
#define APPROX(num) ((double)(((num)-floor(num))>0.5)?(ceil(num)):(floor(num)))
#define CALCITEMHEIGHT(num) (num)+((pItemDef->GetTFlags()&0x400)?(pItemDef->GetHeight()/2):pItemDef->GetHeight())
//#define DEBUGCANSEELOS 1
//#ifdef _DEBUG
#ifdef DEBUGCANSEELOS
	#define WARNLOS(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNLOS(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_LOS ) { g_pLog->EventWarn _x_; }
#endif

bool inline CChar::CanSeeLOS_New_Failed( CPointMap * pptBlock, CPointMap &ptNow ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New_Failed");
	if ( pptBlock )
		* pptBlock = ptNow;
	return false;
}

bool CChar::CanSeeLOS_New( const CPointMap & ptDst, CPointMap * pptBlock, int iMaxDist, WORD flags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS_New");
	if ( IsPriv( PRIV_GM ))
	{
		WARNLOS(("GM Pass\n"))
		return( true );
	}
		
	CPointMap ptSrc = GetTopPoint();
	CPointMap ptNow(ptSrc);

	if ( ptSrc.m_map != ptDst.m_map )	// Different map
		return this->CanSeeLOS_New_Failed( pptBlock, ptNow );
		
	if ( ptSrc == ptDst )	// Same point ^^
		return true;

	ptSrc.m_z += GetHeightMount( true ); //true - substract one from the height because of eyes height
	WARNLOS(("Total Z: %d\n",ptSrc.m_z))
	/*
	if ( IsHuman() )
	{
		WARNLOS(("LOS: Human\n"))

		if ( IsStatFlag(STATF_OnHorse) )
		{
			WARNLOS(("LOS: Human Mounted\n"))

			// Mounted player?
			//ptSrc.m_z += (g_Cfg.m_iMountHeight) ? (g_Cfg.m_iMountHeight) : (PLAYER_HEIGHT + 4); //20 should be height of one floor, so 19 (15+4) is our maximum height
			ptSrc.m_z += PLAYER_HEIGHT + 4; //20 should be height of one floor, so 19 (15+4) is our maximum height
		}
		else
		{
			ptSrc.m_z += (PLAYER_HEIGHT - 1); // Eye height
		}
	}
	else // TODO: Well i have no clue for determining height of Npcs/Morphed players. Adding a def for npcs??
	{
		WARNLOS(("LOS: GetHeight\n"))
		ptSrc.m_z += GetHeight();
	}
	WARNLOS(("Total Z: %d\n",ptSrc.m_z))*/

	int dx, dy, dz;
	double dist2d, dist3d;
	
	dx = ptDst.m_x - ptSrc.m_x;
	dy = ptDst.m_y - ptSrc.m_y;
	dz = ptDst.m_z - ptSrc.m_z;
	
	dist2d = sqrt(pow((double)dx,2)+pow((double)dy,2));
	if (dz)
		dist3d = sqrt(pow((double)dist2d,2)+pow((double)dz,2));
	else
		dist3d = dist2d;
	
	if ( APPROX(dist2d) > ((double)iMaxDist) )
	{
		WARNLOS(("( APPROX(dist2d)(%d) > ((double)iMaxDist)(%d) ) --> NOLOS\n",APPROX(dist2d),((double)iMaxDist)))
		return( CanSeeLOS_New_Failed( pptBlock, ptNow ));
	}
	
	double dFactorX, dFactorY, dFactorZ;
	
	dFactorX = dx / dist3d;
	dFactorY = dy / dist3d;
	dFactorZ = dz / dist3d;
	
	double nPx, nPy, nPz;
	nPx = ptSrc.m_x; nPy = ptSrc.m_y; nPz = ptSrc.m_z;
	
	std::vector<CPointMap> path;
	
	while ( true )
	{
		if ( BETWEENPOINT(nPx,ptDst.m_x,ptSrc.m_x) && BETWEENPOINT(nPy,ptDst.m_y,ptSrc.m_y) && BETWEENPOINT(nPz,ptDst.m_z,ptSrc.m_z) )
		{
			dx = (int)APPROX(nPx);
			dy = (int)APPROX(nPy);
			dz = (int)APPROX(nPz);

			// Add point to vector
			if ( path.size() )
			{
				CPointMap ptEnd = path.at(path.size() - 1);
				if ( ptEnd.m_x != dx || ptEnd.m_y != dy || ptEnd.m_z != dz )
					path.push_back(CPointMap(dx,dy,dz,ptSrc.m_map));
			}
			else
			{
				path.push_back(CPointMap(dx,dy,dz,ptSrc.m_map));
			}
			WARNLOS(("PATH X:%d Y:%d Z:%d\n",dx,dy,dz))

			nPx += dFactorX;
			nPy += dFactorY;
			nPz += dFactorZ;
		}
		else
			break;
	}
	
	if ( path.size() )
	{
		if ( path.at(path.size() - 1) != ptDst ) 
			path.push_back(CPointMap(ptDst.m_x,ptDst.m_y,ptDst.m_z,ptDst.m_map));
	}
	else
	{
		path.clear();
		return( CanSeeLOS_New_Failed( pptBlock, ptNow ));
	}
	
	WARNLOS(("Path calculated %d\n", path.size()))
	// Ok now we should loop through all the points and checking for maptile, staticx, items, multis.
	// If something is in the way and it has the wrong flags LOS return false
	
	const CGrayMapBlock * pBlock		= NULL;		// Block of the map (for statics)
	const CUOStaticItemRec * pStatic	= NULL;		// Statics iterator (based on GrayMapBlock)
	const CGrayMulti * pMulti 			= NULL;		// Multi Def (multi check)
	const CUOMultiItemRec * pMultiItem	= NULL;		// Multi item iterator
	CRegionBase * pRegion				= NULL;		// Nulti regions
	CRegionLinks rlinks;							// Links to multi regions
	CItem * pItem						= NULL;
	CItemBase * pItemDef 				= NULL;
	CItemBaseDupe * pDupeDef			= NULL;
	int iQtyr, iQty, i, ii, iii, s		= 0;

	bool IsID_Chair( ITEMID_TYPE id );

	DWORD wTFlags = 0;
	BYTE Height = 0;
	WORD terrainid = 0;
	bool bPath = true;
	bool bNullTerrain = false;

	CRegionBase * pSrcRegion = ptSrc.GetRegion( REGION_TYPE_AREA | REGION_TYPE_ROOM | REGION_TYPE_MULTI );
	CRegionBase * pNowRegion = NULL;
	
	int lp_x = 0; int lp_y = 0; 
	signed char min_z = 0, max_z = 0, z = 0;
	
	for( i = 0; i < path.size(); lp_x = ptNow.m_x, lp_y = ptNow.m_y, pItemDef = NULL, pStatic = NULL, pMulti = NULL, pMultiItem = NULL, min_z = 0, max_z = 0, ++i )
	{
		ptNow = path.at(i);
		WARNLOS(("---------------------------------------------\n"));
		WARNLOS(("Point %d,%d,%d \n", ptNow.m_x,ptNow.m_y,ptNow.m_z))

		pNowRegion = ptNow.GetRegion( REGION_TYPE_AREA | REGION_TYPE_ROOM | REGION_TYPE_MULTI );

		if (( flags & LOS_NO_OTHER_REGION ) && ( pSrcRegion != pNowRegion ))
		{
			WARNLOS(("flags & 0200 and path is leaving my region - BLOCK\n"))
			bPath = false;
			break;
		}

		if (( flags & LOS_NC_MULTI ) && ( ptNow.GetRegion( REGION_TYPE_MULTI) ) && ( ptNow.GetRegion( REGION_TYPE_MULTI) != ptSrc.GetRegion( REGION_TYPE_MULTI )))
		{
			WARNLOS(("flags & 0400 and path is crossing another multi - BLOCK\n"))
			bPath = false;
			break;
		}

		if ( lp_x != ptNow.m_x || lp_y != ptNow.m_y )
		{
			WARNLOS(("\tLoading new map block.\n"))
			pBlock = g_World.GetMapBlock( ptNow );
		}

		if ( !pBlock ) // something is wrong
		{
			WARNLOS(("GetMapBlock Failed\n"))
			bPath = false;
			break;
		}

		if (!( flags & LOS_NB_TERRAIN ))
		{
			if (!(( flags & LOS_NB_LOCAL_TERRAIN ) && ( pSrcRegion == pNowRegion )))
			{
				// ------ MapX.mul Check ----------

				terrainid = pBlock->GetTerrain( UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_wTerrainIndex;
				WARNLOS(("Terrain %d\n", terrainid))

				if (( flags & LOS_FISHING ) && ( ptSrc.GetDist(ptNow) >= 2 ) && ( g_World.GetTerrainItemType( terrainid ) != IT_WATER ) && ( g_World.GetTerrainItemType( terrainid ) != IT_NORMAL ))
				{
					WARNLOS(("Terrain %d blocked - flags & LOS_FISHING, distance >= 2 and type of pItemDef is not IT_WATER\n",terrainid))
						WARNLOS(("ptSrc: %d,%d,%d; ptNow: %d,%d,%d; terrainid: %d; terrainid type: %d\n",ptSrc.m_x,ptSrc.m_y,ptSrc.m_z,ptNow.m_x,ptNow.m_y,ptNow.m_z,terrainid,g_World.GetTerrainItemType( terrainid )))
					bPath = false;
					break;
				}

				#define MAPTILEMIN minimum(minimum(minimum(pBlock->GetTerrain(0,0)->m_z,pBlock->GetTerrain(0,1)->m_z),pBlock->GetTerrain(1,0)->m_z),pBlock->GetTerrain(1,1)->m_z)
				#define MAPTILEMAX maximum(maximum(maximum(pBlock->GetTerrain(0,0)->m_z,pBlock->GetTerrain(0,1)->m_z),pBlock->GetTerrain(1,0)->m_z),pBlock->GetTerrain(1,1)->m_z);

				//#define MAPTILEZ pBlock->GetTerrain( UO_BLOCK_OFFSET(ptNow.m_x), UO_BLOCK_OFFSET(ptNow.m_y))->m_z;

				if (( terrainid != TERRAIN_HOLE ) && ( terrainid != 475 ))
				{
					if ( terrainid < 430 || terrainid > 437 )
					{
						min_z = MAPTILEMIN;
						max_z = MAPTILEMAX;
						WARNLOS(("Terrain %d - m:%d M:%d\n", terrainid, min_z, max_z))
						if ( CUOMapMeter::IsTerrainNull( terrainid ) )
							bNullTerrain = true; //what if there are some items on that hole?
						if (( min_z <= ptNow.m_z && max_z >= ptNow.m_z ) && ( ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y  || min_z > ptDst.m_z || max_z < ptDst.m_z ))
						{
							WARNLOS(("Terrain %d - m:%d M:%d - block\n", terrainid, min_z, max_z))
							bPath = false;
							break;
						}
					}
				}
			#undef MAPTILEMIN
			#undef MAPTILEMAX
			//#undef MAPTILEZ
			}
		}

		// ----------------------------------

		// ------ StaticsX.mul Check --------
		if (!( flags & LOS_NB_STATIC ))
		{
			if (!(( flags & LOS_NB_LOCAL_STATIC ) && ( pSrcRegion == pNowRegion )))
			{
				for ( s = 0; s < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, pItemDef = NULL, ++s )
				{
					pStatic = pBlock->m_Statics.GetStatic(s);
					if (pStatic->m_x+pBlock->m_x != ptNow.m_x || pStatic->m_y+pBlock->m_y != ptNow.m_y)
						continue;

					pItemDef = CItemBase::FindItemBase( pStatic->GetDispID() );
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( ! pItemDef )
					{
						WARNLOS(("STATIC - Cannot get pItemDef for item (0%x)\n", pStatic->GetDispID()))
					}
					else
					{
						if (( flags & LOS_FISHING ) && ( ptSrc.GetDist(ptNow) >= 2 ) && ( pItemDef->GetType() != IT_WATER ) && (( pItemDef->m_Can & CAN_I_DOOR ) || ( pItemDef->m_Can & CAN_I_PLATFORM ) || ( pItemDef->m_Can & CAN_I_BLOCK ) || ( pItemDef->m_Can & CAN_I_CLIMB ) || ( pItemDef->m_Can & CAN_I_FIRE ) || ( pItemDef->m_Can & CAN_I_ROOF )))
						{
							WARNLOS(("pStatic blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"))
							bPath = false;
							break;
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if (( IsSetEF(EF_NewPositionChecks) ) && ( pItemDef->GetID() != pStatic->GetDispID() )) //not a parent item
						{
							WARNLOS(("Not a parent item (STATIC)\n"))
							pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pStatic->GetDispID() );
							if ( ! pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (static) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pStatic->GetDispID(),ptNow.m_x,ptNow.m_y,pStatic->m_z);
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
							WARNLOS(("Parent item or EF_NewPositionChecks off (DYNAMIC)\n"))

						Height = ( wTFlags & UFLAG2_CLIMBABLE ) ? ( Height / 2 ) : ( Height );

						//if (( wTFlags & 0x2000|0x40 ) && ( IsSetEF(EF_NewPositionChecks) )
						if (( wTFlags & UFLAG1_WALL|UFLAG1_BLOCK ) && !(( wTFlags & UFLAG2_WINDOW ) && ( flags & LOS_NB_WINDOWS )) && ( IsSetEF(EF_NewPositionChecks) ) || (( wTFlags & 0x3000 ) && ( !IsSetEF(EF_NewPositionChecks) )))
						{
							WARNLOS(("pStatic %0x %d,%d,%d - %d\n",pStatic->GetDispID(),pStatic->m_x,pStatic->m_y,pStatic->m_z,Height))
							min_z = pStatic->m_z;
							max_z = IsSetEF(EF_NewPositionChecks) ? ( Height + min_z ) : CALCITEMHEIGHT(min_z);
							WARNLOS(("wTFlags(0%x)\n",wTFlags))

							WARNLOS(("pStatic %0x Z check: %d,%d (Now: %d) (Dest: %d).\n",pStatic->GetDispID(),min_z,max_z,ptNow.m_z,ptDst.m_z))
							if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
							{
								if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
								{
									WARNLOS(("pStatic blocked - m:%d M:%d\n", min_z,max_z))
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}
		
		if ( !bPath )
			break;
	
		// ----------------------------------
		
		// --------- In game items ----------
		if (!( flags & LOS_NB_DYNAMIC ))
		{
			if (!(( flags & LOS_NB_LOCAL_DYNAMIC ) && ( pSrcRegion == pNowRegion )))
			{
				CWorldSearch AreaItems( ptNow, 0 );
				while(true)
				{
					pItem = AreaItems.GetItem();
					if ( !pItem )
						break;
					if (pItem->GetUnkPoint().m_x != ptNow.m_x || pItem->GetUnkPoint().m_y != ptNow.m_y)
						continue;
					if ( !CanSeeItem(pItem) )
						continue;

					pItemDef = CItemBase::FindItemBase( pItem->GetDispID() );
					wTFlags = 0;
					Height = 0;
					bNullTerrain = false;

					if ( ! pItemDef )
					{
						WARNLOS(("DYNAMIC - Cannot get pItemDef for item (0%x)\n", pItem->GetDispID()))
					}
					else
					{
						if (( flags & LOS_FISHING ) && ( ptSrc.GetDist(ptNow) >= 2 ) && ( pItemDef->GetType() != IT_WATER ) && (( pItemDef->m_Can & CAN_I_DOOR ) || ( pItemDef->m_Can & CAN_I_PLATFORM ) || ( pItemDef->m_Can & CAN_I_BLOCK ) || ( pItemDef->m_Can & CAN_I_CLIMB ) || ( pItemDef->m_Can & CAN_I_FIRE ) || ( pItemDef->m_Can & CAN_I_ROOF )))
						{
							WARNLOS(("pItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"))
							bPath = false;
							break;
							// return( CanSeeLOS_New_Failed( pptBlock, ptNow ));
						}

						wTFlags = pItemDef->GetTFlags();
						Height = pItemDef->GetHeight();

						if (( IsSetEF(EF_NewPositionChecks) ) && ( pItemDef->GetID() != pItem->GetDispID() )) //not a parent item
						{
							WARNLOS(("Not a parent item (DYNAMIC)\n"))
							pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pItem->GetDispID() );
							if ( ! pDupeDef )
							{
								g_Log.EventDebug("Failed to get non-parent reference (dynamic) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pItem->GetDispID(),ptNow.m_x,ptNow.m_y,pItem->GetUnkZ());
								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();
							}
							else
							{
								wTFlags = pDupeDef->GetTFlags();
								Height = pDupeDef->GetHeight();
							}
						}
						else
							WARNLOS(("Parent item or EF_NewPositionChecks off (DYNAMIC)\n"))

						Height = ( wTFlags & UFLAG2_CLIMBABLE ) ? ( Height / 2 ) : ( Height );

						//if (( wTFlags & 0x2000|0x40 ) && ( IsSetEF(EF_NewPositionChecks) )
						if (( wTFlags & UFLAG1_WALL|UFLAG1_BLOCK  ) && !(( wTFlags & UFLAG2_WINDOW ) && ( flags & LOS_NB_WINDOWS )) && ( IsSetEF(EF_NewPositionChecks) ) || (( wTFlags & 0x3000 ) && ( !IsSetEF(EF_NewPositionChecks) )))
						{
							WARNLOS(("pItem %0x(%0x) %d,%d,%d - %d\n",pItem->GetUID(),pItem->GetDispID(),pItem->GetUnkPoint().m_x,pItem->GetUnkPoint().m_y,pItem->GetUnkPoint().m_z,Height))
							min_z = pItem->GetUnkPoint().m_z;
							max_z = IsSetEF(EF_NewPositionChecks) ? ( Height + min_z ) : CALCITEMHEIGHT(min_z);
							WARNLOS(("wTFlags(0%x)\n",wTFlags))

							WARNLOS(("pItem %0x(%0x) Z check: %d,%d (Now: %d) (Dest: %d).\n",pItem->GetUID(),pItem->GetDispID(),min_z,max_z,ptNow.m_z,ptDst.m_z))
							if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
							{
								if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
								{
									WARNLOS(("pItem blocked - m:%d M:%d\n", min_z,max_z))
									bPath = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( !bPath )
			break;

		// ----------------------------------
		
		// ----------- Multis ---------------
		
		// TODO: I think that this part should be optimized
		if (!( flags & LOS_NB_MULTI ))
		{
			if (!(( flags & LOS_NB_LOCAL_MULTI ) && ( pSrcRegion == pNowRegion )))
			{
				iQtyr = ptNow.GetRegions( REGION_TYPE_MULTI, rlinks );
				if ( iQtyr )
				{
					for ( ii = 0; ii < iQtyr; pMulti = NULL, ++ii, pItem = NULL, pRegion = NULL )
					{
						pRegion = rlinks.GetAt(ii);
						if ( pRegion )
							pItem = pRegion->GetResourceID().ItemFind();

						if ( !pItem )
							continue;

						pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID() );
						if ( !pMulti )
							continue;

						iQty = pMulti->GetItemCount();
						for ( iii = 0; --iQty; pItemDef = NULL, pMultiItem = NULL, ++iii )
						{
							pMultiItem = pMulti->GetItem(iii);
		
							if ( !pMultiItem )
								break;

							if ( !pMultiItem->m_visible )
								continue;

							if ( ( pMultiItem->m_dx + pItem->GetTopPoint().m_x != ptNow.m_x ) || ( pMultiItem->m_dy + pItem->GetTopPoint().m_y != ptNow.m_y ) )
								continue;

							pItemDef = CItemBase::FindItemBase( pMultiItem->GetDispID() );
							wTFlags = 0;
							Height = 0;
							bNullTerrain = false;

							if ( ! pItemDef )
							{
								WARNLOS(("MULTI - Cannot get pItemDef for item (0%x)\n", pMultiItem->GetDispID()))
							}
							else
							{
								if (( flags & LOS_FISHING ) && ( ptSrc.GetDist(ptNow) >= 2 ) && ( pItemDef->GetType() != IT_WATER ) && (( pItemDef->m_Can & CAN_I_DOOR ) || ( pItemDef->m_Can & CAN_I_PLATFORM ) || ( pItemDef->m_Can & CAN_I_BLOCK ) || ( pItemDef->m_Can & CAN_I_CLIMB ) || ( pItemDef->m_Can & CAN_I_FIRE ) || ( pItemDef->m_Can & CAN_I_ROOF )))
								{
									WARNLOS(("pMultiItem blocked - flags & 0800, distance >= 2 and type of pItemDef is not IT_WATER\n"))
									bPath = false;
									break;
									// return( CanSeeLOS_New_Failed( pptBlock, ptNow ));
								}

								wTFlags = pItemDef->GetTFlags();
								Height = pItemDef->GetHeight();

								if (( IsSetEF(EF_NewPositionChecks) ) && ( pItemDef->GetID() != pMultiItem->GetDispID() )) //not a parent item
								{
									WARNLOS(("Not a parent item (MULTI)\n"))
									pDupeDef = CItemBaseDupe::GetDupeRef((ITEMID_TYPE) pMultiItem->GetDispID() );
									if ( ! pDupeDef )
									{
										g_Log.EventDebug("Failed to get non-parent reference (multi) (DispID 0%x) (X: %d Y: %d Z: %d)\n",pMultiItem->GetDispID(),ptNow.m_x,ptNow.m_y,pMultiItem->m_dz+pItem->GetTopPoint().m_z);
										wTFlags = pItemDef->GetTFlags();
										Height = pItemDef->GetHeight();
									}
									else
									{
										wTFlags = pDupeDef->GetTFlags();
										Height = pDupeDef->GetHeight();
									}
								}
								else
									WARNLOS(("Parent item or EF_NewPositionChecks off (MULTI)\n"))

								Height = ( wTFlags & UFLAG2_CLIMBABLE ) ? ( Height / 2 ) : ( Height );

								//if (( wTFlags & 0x2000|0x40 ) && ( IsSetEF(EF_NewPositionChecks) )
								if (( wTFlags & UFLAG1_WALL|UFLAG1_BLOCK  ) && !(( wTFlags & UFLAG2_WINDOW ) && ( flags & LOS_NB_WINDOWS )) && ( IsSetEF(EF_NewPositionChecks) ) || (( wTFlags & 0x3000 ) && ( !IsSetEF(EF_NewPositionChecks) )))
								{
									WARNLOS(("pMultiItem %0x %d,%d,%d - %d\n",pMultiItem->GetDispID(),pMultiItem->m_dx,pMultiItem->m_dy,pMultiItem->m_dz,Height))
									min_z = pMultiItem->m_dz + pItem->GetTopPoint().m_z;
									max_z = IsSetEF(EF_NewPositionChecks) ? ( Height + min_z ) : CALCITEMHEIGHT(min_z);
									WARNLOS(("wTFlags(0%x)\n",wTFlags))

									if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
									{
										if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
										{
											WARNLOS(("pMultiItem blocked - m:%d M:%d\n", min_z,max_z))
											bPath = false;
											break;
										}
									}
								}
							}
						}

						if ( !bPath )
							break;
					}
				}
			}
		}

		if ( bNullTerrain )
			bPath = false;

		if ( !bPath )
			break;
			
		// ----------------------------------
	}

	path.clear();

	if ( !bPath )
	{
		return( CanSeeLOS_New_Failed( pptBlock, ptNow ));
	}

	return true;
}

#ifdef DEBUGCANSEELOS
	#undef DEBUGCANSEELOS
#endif
#undef WARNLOS
#undef APPROX
#undef BETWEENPOINT
#undef CALCITEMHEIGHT

bool CChar::CanSeeLOS( const CObjBaseTemplate * pObj, WORD wFlags ) const
{
	ADDTOCALLSTACK("CChar::CanSeeLOS");
	if ( ! CanSee( pObj ))
		return( false );
	pObj = pObj->GetTopLevelObj();
	return( CanSeeLOS( pObj->GetTopPoint(), NULL, pObj->GetVisualRange(), wFlags));
}

bool CChar::CanTouch( const CPointMap & pt ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am.
	// swords, spears, arms length = x units.
	// Use or Grab.
	// Check for blocking objects.
	// It this in a container we can't get to ?

	return( CanSeeLOS( pt, NULL, 6 ));
}

bool CChar::CanTouch( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CChar::CanTouch");
	// Can I reach this from where i am. swords, spears, arms length = x units.
	// Use or Grab. May be in snooped container.
	// Check for blocking objects. Is this in a container we can't get to ?

	if ( !pObj )
		return false;

	const CItem * thisItem = dynamic_cast <const CItem*> (pObj);

	const CObjBaseTemplate	*pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	const CItem * pItem;
	const CChar * pChar;

	bool fDeathImmune = IsPriv(PRIV_GM);
	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
		pItem = dynamic_cast <const CItem*> (pObj);
		if ( !pItem )
			return false;

		switch ( pItem->GetType())
		{
			case IT_SIGN_GUMP:	// can be seen from a distance.
				return( iDist < pObjTop->GetVisualRange());
	
			case IT_TELESCOPE:
			case IT_SHRINE:	// We can use shrines when dead !!
				fDeathImmune = true;
				break;
			case IT_SHIP_SIDE:
			case IT_SHIP_SIDE_LOCKED:
			case IT_SHIP_PLANK:
			case IT_ARCHERY_BUTTE:	// use from distance.
				if ( IsStatFlag( STATF_Sleeping | STATF_Freeze | STATF_Stone ))
					break;
				return( GetTopDist3D( pItem->GetTopLevelObj()) <= UO_MAP_VIEW_SIZE );
		}
	}

	if ( !fDeathImmune )
		if ( IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) )
			return false;

	//	search up to top level object
	if ( pObjTop != this )
	{
		if ( pObjTop->IsChar() )
		{
			pChar = dynamic_cast <const CChar*> (pObjTop);
			if ( !pChar )
				return false;

			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return GetPrivLevel() >= pChar->GetPrivLevel();
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Stone) )
				return false;
		}

		CObjBase * pObjCont;
		CObjBase * pObjTest = const_cast<CObjBase*>(pObj);
		while (true)
		{
			pItem = dynamic_cast <const CItem*>(pObjTest);
			if ( !pItem )
				break;

			// What is this inside of ?
			pObjCont = pItem->GetContainer();
			if ( !pObjCont )
				break;

			pObjTest = pObjCont;
			if ( !CanSeeInContainer(dynamic_cast <const CItemContainer*>(pObjTest)) )
				return false;
		}
	}

	if ( IsPriv(PRIV_GM) )
		return true;

	if ( ! CanSeeLOS(pObjTop->GetTopPoint(), NULL, pObjTop->GetVisualRange()) )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNORELOS )
			return true;
		else if ( pObj->IsChar() && pChar->GetAbilityFlags() & CAN_C_DCIGNORELOS )
			return true;
		else if ( pObj->IsItem() && pItem->GetAbilityFlags() & CAN_I_DCIGNORELOS )
			return true;
		else
			return false;
	}
	if ( iDist > 3 )
	{
		if ( GetAbilityFlags() & CAN_C_DCIGNOREDIST )
			return true;
		else if ( pObj->IsChar() && pChar->GetAbilityFlags() & CAN_C_DCIGNOREDIST )
			return true;
		else if ( pObj->IsItem() && pItem->GetAbilityFlags() & CAN_I_DCIGNOREDIST )
			return true;
		else
			return false;
	}
	return true;
}

IT_TYPE CChar::CanTouchStatic( CPointMap & pt, ITEMID_TYPE id, CItem * pItem )
{
	ADDTOCALLSTACK("CChar::CanTouchStatic");
	// Might be a dynamic or a static.
	// RETURN:
	//  IT_JUNK = too far away.
	//  set pt to the top level point.

	if ( pItem )
	{
		if ( ! CanTouch( pItem ))
			return( IT_JUNK );
		pt = GetTopLevelObj()->GetTopPoint();
		return( pItem->GetType());
	}

	// Its a static !

	CItemBase * pItemDef = CItemBase::FindItemBase(id);
	if ( pItemDef == NULL )
		return( IT_NORMAL );

	if ( ! CanTouch( pt ))
		return( IT_JUNK );

	// Is this static really here ?
	const CGrayMapBlock * pMapBlock = g_World.GetMapBlock( pt );
	if ( !pMapBlock )
		return IT_JUNK;

	int x2=pMapBlock->GetOffsetX(pt.m_x);
	int y2=pMapBlock->GetOffsetY(pt.m_y);

	int iQty = pMapBlock->m_Statics.GetStaticQty();
	for ( int i=0; i < iQty; ++i )
	{
		if ( ! pMapBlock->m_Statics.IsStaticPoint(i,x2,y2))
			continue;
		const CUOStaticItemRec * pStatic = pMapBlock->m_Statics.GetStatic( i );
		if ( id == pStatic->GetDispID() )
			return( pItemDef->GetType() );
	}

	return( IT_NORMAL );
}

bool CChar::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CChar::CanHear");
	// can we hear text or sound. (not necessarily understand it (ghost))
	// Can't hear TALKMODE_SAY through house walls.
	// NOTE: Assume pClient->CanHear() has already been called. (if it matters)

	if ( pSrc == NULL )	// must be broadcast i guess.
		return( true );
	bool	fThrough	= false;;
	int		iHearRange;
	
	switch ( mode )
	{
		case TALKMODE_YELL:
			if ( g_Cfg.m_iDistanceYell < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceYell;
			fThrough	= true;
			break;
		case TALKMODE_BROADCAST:
			return( true );
		case TALKMODE_WHISPER:
			if ( g_Cfg.m_iDistanceWhisper < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceWhisper;
			break;
		default:
			if ( g_Cfg.m_iDistanceTalk < 0 )
				return true;
			iHearRange = g_Cfg.m_iDistanceTalk;
			break;
	}

	pSrc = pSrc->GetTopLevelObj();
	int iDist = GetTopDist3D( pSrc );
	if ( iDist > iHearRange )	// too far away
		return( false );

	if ( IsSetOF( OF_NoHouseMuteSpeech ) )
		return( true );

	if ( fThrough )	// a yell goes through walls..
		return( true );

	// sound can be blocked if in house.
	CRegionWorld * pSrcRegion;
	if ( pSrc->IsChar())
	{
		const CChar* pCharSrc = dynamic_cast <const CChar*> (pSrc);
		ASSERT(pCharSrc);
		pSrcRegion = pCharSrc->GetRegion();
	}
	else
	{
		pSrcRegion = dynamic_cast <CRegionWorld *>( pSrc->GetTopPoint().GetRegion( REGION_TYPE_MULTI|REGION_TYPE_AREA ));
	}

	if ( m_pArea == pSrcRegion )	// same region is always ok.
		return( true );
	if ( pSrcRegion == NULL || m_pArea == NULL ) // should not happen really.
		return( false );
	if ( pSrcRegion->GetResourceID().IsItem() && ! pSrcRegion->IsFlag(REGION_FLAG_SHIP))
		return( false );
	if ( m_pArea->GetResourceID().IsItem() && ! m_pArea->IsFlag(REGION_FLAG_SHIP))
		return( false );

	return( true );
}

bool CChar::CanMove( CItem * pItem, bool fMsg ) const
{
	ADDTOCALLSTACK("CChar::CanMove");
	// Is it possible that i could move this ?
	// NOT: test if need to steal. IsTakeCrime()
	// NOT: test if close enough. CanTouch()

	if ( IsPriv(PRIV_ALLMOVE|PRIV_DEBUG|PRIV_GM) )
		return true;

	if ( IsStatFlag(STATF_Stone|STATF_Freeze|STATF_Insubstantial|STATF_DEAD|STATF_Sleeping) )
	{
		if ( fMsg )
			SysMessagef("%s", g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_DEAD));
		return false;
	}
	if ( !pItem )
		return false;

	if ( pItem->IsTopLevel() )
	{
		if ( pItem->IsTopLevelMultiLocked() )
		{
			if ( fMsg )
				SysMessagef("%s", g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_MULTI));

			return false;
		}
	}
	else	// equipped or in a container.
	{
		// Can't move items from the trade window (client limitation)
		if ( pItem->GetContainer()->IsContainer() )
		{
			const CItemContainer * pItemCont = dynamic_cast <const CItemContainer *> (pItem->GetContainer());
			if ( pItemCont && pItemCont->IsType( IT_EQ_TRADE_WINDOW ) )
			{
				SysMessage(g_Cfg.GetDefaultMsg( DEFMSG_TRADE_CANTMOVE ));
				return false;
			}
		}

		// Can't move equipped cursed items
		if ( pItem->IsAttr(ATTR_CURSED|ATTR_CURSED2) && pItem->IsItemEquipped() )
		{
			pItem->SetAttr(ATTR_IDENTIFIED);
			if ( fMsg )
				SysMessagef("%s %s", (LPCTSTR) pItem->GetName(), g_Cfg.GetDefaultMsg(DEFMSG_CANTMOVE_CURSED));

			return false;
		}

		// Can't steal/move newbie items on another cchar. (even if pet)
		if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_BLESSED2|ATTR_CURSED|ATTR_CURSED2) )
		{
			const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
			if ( pObjTop->IsItem() )	// is this a corpse or sleeping person ?
			{
				const CItemCorpse * pCorpse = dynamic_cast <const CItemCorpse *>(pObjTop);
				if ( pCorpse )
				{
					CChar * pChar = pCorpse->IsCorpseSleeping();
					if ( pChar && pChar != this )
						return false;
				}
			}
			else if ( pObjTop->IsChar() && pObjTop != this )
			{
				if ( !pItem->IsItemEquipped() || pItem->GetEquipLayer() != LAYER_DRAGGING )
					return false;
			}
		}

		if ( pItem->IsItemEquipped() )
		{
			LAYER_TYPE layer = pItem->GetEquipLayer();
			switch ( layer )
			{
			case LAYER_DRAGGING:
				return true;
			case LAYER_HAIR:
			case LAYER_BEARD:
			case LAYER_PACK:
				if ( !IsPriv(PRIV_GM) )
					return false;
				break;
			default:
				if ( !CItemBase::IsVisibleLayer(layer) && ! IsPriv(PRIV_GM) )
					return false;
			}
		}
	}

	return pItem->IsMovable();
}

bool CChar::IsTakeCrime( const CItem * pItem, CChar ** ppCharMark ) const
{
	ADDTOCALLSTACK("CChar::IsTakeCrime");
	// We are snooping or stealing.
	// Is taking this item a crime ?
	// RETURN:
	//  ppCharMark = The character we are offending.
	//  false = no crime.

	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE))
		return( false );

	CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	CChar * pCharMark = dynamic_cast <CChar *> (pObjTop);
	if ( ppCharMark != NULL )
	{
		*ppCharMark = pCharMark;
	}

	if ( static_cast <const CObjBase *>(pObjTop) == this )
	{
		// this is yours
		return( false );
	}

	if ( pCharMark == NULL )	// In some (or is) container.
	{
		if ( pItem->IsAttr(ATTR_OWNED) && pItem->m_uidLink != GetUID())
			return( true );

		CItemContainer * pCont = dynamic_cast <CItemContainer *> (pObjTop);
		if (pCont)
		{
			if ( pCont->IsAttr(ATTR_OWNED))
				return( true );

			// On corpse
			// ??? what if the container is locked ?
		}

		const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pObjTop);
		if ( pCorpseItem )
		{
			// Taking stuff off someones corpse can be a crime !
			return( const_cast <CChar*>(this)->CheckCorpseCrime( pCorpseItem, true, true ));
		}

		return( false );	// i guess it's not a crime.
	}

	if ( pCharMark->NPC_IsOwnedBy( this ))	// He let's you
		return( false );

	// Pack animal has no owner ?
	if ( pCharMark->GetNPCBrain() == NPCBRAIN_ANIMAL &&	// free to take.
		! pCharMark->IsStatFlag( STATF_Pet ) &&
		! pCharMark->m_pPlayer )
	{
		return( false );
	}

	return( true );
}

bool CChar::CanUse( CItem * pItem, bool fMoveOrConsume ) const
{
	ADDTOCALLSTACK("CChar::CanUse");
	// Can the Char use ( CONSUME )  the item where it is ?
	// NOTE: Although this implies that we pick it up and use it.

	if ( ! CanTouch(pItem) )
		return( false );

	if ( fMoveOrConsume )
	{
		if ( ! CanMove(pItem))
			return( false );	// ? using does not imply moving in all cases ! such as reading ?
		if ( IsTakeCrime( pItem ))
			return( false );
	}
	else
	{
		// Just snooping i guess.
		if ( pItem->IsTopLevel())
			return( true );
		// The item is on another character ?
		const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop != ( static_cast <const CObjBaseTemplate *>(this)))
		{
			if ( IsPriv( PRIV_GM ))
				return( true );
			if ( pItem->IsType(IT_CONTAINER))
				return( true );
			if ( pItem->IsType(IT_BOOK))
				return( true );
			return( false );
		}
	}

	return( true );
}

CRegionBase * CChar::CheckValidMove( CPointBase & ptDest, WORD * pwBlockFlags, DIR_TYPE dir ) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove");
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( IsSetEF( EF_DiagonalWalkCheck ) && GetTopPoint().IsValidPoint() && (dir % 2) )
	{
		CPointMap	ptTest;
		DIR_TYPE	dirTest1	= (DIR_TYPE) (dir-1);	// get 1st ortogonal
		DIR_TYPE	dirTest2	= (DIR_TYPE) (dir+1);	// get 2nd ortogonal
		if ( dirTest2 == DIR_QTY )		// roll over
			dirTest2	= (DIR_TYPE) 0;

		ptTest		= GetTopPoint();
		ptTest.Move( dirTest1 );
		if ( !CheckValidMove( ptTest, pwBlockFlags ) )
				return NULL;

		ptTest		= GetTopPoint();
		ptTest.Move( dirTest2 );
		if ( !CheckValidMove( ptTest, pwBlockFlags ) )
				return NULL;
	}

	CRegionBase * pArea = ptDest.GetRegion( REGION_TYPE_MULTI | REGION_TYPE_AREA );
	if ( pArea == NULL )
	{
		// Only report the missing region if the destination is within the map boundaries.
		if ( ptDest.IsValidPoint() )
			g_Log.Event( LOGL_WARN, "MOVE: UID(0%x) is moving into a non-defined region (Point: %d,%d,%d,%d)\n", (DWORD)GetUID(), ptDest.m_x, ptDest.m_y, ptDest.m_z, ptDest.m_map );
		return( NULL );
	}

	// In theory the client only lets us go valid places.
	// But in reality the client and server are not really in sync.
	// if ( IsClient()) return( pArea );

	WORD wCan = GetMoveBlockFlags();

	//	Blocking dynamic items
	if (( !IsPriv(PRIV_GM) ) && ( !IsPriv(PRIV_ALLMOVE) ))
	{
		CWorldSearch AreaItems(ptDest);
		while (true)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == NULL )
				break;

			CItemBase * pItemDef = pItem->Item_GetDef();
			if ( (pItem->GetTopZ() <= ptDest.m_z + PLAYER_HEIGHT) && (pItem->GetTopZ() >= ptDest.m_z) && (pItemDef->m_Can != 00) )
			{ // it IS in my way and HAS a flag set, check further
				if ( (pItemDef->m_Can & 0x08) && !(wCan & 0x08) )
				{ // item is blocking, and I cannot pass thru walls
					return (NULL);
				} else if ( (pItemDef->m_Can & 0x01) && !(wCan & 0x01) )
				{ // item is a door, and I'm no ghost
					return (NULL);
				} else if ( (pItemDef->m_Can & 0x02) && !(wCan & 0x02) )
				{ // item is water, and I cannot swim
					return (NULL);
				} else if ( (pItemDef->m_Can & 0x04) && !(wCan & 0x04) )
				{ // item is walkable, but I cannot walk at all!
					return (NULL);
				} else if ( (pItemDef->m_Can & 0x020) && !(wCan & 0x020) && (m_pPlayer == NULL) )
				{ // item is fire, and I'm not immune against this
				  // should only work for NPC, they are way too smart to do sth so silly
					return (NULL);
				} else if ( (pItemDef->m_Can & 0x040) && !(wCan & 0x040) )
				{ // item is under a roof, and I'm too large to walk there
					return (NULL);
				}
				// in all other cases I may pass
			}
		}
	}

	WORD wBlockFlags = wCan;
	signed char z = g_World.GetHeightPoint( ptDest, wBlockFlags, true );
	if ( wCan != 0xFFFF )
	{
		if ( wBlockFlags &~ wCan )
		{
			return( NULL );
		}

		CCharBase* pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		if ( ! pCharDef->Can( CAN_C_FLY ))
		{
			if ( z > ptDest.m_z + PLAYER_HEIGHT )	// Too high to climb.
			{
				return( NULL );
			}
		}
		if ( z >= UO_SIZE_Z )
		{
			return( NULL );
		}
	}

	if ( pwBlockFlags )
	{
		*pwBlockFlags = wBlockFlags;
	}

	ptDest.m_z = z;
	return( pArea );

}

bool CChar::IsVerticalSpace( CPointMap ptDest, bool fForceMount )
{
	ADDTOCALLSTACK("CChar::IsVerticalSpace");
	if ( IsPriv( PRIV_GM ) || IsPriv( PRIV_ALLMOVE ) || !( ptDest.IsValidPoint() ))
		return true;

	WORD wBlockFlags = GetMoveBlockFlags();
	if ( wBlockFlags & CAN_C_WALK )
		wBlockFlags |= CAN_I_CLIMB;

	CGrayMapBlockState block( wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount( false ), ptDest.m_z + m_zClimbHeight + 2, GetHeightMount( false ) );
	g_World.GetHeightPoint_New( ptDest, block, true );

	//DEBUG_ERR(("GetHeightMount( false ) %d  ptDest.m_z %d  block.m_Top.m_z %d\n",GetHeightMount( false )+4,ptDest.m_z,block.m_Top.m_z));
	if ( GetHeightMount( false ) + ptDest.m_z + (( fForceMount ) ? ( 4 ) : ( 0 )) >= block.m_Top.m_z ) //4 is the height of a rideable
		return false;
	return true;
}

CRegionBase * CChar::CheckValidMove_New( CPointBase & ptDest, WORD * pwBlockFlags, DIR_TYPE dir, signed char * pClimbHeight, bool fPathFinding ) const
{
	ADDTOCALLSTACK("CChar::CheckValidMove_New");
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( IsSetEF( EF_DiagonalWalkCheck ) && GetTopPoint().IsValidPoint() && !fPathFinding && (dir % 2) )
	{
		CPointMap	ptTest;
		DIR_TYPE	dirTest1	= (DIR_TYPE) (dir-1);	// get 1st ortogonal
		DIR_TYPE	dirTest2	= (DIR_TYPE) (dir+1);	// get 2nd ortogonal
		if ( dirTest2 == DIR_QTY )		// roll over
			dirTest2	= (DIR_TYPE) 0;

		ptTest		= GetTopPoint();
		ptTest.Move( dirTest1 );
		if ( !CheckValidMove_New( ptTest, pwBlockFlags, DIR_QTY, pClimbHeight ) )
				return NULL;

		ptTest		= GetTopPoint();
		ptTest.Move( dirTest2 );
		if ( !CheckValidMove_New( ptTest, pwBlockFlags, DIR_QTY, pClimbHeight ) )
				return NULL;

	}
	CRegionBase * pArea = ptDest.GetRegion( REGION_TYPE_MULTI | REGION_TYPE_AREA );
	if ( pArea == NULL )
	{
		WARNWALK(("Failed to get region\n"));
		return( NULL );
	}

	WORD wCan = GetMoveBlockFlags();
	WARNWALK(("GetMoveBlockFlags() (0x%x)\n",wCan));

	WORD wBlockFlags = wCan;

	if ( wCan & CAN_C_WALK )
	{
		wBlockFlags |= CAN_I_CLIMB; // If we can walk than we can climb. Ignore CAN_C_FLY at all here
		WARNWALK(("wBlockFlags (0%x) wCan(0%x)\n",wBlockFlags,wCan));
	}
	//ptDest.m_z += m_zClimbHeight; // If we are climbing than check z at the top of stairs
	//CGrayMapBlockState block( wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + PLAYER_HEIGHT + 3, ptDest.m_z + m_zClimbHeight + 2 );
	//WARNWALK(("\t\tCGrayMapBlockState block( 0%x, %d, %d, %d );ptDest.m_z(%d) m_zClimbHeight(%d)\n",wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + PLAYER_HEIGHT + 3, ptDest.m_z + m_zClimbHeight + 2,ptDest.m_z,m_zClimbHeight));
	CGrayMapBlockState block( wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount( false ), ptDest.m_z + m_zClimbHeight + 2, GetHeightMount( false ) );
	WARNWALK(("\t\tCGrayMapBlockState block( 0%x, %d, %d, %d );ptDest.m_z(%d) m_zClimbHeight(%d)\n",wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + GetHeightMount( false ), ptDest.m_z + m_zClimbHeight + 2,ptDest.m_z,m_zClimbHeight));

	if ( ! ptDest.IsValidPoint() )
	{
		DEBUG_ERR(("Character 0%x on %d,%d,%d wants to move into an invalid location %d,%d,%d.\n",GetUID().GetObjUID(),GetTopPoint().m_x,GetTopPoint().m_y,GetTopPoint().m_z,ptDest.m_x,ptDest.m_y,ptDest.m_z));
		return NULL;
	}
	g_World.GetHeightPoint_New( ptDest, block, true );

	// Pass along my results.
	wBlockFlags = block.m_Bottom.m_dwBlockFlags;

	if ( block.m_Top.m_dwBlockFlags )
	{
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.
		//WARNWALK(("block.m_Top.m_z (%d) > ptDest.m_z (%d) - m_zClimbHeight (%d) + (block.m_Top.m_wTile (0x%x) > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 )(%d)\n",block.m_Top.m_z,ptDest.m_z,m_zClimbHeight,block.m_Top.m_wTile,ptDest.m_z - m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 )));
		//if ( block.m_Top.m_z < ptDest.m_z - m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 ) )
		WARNWALK(("block.m_Top.m_z (%d) > ptDest.m_z (%d) - m_zClimbHeight (%d) + (block.m_Top.m_wTile (0x%x) > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 )(%d)\n",block.m_Top.m_z,ptDest.m_z,m_zClimbHeight,block.m_Top.m_wTile,ptDest.m_z - (m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 ))));
		if ( block.m_Top.m_z < ptDest.m_z - (m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? GetHeightMount( false ) : GetHeightMount( false )/2 )) )
			wBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
	}

	if (( wCan != 0xFFFF ) && ( wBlockFlags != 0x0 ))
	{
		WARNWALK(("BOTTOMitemID (0%x) TOPitemID (0%x)\n",(block.m_Bottom.m_wTile - TERRAIN_QTY),(block.m_Top.m_wTile - TERRAIN_QTY)));
		CCharBase* pCharDef = Char_GetDef();
		ASSERT(pCharDef);

		if ( ( wBlockFlags & CAN_I_DOOR ) && ! pCharDef->Can( CAN_C_GHOST ))
		{
			wBlockFlags |= CAN_I_BLOCK;
		}
		if ( ( wBlockFlags & CAN_I_WATER ) && ! pCharDef->Can( CAN_C_SWIM ))
		{
			wBlockFlags |= CAN_I_BLOCK;
		}
		if ( ( wBlockFlags & CAN_I_ROOF ) && ! pCharDef->Can( CAN_C_INDOORS ))
		{
			wBlockFlags |= CAN_I_BLOCK;
		}

		if ( ( wBlockFlags & CAN_I_DOOR ) && pCharDef->Can( CAN_C_GHOST ))
		{
			wBlockFlags &= ~CAN_I_BLOCK;
		}
		if ( ( wBlockFlags & CAN_I_WATER ) && pCharDef->Can( CAN_C_SWIM ))
		{
			wBlockFlags &= ~CAN_I_BLOCK;
		}
		if ( ( wBlockFlags & CAN_I_PLATFORM ) && pCharDef->Can( CAN_C_WALK ))
		{
			wBlockFlags &= ~CAN_I_BLOCK;
		}
		if ( ( wBlockFlags & CAN_I_ROOF ) && pCharDef->Can( CAN_C_INDOORS ))
		{
			wBlockFlags &= ~CAN_I_BLOCK;
		}

		//DEBUG_ERR(("wBlockFlags (0%x) pCharDef->m_Can (0%x)\n",wBlockFlags,pCharDef->m_Can));
		if ( ! pCharDef->Can( CAN_C_FLY ))
		{
			if ( ! ( wBlockFlags & CAN_I_CLIMB ) ) // we can climb anywhere
			{
				WARNWALK(("block.m_Lowest.m_z %d  block.m_Bottom.m_z %d  block.m_Top.m_z %d\n",block.m_Lowest.m_z,block.m_Bottom.m_z,block.m_Top.m_z));
				if ( block.m_Bottom.m_wTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + 2) // Too high to climb.
						return( NULL );
				}
				else if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + GetHeightMount( false ) + 3)
						return( NULL );
			}
		}

		//DEBUG_ERR(("wBlockFlags (0%x) pCharDef->Can( CAN_C_PASSWALLS ) (%d)\n",wBlockFlags,pCharDef->Can( CAN_C_PASSWALLS )));
		if (( wBlockFlags & CAN_I_BLOCK ) && ( ! pCharDef->Can( CAN_C_PASSWALLS )) )
				return NULL;

		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return( NULL );
	}
	WARNWALK(("GetHeightMount( false ) %d  block.m_Top.m_z  %d ptDest.m_z  %d\n",GetHeightMount( false ),block.m_Top.m_z,ptDest.m_z));
	if (( GetHeightMount( false ) + ptDest.m_z >= block.m_Top.m_z ) && ( g_Cfg.m_iMountHeight ) && ( !IsPriv( PRIV_GM ) ) && ( !IsPriv( PRIV_ALLMOVE ) ))
	{
		SysMessageDefault( DEFMSG_MOUNT_CEILING );
		return NULL;
	}

	if ( pwBlockFlags )
	{
		*pwBlockFlags = wBlockFlags;
	}
	if ( ( wBlockFlags & CAN_I_CLIMB ) && ( pClimbHeight ) )
		*pClimbHeight = block.m_zClimbHeight;


	ptDest.m_z = block.m_Bottom.m_z;
	return( pArea );
}

void CChar::FixClimbHeight()
{
	ADDTOCALLSTACK("CChar::FixClimbHeight");
	if ( IsSetEF( EF_WalkCheck ) ) // get correct climb height (or could be stacked)
	{
		CPointBase pt = GetTopPoint();
		CGrayMapBlockState block( CAN_I_CLIMB, pt.m_z , pt.m_z + GetHeightMount( false ) + 3, pt.m_z + 2, GetHeightMount( false ) );

		g_World.GetHeightPoint_New( pt, block, true );

		if ( ( block.m_Bottom.m_z == pt.m_z ) && ( block.m_dwBlockFlags & CAN_I_CLIMB ) ) // we are standing on stairs
			m_zClimbHeight = block.m_zClimbHeight;
		else
			m_zClimbHeight = 0;
	}
	m_fClimbUpdated = true;
}