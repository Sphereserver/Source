#include <math.h>
#include "graysvr.h"

bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount )
{
	return IsResourceMatch( rid, dwAmount, 0 );
}

bool CChar::IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwAmount, DWORD dwArgResearch )
{
	// Is the char a match for this test ?
	switch ( rid.GetResType())
	{
	case RES_SKILL:
		// Do i have this skill level at least ?
		// A min skill is required.
		if ( Skill_GetBase((SKILL_TYPE) rid.GetResIndex()) < dwAmount )
			return false;
		return true;

	case RES_CHARDEF:
		// Am i this type of char ?
		{
			CCharBase * pCharDef = Char_GetDef();
			if ( pCharDef->GetResourceID() == rid )
				return true;
		}
		break;
	case RES_SPEECH:	// do i have this speech ?
		if ( m_pNPC != NULL )
		{
			if ( m_pNPC->m_Speech.FindResourceID( rid ) >= 0 )
				return true;
			CCharBase * pCharDef = Char_GetDef();
			if ( pCharDef->m_Speech.FindResourceID( rid ) >= 0 )
				return true;
		}
		break;
	case RES_EVENTS:	// do i have these events ?
		{
			if ( m_OEvents.FindResourceID( rid ) >= 0 )
				return true;
			if ( m_pNPC != NULL )
			{
				CCharBase * pCharDef = Char_GetDef();
				if ( pCharDef->m_TEvents.FindResourceID( rid ) >= 0 )
					return true;
			}
		}
		break;
	case RES_TYPEDEF:
		// Do i have these in my posession ?
		if ( ! ContentConsume( rid, dwAmount, true, dwArgResearch ) )
		{
			return true;
		}
		return false;
	case RES_ITEMDEF:
		// Do i have these in my posession ?
		if ( ! ContentConsume( rid, dwAmount, true ) )
		{
			return true;
		}
		return false;

	default:
		// No idea.
		return false;
	}
	return false;
}

CItemContainer * CChar::GetBank( LAYER_TYPE layer )
{
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
			g_Log.Error("Junk in bank box layer %d!\n", layer);
			pItemTest->Delete();
		}

		// Give them a bank box if not already have one.
		pBankBox = dynamic_cast <CItemContainer *>( CItem::CreateScript(id, this));
		pBankBox->SetAttr(ATTR_NEWBIE | ATTR_MOVE_NEVER);
		LayerAdd( pBankBox, layer );
	}
	return( pBankBox );
}

CItem * CChar::LayerFind( LAYER_TYPE layer ) const
{
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
				return iRet;
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
			return iRet;
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
	// Get a percent of load.
	if ( IsPriv(PRIV_GM))
		return 1;
	int	MaxCarry = g_Cfg.Calc_MaxCarryWeight(this);
	if ( !MaxCarry ) return 1000;	// suppose self extra-overloaded
	return( IMULDIV( iWeight, 100, MaxCarry));
}

bool CChar::CanCarry( const CItem * pItem ) const
{
	if ( IsPriv(PRIV_GM))
		return true;

	int iMaxWeight = g_Cfg.Calc_MaxCarryWeight(this);

	// We are already carrying it ?
	const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	if ( this == pObjTop )
	{
		if ( GetTotalWeight() > iMaxWeight )
			return false;
	}
	else
	{
		int iWeight = pItem->GetWeight();
		if ( GetTotalWeight() + iWeight > iMaxWeight )
			return false;
	}

	return true;
}

LAYER_TYPE CChar::CanEquipLayer( CItem * pItem, LAYER_TYPE layer, CChar * pCharMsg, bool fTest )
{
	// This takes care of any conflicting items in the slot !
	// NOTE: Do not check to see if i can pick this up or steal this etc.
	// LAYER_NONE = can't equip this .

	CCharBase * pCharDef = Char_GetDef();
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
				SysMessagef( "Not strong enough to equip %s.", pItem->GetName());
				if ( pCharMsg != this )
				{
					pCharMsg->SysMessagef( "Not strong enough to equip %s.", pItem->GetName());
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
			goto cantequipthis;
		}
		break;
	case LAYER_BEARD:
		if ( ! pItem->IsType(IT_BEARD))
		{
			goto cantequipthis;
		}
		break;
	case LAYER_PACK:
		if ( ! pItem->IsType(IT_CONTAINER))
		{
			goto cantequipthis;
		}
		break;
	case LAYER_BANKBOX:
		if ( ! pItem->IsType(IT_EQ_BANK_BOX))
		{
			goto cantequipthis;
		}
		break;
	case LAYER_HORSE:
		// Only humans can ride horses !?
		if ( ! pItem->IsType(IT_EQ_HORSE) || ! IsHuman())
		{
			goto cantequipthis;
		}
		break;
	case LAYER_HAND1:
	case LAYER_HAND2:
		if ( ! pItemDef->IsTypeEquippable())
		{
			goto cantequipthis;
		}
		if ( ! pCharDef->Can( CAN_C_USEHANDS ))
		{
			goto cantequipthis;
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
cantequipthis:	// some creatures can equip certain special items ??? (orc lord?)
					if ( pCharMsg)
						pCharMsg->SysMessage("You can't equip this.");
					return LAYER_NONE;	// can't equip stuff.
				}
			}
		}
		break;
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
	// If they are standing on there own corpse then res the corpse !
	CWorldSearch Area(GetTopPoint(), iRadius);
	while ( CItem *pItem = Area.GetItem() )
	{
		if ( !pItem->IsType(IT_CORPSE) ) continue;

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
	int str = Stat_GetAdjusted(STAT_STR);
	if ( !str ) return 0;
	return IMULDIV(Stat_GetVal(STAT_STR), 100, str);
}

bool CChar::IsSwimming() const
{
	// Am i in/over/slightly under the water now ?
	// NOTE: This is a bit more complex because we need to test if we are slightly under water.

	CPointMap ptTop = GetTopPoint();

	CPointMap pt = g_World.FindItemTypeNearby( ptTop, IT_WATER );
	if ( ! pt.IsValidPoint())
		return false;	// no water here.

	int iDistZ = ptTop.m_z - pt.m_z;
	if ( iDistZ < -PLAYER_HEIGHT )	// far under the water somehow
		return false;
	if ( iDistZ <= 0 )	// in or below the water
		return true;

	// Is there a solid surface under us ?
	WORD wBlockFlags = GetMoveBlockFlags();
	if ( g_World.GetHeightPoint(ptTop, wBlockFlags, true) == pt.m_z )
		return true;

	return false;
}

NPCBRAIN_TYPE CChar::GetNPCBrain( bool fDefault ) const
{
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
	switch ( GetDispID())
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
			return( "he" );
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
			return( "she" );
		default:
			return( "it" );
	}
}

LPCTSTR CChar::GetPossessPronoun() const
{
	switch ( GetDispID())
	{
		case CREID_MAN:
		case CREID_GHOSTMAN:
		case CREID_ELFMAN:
		case CREID_ELFGHOSTMAN:
			return( "his" );
		case CREID_WOMAN:
		case CREID_GHOSTWOMAN:
		case CREID_ELFWOMAN:
		case CREID_ELFGHOSTWOMAN:
			return( "her" );
		default:
			return( "it's" );
	}
}

BYTE CChar::GetModeFlag( bool fTrueSight ) const
{
	BYTE mode = 0;
	if ( IsStatFlag( STATF_Poisoned ))
		mode |= CHARMODE_POISON;
	if ( IsStatFlag( STATF_War ))
		mode |= CHARMODE_WAR;
	if ( IsStatFlag(STATF_Freeze|STATF_Hallucinating|STATF_Stone) )
		mode |= CHARMODE_YELLOW;
	if ( ! fTrueSight && IsStatFlag(STATF_Insubstantial|STATF_Invisible|STATF_Hidden))	// if not me, this will not show up !
		mode |= CHARMODE_INVIS;
	return( mode );
}

BYTE CChar::GetLightLevel() const
{
	// Get personal light level.

	if ( IsStatFlag( STATF_DEAD ) || IsPriv(PRIV_DEBUG))	// dead don't need light.
		return( LIGHT_BRIGHT + 1 );
	if ( IsStatFlag( STATF_NightSight ))
		return( LIGHT_BRIGHT );
	return( GetTopSector()->GetLight());
}

CItem * CChar::GetSpellbook( SPELL_TYPE iSpell ) const
{
	CItem	*pBook = NULL;
	int		i = iSpell;

	//	search for suitable book in hands first
	pBook = GetContentHead();
	for ( ; pBook != NULL; pBook = pBook->GetNext() )
	{
		if ( pBook->IsTypeSpellbook() )
		{
			if (( i > pBook->m_itSpellbook.m_baseid ) || ( i - ( pBook->m_itSpellbook.m_baseid + 1 ) < 96 ))
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
				if (( i > pBook->m_itSpellbook.m_baseid ) || ( i - ( pBook->m_itSpellbook.m_baseid + 1 ) < 96 ))
					return pBook;
			}
		}
	}

	return NULL;
}

int CChar::Food_GetLevelPercent() const
{
	int	max	= Stat_GetMax(STAT_FOOD);
	if ( max == 0 )
		return 100;
	else
		return IMULDIV( Stat_GetVal(STAT_FOOD), 100, max );
}

LPCTSTR CChar::Food_GetLevelMessage( bool fPet, bool fHappy ) const
{
	int	max	= Stat_GetMax(STAT_FOOD);
	if ( max == 0)
		return "unaffected by hunger";
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
	int iRet = pCharDef->m_FoodType.FindResourceMatch( pObj );
	if ( iRet >= 0 )
	{
		return( pCharDef->m_FoodType[iRet].GetResQty()); // how bad do i want it ?
	}

	// ???
	return 0;
}

LPCTSTR CChar::GetTradeTitle() const // Paperdoll title for character p (2)
{
	static const CValStr sm_SkillTitles[] =
	{
		"", INT_MIN,
		"Neophyte", 300,
		"Novice", 400,
		"Apprentice", 500,
		"Journeyman", 600,
		"Expert", 700,
		"Adept", 800,
		"Master", 900,
		"Grandmaster", 980,
		NULL, INT_MAX,
	};

	if ( ! m_sTitle.IsEmpty())
		return( m_sTitle );

	TEMPSTRING(pTemp);
	CCharBase* pCharDef = Char_GetDef();

	// Incognito ?
	// If polymorphed then use the poly name.
	if ( IsStatFlag( STATF_Incognito ) ||
		! IsHuman() ||
		( m_pNPC && pCharDef->GetTypeName() != pCharDef->GetTradeName()))
	{
		if ( ! IsIndividualName())
			return( "" );	// same as type anyhow.
		sprintf( pTemp, "the %s", pCharDef->GetTradeName());
		return( pTemp );
	}

	SKILL_TYPE skill = Skill_GetBest();
	int len = sprintf( pTemp, "%s ", sm_SkillTitles->FindName( Skill_GetBase(skill)));
	sprintf( pTemp+len, g_Cfg.GetSkillDef(skill)->m_sTitle, (pCharDef->IsFemale()) ? "woman" : "man" );
	return( pTemp );
}

bool CChar::CanDisturb(CChar *pChar) const
{
	//	we cannot disturb more privileged chars with hidden status
	if ( pChar->IsStatFlag(STATF_Insubstantial) && ( GetPrivLevel() < pChar->GetPrivLevel() ))
		return false;

	return true;
}

bool CChar::CanSeeTrue(const CChar *pChar) const
{
	if ( pChar == this )
		return false;

	return ( pChar->GetPrivLevel() < GetPrivLevel() );
}

bool CChar::CanSeeInContainer( const CItemContainer * pContItem ) const
{
	// This is a container of some sort. Can i see inside it ?

	if ( pContItem == NULL )	// must be a CChar ?
		return true;

	if ( pContItem->IsSearchable())	// not a bank box or locked box.
		return true;

	if ((LAYER_TYPE)pContItem->GetContainedLayer()==LAYER_VENDOR_STOCK)
		return ( true );

	// Not normally searchable.
	// Make some special cases for searchable.

	const CChar * pChar = dynamic_cast <const CChar*> (pContItem->GetTopLevelObj());
	if ( pChar == NULL )
	{
		if ( IsPriv(PRIV_GM))
			return true;
		return false;
	}

	if ( pContItem->IsType(IT_EQ_TRADE_WINDOW) )
	{
		// Trade windows should be viewable by both trade participants
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
		return false;

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
	// Can I see this object ( char or item ) ?
	if ( !pObj || /*pObj->*/IsDisconnected() )
		return false;

	//	first check the distance since if this will fail, we do not need to scan all
	//	subcontainers to find this result ;)
	if ( pObj->GetTopLevelObj()->GetTopPoint().GetDist(GetTopPoint()) > pObj->GetVisualRange() )
		return false;

	if ( pObj->IsItem() )
	{
		const CItem * pItem = static_cast <const CItem*>(pObj);
		if ( !pItem || !CanSeeItem(pItem) )
			return false;
		CObjBase * pObjCont = pItem->GetContainer();
		if ( pObjCont )
		{
			if ( !CanSeeInContainer( dynamic_cast <const CItemContainer*>(pObjCont) ))
				return false;
			return( CanSee( pObjCont ));
		}
	}
	else
	{
		const CChar * pChar = static_cast <const CChar*>(pObj);
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
						return false;
				}
				else
				{
					if ( GetPrivLevel() < pChar->GetPrivLevel())
						return false;
				}
			}
			return true;
		}

		if ( pChar->IsStatFlag(STATF_DEAD) && m_pNPC )
		{
			if ( !(   m_pNPC->m_Brain == NPCBRAIN_HEALER || Skill_GetBase( SKILL_SPIRITSPEAK ) >= 1000 ) )
				return false;
		}
		else if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ))
		{
			// Characters can be invisible, but not to GM's (true sight ?)
			// equal level can see each other
			if ( pChar->GetPrivLevel() <= PLEVEL_Player )
			{
				if ( GetPrivLevel() <= pChar->GetPrivLevel())
					return false;
			}
			else
			{
				if ( GetPrivLevel() < pChar->GetPrivLevel())
					return false;
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
			if ( !IsPriv(PRIV_GM) )
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

bool CChar::CanSeeLOS( const CPointMap & ptDst, CPointMap * pptBlock, int iMaxDist ) const
{
	CPointMap ptSrc = GetTopPoint();

	if ( IsPriv(PRIV_GM) ) return true;

	if ( pptBlock != NULL )
	{
		*pptBlock = ptSrc;
	}

	if ( m_pPlayer && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_PLAYER) )
	{
		return CanSeeLOS_New(ptDst, pptBlock, iMaxDist);
	}
	if ( m_pNPC && (g_Cfg.m_iAdvancedLos & ADVANCEDLOS_NPC) )
	{
		return CanSeeLOS_New(ptDst, pptBlock, iMaxDist);
	}
	
	// Max distance of iMaxDist
	// Line of sight check
	// ToDo: move repeated blocks to a separate functions?
	// 3D LOS later - real LOS, i.e. we can't shoot through the floor, but can shoot through the hole in it

	int iDist = ptSrc.GetDist(ptDst);
	if ( iDist > iMaxDist ) return false;

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
					if ( zDiff > PLAYER_HEIGHT ) return false;
					else ptTest.m_z	= z;

					if ( wBlockFlags & ( CAN_I_BLOCK | CAN_I_DOOR ))
					{
						ptSrc = ptTest;
						return false;
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

			if ( zDiff > PLAYER_HEIGHT ) return false;
			else ptSrc.m_z	= z;

			if ( wBlockFlags & (CAN_I_BLOCK|CAN_I_DOOR) ) return false;
			if ( iDistTry > iMaxDist ) return false;
			iDistTry++;
		}
	}

	if ( abs( ptSrc.m_z - ptDst.m_z ) > PLAYER_HEIGHT ) return false;
	return true; // made it all the way to the object with no obstructions.
}

// a - gradient < x < b + gradient
#define BETWEENPOINT(coord, coordt, coords) ( ((coord) > (((double)min((coordt),(coords)))-0.5)) && ((coord) < (((double)max((coordt),(coords)))+0.5)) )
#define APPROX(num) ((double)(((num)-floor(num))>0.5)?(ceil(num)):(floor(num)))
#define CALCITEMHEIGHT(num) (num)+((pItemDef->GetTFlags()&0x400)?(pItemDef->GetHeight()/2):pItemDef->GetHeight())
// #define DEBUGCANSEELOS 1
#ifdef DEBUGCANSEELOS
	#define WARNLOS(_x_)		g_pLog->EventWarn _x_
#else
	#define WARNLOS(_x_)
#endif

bool CChar::CanSeeLOS_New( const CPointMap & ptDst, CPointMap * pptBlock, int iMaxDist ) const
{
	CPointMap ptSrc = GetTopPoint();
	CPointMap ptNow(ptSrc);
	
	if ( ptSrc.m_map != ptDst.m_map ) return false;
	if ( ptSrc == ptDst ) return true;

	if ( IsHuman() )
	{
		WARNLOS(("LOS: Human\n"));
		if ( IsStatFlag(STATF_OnHorse) )
		{
			WARNLOS(("LOS: Human Mounted\n"));
			ptSrc.m_z += (PLAYER_HEIGHT + 7);	// Mounted player?
		}
		else
		{
			ptSrc.m_z += (PLAYER_HEIGHT - 3);	// Eye height
		}
	}
	else // TODO: Well i have no clue for determining height of Npcs/Morphed players. Adding a def for npcs??
	{
		WARNLOS(("LOS: Random (min: 2)\n"));
		ptSrc.m_z += (Calc_GetRandVal(PLAYER_HEIGHT) + 2);
	}
	
	double dist2d, dist3d;
	long dx, dy, dz;
	
	dx = ptDst.m_x - ptSrc.m_x;
	dy = ptDst.m_y - ptSrc.m_y;
	dz = ptDst.m_z - ptSrc.m_z;
	
	dist2d = sqrt(pow((double)dx,2)+pow((double)dy,2));
	if (dz)
		dist3d = sqrt(pow((double)dist2d,2)+pow((double)dz,2));
	else
		dist3d = dist2d;
	
	if ( APPROX(dist2d) > ((double)iMaxDist) ) return false;
	
	double dFactorX, dFactorY, dFactorZ;
	
	dFactorX = dx / dist3d;
	dFactorY = dy / dist3d;
	dFactorZ = dz / dist3d;
	
	double nPx, nPy, nPz;
	nPx = ptSrc.m_x; nPy = ptSrc.m_y; nPz = ptSrc.m_z;
	
	CGTypedArray<CPointMap,CPointMap> path;
	
	while ( true )
	{
		if ( BETWEENPOINT(nPx,ptDst.m_x,ptSrc.m_x) && BETWEENPOINT(nPy,ptDst.m_y,ptSrc.m_y) && BETWEENPOINT(nPz,ptDst.m_z,ptSrc.m_z) )
		{
			dx = (int)APPROX(nPx);
			dy = (int)APPROX(nPy);
			dz = (int)APPROX(nPz);

			// Add point to vector
			if ( path.GetCount() )
			{
				CPointMap ptEnd = path.GetAt(path.GetCount()-1);
				if ( ptEnd.m_x != dx || ptEnd.m_y != dy || ptEnd.m_z != dz )
					path.Add(CPointMap(dx,dy,dz,ptSrc.m_map));
			}
			else
			{
				path.Add(CPointMap(dx,dy,dz,ptSrc.m_map));
			}

			nPx += dFactorX;
			nPy += dFactorY;
			nPz += dFactorZ;
		}
		else
			break;
	}
	
	if ( !path.GetCount() ) return false;

	if ( path.GetAt(path.GetCount()-1) != ptDst ) 
	{
		path.Add(CPointMap(ptDst.m_x,ptDst.m_y,ptDst.m_z,ptDst.m_map));
	}
	
	WARNLOS(("Path calculated %d\n", path.GetCount()));
	// Ok now we should loop through all the points and checking for maptile, staticx, items, multis.
	// If something is in the way and it has the wrong flags LOS return false
	
	const CGrayMapBlock * pBlock		= NULL;		// Block of the map (for statics)
	// const CUOMapMeter * pMeter 		= NULL;		// Block of the map (for map tile)
	const CUOStaticItemRec * pStatic	= NULL;		// Statics iterator (based on GrayMapBlock)
	const CGrayMulti * pMulti 		= NULL;		// Multi Def (multi check)
	const CUOMultiItemRec * pMultiItem 	= NULL;		// Multi item iterator
	CItemBase * pItemDef 			= NULL;
	bool bPath = true;
	
	int lp_x = 0; int lp_y = 0; 
	signed char min_z = 0; signed char max_z = 0;
	
	for(int i = 0; i < path.GetCount(); lp_x = ptNow.m_x, lp_y = ptNow.m_y, pItemDef = NULL, pStatic = NULL, pMulti = NULL, pMultiItem = NULL, min_z = 0, max_z = 0, i++)
	{
		ptNow = path.GetAt(i);
		WARNLOS(("Point %d,%d,%d \n", ptNow.m_x,ptNow.m_y,ptNow.m_z));

		if ( lp_x != ptNow.m_x || lp_y != ptNow.m_y )
		{
			WARNLOS(("\tLoading new map block.\n"));
			pBlock = g_World.GetMapBlock( ptNow );
		}

		if ( !pBlock ) // something is wrong
		{
			WARNLOS(("GetMapBlock Failed\n"));
			bPath = false;
			break;
		}

		// ------ MapX.mul Check ----------
		
		WORD terrainid = pBlock->GetTerrain(0,0)->m_wTerrainIndex;
		WARNLOS(("Terrain %d\n", terrainid));

		#define MAPTILEMIN min(min(min(pBlock->GetTerrain(0,0)->m_z,pBlock->GetTerrain(0,1)->m_z),pBlock->GetTerrain(1,0)->m_z),pBlock->GetTerrain(1,1)->m_z)
		#define MAPTILEMAX max(max(max(pBlock->GetTerrain(0,0)->m_z,pBlock->GetTerrain(0,1)->m_z),pBlock->GetTerrain(1,0)->m_z),pBlock->GetTerrain(1,1)->m_z)

		if ( terrainid != TERRAIN_HOLE && terrainid != 475 )
		{
			if ( terrainid < 430 || terrainid > 437 )
			{
				min_z = MAPTILEMIN;
				max_z = MAPTILEMAX;
				WARNLOS(("Terrain %d - m:%d M:%d\n", terrainid, min_z, max_z));
				if (min_z <= ptNow.m_z && max_z >= ptNow.m_z && (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y  || min_z > ptDst.m_z || max_z < ptDst.m_z))
				{
					WARNLOS(("Terrain %d - m:%d M:%d - block\n", terrainid, min_z, max_z));
					bPath = false;
					break;
				}
			}
		}

		#undef MAPTILEMIN
		#undef MAPTILEMAX

		// ----------------------------------

		// ------ StaticsX.mul Check --------
		
		for ( int s = 0; s < pBlock->m_Statics.GetStaticQty(); pStatic = NULL, pItemDef = NULL, s++ )
		{
			pStatic = pBlock->m_Statics.GetStatic(s);
			if (pStatic->m_x+pBlock->m_x != ptNow.m_x || pStatic->m_y+pBlock->m_y != ptNow.m_y)
				continue;

			pItemDef = CItemBase::FindItemBase( pStatic->GetDispID() );
			if ( !pItemDef )
				continue;

			if ( pItemDef->GetTFlags() & 0x3000 )
			{
				WARNLOS(("pStatic %0x %d,%d,%d - %d\n",pStatic->GetDispID(),pStatic->m_x,pStatic->m_y,pStatic->m_z,pItemDef->GetHeight()));
				min_z = pStatic->m_z;
				max_z = CALCITEMHEIGHT(min_z);

				WARNLOS(("pStatic %0x Z check: %d,%d (Now: %d) (Dest: %d).\n",pStatic->GetDispID(),min_z,max_z,ptNow.m_z,ptDst.m_z));
				if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
				{
					if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
					{
						WARNLOS(("pStatic blocked - m:%d M:%d\n", min_z,max_z));
						bPath = false;
						break;
					}
				}
			}
		}
		
		if ( !bPath )
			break;
	
		// ----------------------------------
		
		// --------- In game items ----------
		
		CWorldSearch AreaItems(ptNow, 0);
		while( CItem *pItem = AreaItems.GetItem() )
		{
			pItemDef = NULL;

			if (pItem->GetUnkPoint().m_x != ptNow.m_x || pItem->GetUnkPoint().m_y != ptNow.m_y)
				continue;
			if ( !CanSeeItem(pItem) )
				continue;
			pItemDef = pItem->Item_GetDef();
			if ( !pItemDef )
				continue;
			if ( pItemDef->GetTFlags() & 0x3000 )
			{
				WARNLOS(("pItem %0x(%0x) %d,%d,%d - %d\n",pItem->GetUID(),pItem->GetDispID(),pItem->GetUnkPoint().m_x,pItem->GetUnkPoint().m_y,pItem->GetUnkPoint().m_z,pItemDef->GetHeight()));
				min_z = pItem->GetUnkPoint().m_z;
				max_z = CALCITEMHEIGHT(min_z);

				WARNLOS(("pItem %0x(%0x) Z check: %d,%d (Now: %d) (Dest: %d).\n",pItem->GetUID(),pItem->GetDispID(),min_z,max_z,ptNow.m_z,ptDst.m_z));
				if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
				{
					if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
					{
						WARNLOS(("pItem blocked - m:%d M:%d\n", min_z,max_z));	
						bPath = false;
						break;
					}
				}
			}
		}

		if ( !bPath )
			break;

		// ----------------------------------
		
		// ----------- Multis ---------------
		
		// TODO: I think that this part should be optimized

		CRegionLinks rlinks;
		int iQtyr = ptNow.GetRegions( REGION_TYPE_MULTI, rlinks );
		if ( iQtyr )
		{
			for ( int i = 0; i < iQtyr; pMulti = NULL, i++)
			{
				CRegionBase * pRegion = rlinks.GetAt(i);
				CItem * pItem = pRegion->GetResourceID().ItemFind();
				if ( !pItem )
					continue;

				pMulti = g_Cfg.GetMultiItemDefs( pItem->GetDispID() );
				if ( !pMulti )
					continue;

				int iQty = pMulti->GetItemCount();
				for ( int i=0; iQty--; pItemDef = NULL, pMultiItem = NULL, i++ )
				{
					pMultiItem = pMulti->GetItem(i);
					
					if ( !pMultiItem )
						break;

					if ( !pMultiItem->m_visible )
						continue;

					if ( ( pMultiItem->m_dx + pItem->GetTopPoint().m_x != ptNow.m_x ) || ( pMultiItem->m_dy + pItem->GetTopPoint().m_y != ptNow.m_y ) )
						continue;
						
					pItemDef = CItemBase::FindItemBase( pMultiItem->GetDispID() );
					
					if ( !pItemDef )
						continue;
						
					if ( pItemDef->GetTFlags() & 0x3000 )
					{
						min_z = pMultiItem->m_dz + pItem->GetTopPoint().m_z;
						max_z = CALCITEMHEIGHT(min_z);

						if (min_z <= ptNow.m_z && max_z >= ptNow.m_z)
						{
							if (ptNow.m_x != ptDst.m_x || ptNow.m_y != ptDst.m_y || min_z > ptDst.m_z || max_z < ptDst.m_z)
							{
								WARNLOS(("pStatic blocked - m:%d M:%d\n", min_z,max_z));	
								bPath = false;
								break;
							}
						}
					}
				}
				
				if ( !bPath )
					break;
			}
		}

		if ( !bPath )
			break;
			
		// ----------------------------------
	}
	return bPath;
}

#ifdef DEBUGCANSEELOS
	#undef WARNLOS
	#undef DEBUGCANSEELOS
#endif
#undef APPROX
#undef BETWEENPOINT
#undef CALCITEMHEIGHT

bool CChar::CanSeeLOS( const CObjBaseTemplate * pObj ) const
{
	if ( ! CanSee( pObj ))
		return false;
	pObj = pObj->GetTopLevelObj();
	return( CanSeeLOS( pObj->GetTopPoint(), NULL, pObj->GetVisualRange()));
}

bool CChar::CanTouch( const CPointMap & pt ) const
{
	// Can I reach this from where i am.
	// swords, spears, arms length = x units.
	// Use or Grab.
	// Check for blocking objects.
	// It this in a container we can't get to ?

	return( CanSeeLOS( pt, NULL, 6 ));
}

bool CChar::CanTouch( const CObjBase * pObj ) const
{
	// Can I reach this from where i am. swords, spears, arms length = x units.
	// Use or Grab. May be in snooped container.
	// Check for blocking objects. Is this in a container we can't get to ?

	if ( !pObj )
		return false;

	const CObjBaseTemplate	*pObjTop = pObj->GetTopLevelObj();
	int iDist = GetTopDist3D(pObjTop);

	bool fDeathImmune = IsPriv(PRIV_GM);
	if ( pObj->IsItem() )	// some objects can be used anytime. (even by the dead.)
	{
		const CItem * pItem = dynamic_cast <const CItem*> (pObj);
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
				if ( IsStatFlag(STATF_Freeze|STATF_Stone) )
					break;
				return( GetTopDist3D( pItem->GetTopLevelObj()) <= UO_MAP_VIEW_SIZE );
		}
	}

	if ( !fDeathImmune )
		if ( IsStatFlag(STATF_DEAD|STATF_Freeze|STATF_Stone) )
			return false;

	//	search up to top level object
	if ( pObjTop != this )
	{
		if ( pObjTop->IsChar() )
		{
			const CChar * pChar = dynamic_cast <const CChar*> (pObjTop);
			if ( !pChar )
				return false;

			if ( pChar == this )
				return true;
			if ( IsPriv(PRIV_GM) )
				return GetPrivLevel() >= pChar->GetPrivLevel();
			if ( pChar->IsStatFlag(STATF_DEAD|STATF_Stone) )
				return false;
		}

		while (true)
		{
			const CItem * pItem = dynamic_cast <const CItem*>(pObj);
			if ( !pItem )
				break;

			// What is this inside of ?
			CObjBase * pObjCont = pItem->GetContainer();
			if ( !pObjCont )
				break;

			pObj = pObjCont;
			if ( !CanSeeInContainer(dynamic_cast <const CItemContainer*>(pObj)) )
				return false;
		}
	}

	if ( IsPriv(PRIV_GM) )
		return true;
	if ( iDist > 3 )
		return false;

	return CanSeeLOS(pObjTop->GetTopPoint(), NULL, pObjTop->GetVisualRange());
}

IT_TYPE CChar::CanTouchStatic( CPointMap & pt, ITEMID_TYPE id, CItem * pItem )
{
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
	for ( int i=0; i < iQty; i++ )
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
	// can we hear text or sound. (not necessarily understand it (ghost))
	// Can't hear TALKMODE_SAY through house walls.
	// NOTE: Assume pClient->CanHear() has already been called. (if it matters)

	if ( pSrc == NULL )	// must be broadcast i guess.
		return true;
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
			return true;
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
		return false;

	if ( !IsSetOF(OF_HouseMuteSpeech) )
		return true;

	if ( fThrough )	// a yell goes through walls..
		return true;

	// sound can be blocked if in house.
	CRegionWorld * pSrcRegion;
	if ( pSrc->IsChar())
	{
		const CChar* pCharSrc = dynamic_cast <const CChar*> (pSrc);
		pSrcRegion = pCharSrc->GetRegion();
	}
	else
	{
		pSrcRegion = dynamic_cast <CRegionWorld *>( pSrc->GetTopPoint().GetRegion( REGION_TYPE_MULTI|REGION_TYPE_AREA ));
	}

	if ( m_pArea == pSrcRegion )	// same region is always ok.
		return true;
	if ( pSrcRegion == NULL || m_pArea == NULL ) // should not happen really.
		return false;
	if ( pSrcRegion->GetResourceID().IsItem() && ! pSrcRegion->IsFlag(REGION_FLAG_SHIP))
		return false;
	if ( m_pArea->GetResourceID().IsItem() && ! m_pArea->IsFlag(REGION_FLAG_SHIP))
		return false;

	return true;
}

bool CChar::CanMove( CItem * pItem, bool fMsg ) const
{
	// Is it possible that i could move this ?
	// NOT: test if need to steal. IsTakeCrime()
	// NOT: test if close enough. CanTouch()

	if ( IsPriv(PRIV_ALLMOVE|PRIV_DEBUG|PRIV_GM) )
		return true;

	if ( IsStatFlag(STATF_Stone|STATF_Freeze|STATF_Insubstantial|STATF_DEAD) )
	{
		if ( fMsg )
			SysMessagef("you can't reach anything in your state.");
		return false;
	}
	if ( !pItem )
		return false;

	if ( pItem->IsTopLevel() )
	{
		if ( pItem->IsTopLevelMultiLocked() )
		{
			if ( fMsg )
				SysMessage("It appears to be locked to the structure.");

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

		// Can't steal/move newbie items on another cchar. (even if pet)
		if ( pItem->IsAttr(ATTR_NEWBIE) )
		{
			const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
			if ( pObjTop->IsChar() && pObjTop != this )
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
	// We are snooping or stealing.
	// Is taking this item a crime ?
	// RETURN:
	//  ppCharMark = The character we are offending.
	//  false = no crime.

	if ( IsPriv(PRIV_GM|PRIV_ALLMOVE))
		return false;

	CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	CChar * pCharMark = dynamic_cast <CChar *> (pObjTop);
	if ( ppCharMark != NULL )
	{
		*ppCharMark = pCharMark;
	}

	if ( static_cast <const CObjBase *>(pObjTop) == this )
	{
		// this is yours
		return false;
	}

	if ( pCharMark == NULL )	// In some (or is) container.
	{
		if ( pItem->IsAttr(ATTR_OWNED) && pItem->m_uidLink != GetUID())
			return true;

		CItemContainer * pCont = dynamic_cast <CItemContainer *> (pObjTop);
		if (pCont)
		{
			if ( pCont->IsAttr(ATTR_OWNED))
				return true;

			// On corpse
			// ??? what if the container is locked ?
		}

		const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pObjTop);
		if ( pCorpseItem )
		{
			// Taking stuff off someones corpse can be a crime !
			return( const_cast <CChar*>(this)->CheckCorpseCrime( pCorpseItem, true, true ));
		}

		return false;	// i guess it's not a crime.
	}

	if ( pCharMark->NPC_IsOwnedBy( this ))	// He let's you
		return false;

	// Pack animal has no owner ?
	if ( pCharMark->GetNPCBrain() == NPCBRAIN_ANIMAL &&	// free to take.
		! pCharMark->IsStatFlag( STATF_Pet ) &&
		! pCharMark->m_pPlayer )
	{
		return false;
	}

	return true;
}

bool CChar::CanUse( CItem * pItem, bool fMoveOrConsume ) const
{
	// Can the Char use ( CONSUME )  the item where it is ?
	// NOTE: Although this implies that we pick it up and use it.

	if ( ! CanTouch(pItem) )
		return false;

	if ( fMoveOrConsume )
	{
		if ( ! CanMove(pItem))
			return false;	// ? using does not imply moving in all cases ! such as reading ?
		if ( IsTakeCrime( pItem ))
			return false;
	}
	else
	{
		// Just snooping i guess.
		if ( pItem->IsTopLevel())
			return true;
		// The item is on another character ?
		const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop != ( static_cast <const CObjBaseTemplate *>(this)))
		{
			if ( IsPriv( PRIV_GM ))
				return true;
			if ( pItem->IsType(IT_CONTAINER))
				return true;
			if ( pItem->IsType(IT_BOOK))
				return true;
			return false;
		}
	}

	return true;
}

CRegionBase * CChar::CheckValidMove( CPointBase & ptDest, WORD * pwBlockFlags, DIR_TYPE dir ) const
{
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( GetTopPoint().IsValidPoint() && ( dir % 2 ))
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
			g_Log.Event( LOGL_WARN, "MOVE: UID(0%x) is moving into a non-defined region (Point: %d,%d,%d,%d)\n", uid(), ptDest.m_x, ptDest.m_y, ptDest.m_z, ptDest.m_map );
		return NULL;
	}

	WORD wCan = GetMoveBlockFlags();
	WORD wBlockFlags = wCan;
	signed char z = g_World.GetHeightPoint( ptDest, wBlockFlags, true );
	if ( wCan != 0xFFFF )
	{
		if ( wBlockFlags &~ wCan )
		{
			return NULL;
		}

		CCharBase* pCharDef = Char_GetDef();
		if ( ! pCharDef->Can( CAN_C_FLY ))
		{
			if ( z > ptDest.m_z + PLAYER_HEIGHT )	// Too high to climb.
			{
				return NULL;
			}
		}
		if ( z >= UO_SIZE_Z )
		{
			return NULL;
		}
	}

	if ( pwBlockFlags )
	{
		*pwBlockFlags = wBlockFlags;
	}

	ptDest.m_z = z;
	return( pArea );
}

CRegionBase * CChar::CheckValidMove_New( CPointBase & ptDest, WORD * pwBlockFlags, DIR_TYPE dir, signed char * pClimbHeight ) const
{
	// Is it ok to move here ? is it blocked ?
	// ignore other characters for now.
	// RETURN:
	//  The new region we may be in.
	//  Fill in the proper ptDest.m_z value for this location. (if walking)
	//  pwBlockFlags = what is blocking me. (can be null = don't care)

	//	test diagonal dirs by two others *only* when already having a normal location
	if ( GetTopPoint().IsValidPoint() && ( dir % 2 ))
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
		return NULL;

	WORD wCan = GetMoveBlockFlags();
	WORD wBlockFlags = wCan;

	if ( wCan & CAN_C_WALK )
		wBlockFlags |= CAN_I_CLIMB; // If we can walk than we can climb. Ignore CAN_C_FLY at all here

	//ptDest.m_z += m_zClimbHeight; // If we are climbing than check z at the top of stairs
	CGrayMapBlockState block( wBlockFlags, ptDest.m_z, ptDest.m_z + m_zClimbHeight + PLAYER_HEIGHT + 3, ptDest.m_z + m_zClimbHeight + 2 );

	g_World.GetHeightPoint_New( ptDest, block, true );

	// Pass along my results.
	wBlockFlags = block.m_Bottom.m_dwBlockFlags;

	if ( block.m_Top.m_dwBlockFlags )
	{
		wBlockFlags |= CAN_I_ROOF;	// we are covered by something.
		if ( block.m_Top.m_z < ptDest.m_z - m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? PLAYER_HEIGHT : PLAYER_HEIGHT/2 ) )
			wBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
	}


	if ( wCan != 0xFFFF )
	{
		// do not check CAN_I_CLIMB
		if ( ( wBlockFlags &~ CAN_I_CLIMB ) &~ wCan )
		{
			return NULL;
		}
		CCharBase* pCharDef = Char_GetDef();
		if ( ! pCharDef->Can( CAN_C_FLY ))
		{
			if ( ! ( wBlockFlags & CAN_I_CLIMB ) ) // we can climb anywhere
			{
				if ( block.m_Bottom.m_wTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + 2) // Too high to climb.
						return NULL;
				}
				else if ( block.m_Bottom.m_z > ptDest.m_z + m_zClimbHeight + PLAYER_HEIGHT + 3)
						return NULL;
			}
		}
		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
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
	if ( IsSetEF( EF_WalkCheck ) ) // get correct climb height (or could be stacked)
	{
		CPointBase pt = GetTopPoint();
		CGrayMapBlockState block( CAN_I_CLIMB, pt.m_z , pt.m_z + PLAYER_HEIGHT + 3, pt.m_z + 2 );

		g_World.GetHeightPoint_New( pt, block, true );

		if ( ( block.m_Bottom.m_z == pt.m_z ) && ( block.m_dwBlockFlags & CAN_I_CLIMB ) ) // we are standing on stairs
			m_zClimbHeight = block.m_zClimbHeight;
		else
			m_zClimbHeight = 0;
	}
	m_fClimbUpdated = true;
}
