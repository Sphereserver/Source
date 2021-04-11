#include "graysvr.h"	// predef header
#include "../network/send.h"

// Simple string hashing algorithm function (by D. J. Bernstein)
// Original code found at: http://www.cse.yorku.ca/~oz/hash.html
DWORD HashString(LPCTSTR str, size_t length)
{
	DWORD hash = 5381;
	for ( size_t i = 0; i < length; ++i )
		hash = ((hash << 5) + hash) + *str++;

	return hash;
}

///////////////////////////////////////////////////////////
// CClient

void CClient::resendBuffs()
{
	ADDTOCALLSTACK("CClient::resendBuffs");
	// NOTE: If the player logout and login again without close the client, buffs with remaining
	// time will stay cached on client, making it not display the remaining time if the server
	// send this same buff again. To avoid this, we must remove the buff before send it again.

	if ( !IsSetOF(OF_Buffs) || !PacketBuff::CanSendTo(m_NetState) )
		return;

	if ( m_pChar->IsStatFlag(STATF_NightSight) )
		addBuff(BI_NIGHTSIGHT, 1075643, 1075644);
	if ( m_pChar->IsStatFlag(STATF_Hidden|STATF_Insubstantial) )
		addBuff(BI_HIDDEN, 1075655, 1075656);
	if ( m_pChar->IsStatFlag(STATF_Hovering) )
		addBuff(BI_GARGOYLEFLY, 1112193, 1112567);
	if ( m_pChar->m_Act_SkillCurrent == SKILL_MEDITATION )
		addBuff(BI_ACTIVEMEDITATION, 1075657, 1075658);
	if ( m_pChar->LayerFind(LAYER_FLAG_Criminal) )
		addBuff(BI_CRIMINALSTATUS, 1153802, 1153828);

	INT64 iTimerItem = 0;
	CItem *pMemory = m_pChar->LayerFind(LAYER_FLAG_Stuck);
	if ( pMemory )
	{
		iTimerItem = pMemory->GetTimerAdjusted();
		removeBuff(BI_PARALYZE);
		addBuff(BI_PARALYZE, 1075827, 1075828, static_cast<WORD>(maximum(iTimerItem, 0)));
	}

	WORD wStatEffect = 0;
	WORD wTimerEffect = 0;

	TCHAR szNumBuff[7][MAX_NAME_SIZE];
	LPCTSTR pszNumBuff[7] = { szNumBuff[0], szNumBuff[1], szNumBuff[2], szNumBuff[3], szNumBuff[4], szNumBuff[5], szNumBuff[6] };

	for ( CItem *pItem = m_pChar->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsType(IT_SPELL) )
			continue;

		iTimerItem = pItem->GetTimerAdjusted();
		wStatEffect = pItem->m_itSpell.m_spelllevel;
		wTimerEffect = static_cast<WORD>(maximum(iTimerItem, 0));

		switch ( pItem->m_itSpell.m_spell )
		{
			case SPELL_Clumsy:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_CLUMSY);
				addBuff(BI_CLUMSY, 1075831, 1075832, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Feeblemind:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_FEEBLEMIND);
				addBuff(BI_FEEBLEMIND, 1075833, 1075834, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Night_Sight:
			{
				removeBuff(BI_NIGHTSIGHT);
				addBuff(BI_NIGHTSIGHT, 1075643, 1075644, wTimerEffect);
				break;
			}
			case SPELL_Reactive_Armor:
			{
				removeBuff(BI_REACTIVEARMOR);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(wStatEffect, szNumBuff[0], 10);
					for ( int i = 1; i < 5; ++i )
						ITOA(5, szNumBuff[i], 10);

					addBuff(BI_REACTIVEARMOR, 1075812, 1075813, wTimerEffect, pszNumBuff, 5);
				}
				else
					addBuff(BI_REACTIVEARMOR, 1075812, 1070722, wTimerEffect);
				break;
			}
			case SPELL_Weaken:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_WEAKEN);
				addBuff(BI_WEAKEN, 1075837, 1075838, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Agility:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_AGILITY);
				addBuff(BI_AGILITY, 1075841, 1075842, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Cunning:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_CUNNING);
				addBuff(BI_CUNNING, 1075843, 1075844, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Protection:
			case SPELL_Arch_Prot:
			{
				BUFF_ICONS BuffIcon = BI_PROTECTION;
				DWORD BuffCliloc = 1075814;
				if ( pItem->m_itSpell.m_spell == SPELL_Arch_Prot )
				{
					BuffIcon = BI_ARCHPROTECTION;
					BuffCliloc = 1075816;
				}

				removeBuff(BuffIcon);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
					ITOA(-pItem->m_itSpell.m_PolyDex / 10, szNumBuff[1], 10);
					addBuff(BuffIcon, BuffCliloc, 1075815, wTimerEffect, pszNumBuff, 2);
				}
				else
					addBuff(BuffIcon, BuffCliloc, 1070722, wTimerEffect);
				break;
			}
			case SPELL_Strength:
			{
				ITOA(wStatEffect, szNumBuff[0], 10);
				removeBuff(BI_STRENGTH);
				addBuff(BI_STRENGTH, 1075845, 1075846, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Bless:
			{
				for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
					ITOA(wStatEffect, szNumBuff[i], 10);

				removeBuff(BI_BLESS);
				addBuff(BI_BLESS, 1075847, 1075848, wTimerEffect, pszNumBuff, STAT_BASE_QTY);
				break;
			}
			case SPELL_Poison:
			{
				removeBuff(BI_POISON);
				addBuff(BI_POISON, 1017383, 1070722, wTimerEffect);
				break;
			}
			case SPELL_Curse:
			{
				for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
					ITOA(wStatEffect, szNumBuff[i], 10);
				for ( int i = 3; i < 7; ++i )
					ITOA(10, szNumBuff[i], 10);

				removeBuff(BI_CURSE);
				addBuff(BI_CURSE, 1075835, 1075836, wTimerEffect, pszNumBuff, 7);
				break;
			}
			case SPELL_Incognito:
			{
				removeBuff(BI_INCOGNITO);
				addBuff(BI_INCOGNITO, 1075819, 1075820, wTimerEffect);
				break;
			}
			case SPELL_Magic_Reflect:
			{
				removeBuff(BI_MAGICREFLECTION);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					ITOA(-wStatEffect, szNumBuff[0], 10);
					for ( int i = 1; i < 5; ++i )
						ITOA(10, szNumBuff[i], 10);

					addBuff(BI_MAGICREFLECTION, 1075817, 1075818, wTimerEffect, pszNumBuff, 5);
				}
				else
					addBuff(BI_MAGICREFLECTION, 1075817, 1070722, wTimerEffect);
				break;
			}
			case SPELL_Paralyze:
			{
				removeBuff(BI_PARALYZE);
				addBuff(BI_PARALYZE, 1075827, 1075828, wTimerEffect);
				break;
			}
			case SPELL_Invis:
			{
				removeBuff(BI_INVISIBILITY);
				addBuff(BI_INVISIBILITY, 1075825, 1075826, wTimerEffect);
				break;
			}
			case SPELL_Mass_Curse:
			{
				for ( int i = STAT_STR; i < STAT_BASE_QTY; ++i )
					ITOA(wStatEffect, szNumBuff[i], 10);

				removeBuff(BI_MASSCURSE);
				addBuff(BI_MASSCURSE, 1075839, 1075840, wTimerEffect, pszNumBuff, 3);
				break;
			}
			case SPELL_Polymorph:
			case SPELL_BeastForm:
			case SPELL_Monster_Form:
			{
				LPCTSTR pszName = "creature";
				CCharBase *pPolyCharDef = CCharBase::FindCharBase(m_pChar->m_atMagery.m_SummonID);
				if ( pPolyCharDef && (pPolyCharDef->GetName()[0] != '#') )
				{
					pszName = pPolyCharDef->GetName();
					_strlwr(const_cast<TCHAR *>(pszName));
				}

				strncpy(szNumBuff[0], Str_GetArticleAndSpace(pszName), sizeof(szNumBuff[0]));
				szNumBuff[0][sizeof(szNumBuff[0]) - 1] = '\0';
				szNumBuff[0][strlen(szNumBuff[0]) - 1] = '\0';		// trim whitespace from "a " / "an " strings
				strncpy(szNumBuff[1], pszName, sizeof(szNumBuff[1]));
				szNumBuff[1][sizeof(szNumBuff[1]) - 1] = '\0';
				removeBuff(BI_POLYMORPH);
				addBuff(BI_POLYMORPH, 1075824, 1075823, wTimerEffect, pszNumBuff, 2);
				break;
			}
			case SPELL_Blood_Oath:
			{
				const CChar *pCaster = pItem->m_uidLink.CharFind();
				if ( pCaster )
				{
					strncpy(szNumBuff[0], pCaster->GetName(), sizeof(szNumBuff[0]));
					szNumBuff[0][sizeof(szNumBuff[0]) - 1] = '\0';
					strncpy(szNumBuff[1], pCaster->GetName(), sizeof(szNumBuff[1]));
					szNumBuff[1][sizeof(szNumBuff[1]) - 1] = '\0';
					removeBuff(BI_BLOODOATHCURSE);
					addBuff(BI_BLOODOATHCURSE, 1075659, 1075660, wTimerEffect, pszNumBuff, 2);
				}
				break;
			}
			case SPELL_Corpse_Skin:
			{
				removeBuff(BI_CORPSESKIN);
				addBuff(BI_CORPSESKIN, 1075663, 1075664, wTimerEffect);
				break;
			}
			case SPELL_Horrific_Beast:
			{
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[1], 10);
				removeBuff(BI_HORRIFICBEAST);
				addBuff(BI_HORRIFICBEAST, 1060514, 1153763, wTimerEffect, pszNumBuff, 2);
				break;
			}
			case SPELL_Lich_Form:
			{
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[1], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[2], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[3], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[4], 10);
				removeBuff(BI_LICHFORM);
				addBuff(BI_LICHFORM, 1060515, 1153767, wTimerEffect, pszNumBuff, 5);
				break;
			}
			case SPELL_Strangle:
			{
				int iDex = m_pChar->Stat_GetAdjusted(STAT_DEX);
				double dStamPenalty = 3 - ((static_cast<double>(m_pChar->Stat_GetVal(STAT_DEX)) / maximum(1, iDex)) * 2);
				WORD wTimerTotal = 0;
				for ( WORD w = 0; w < wStatEffect; ++w )
					wTimerTotal += (wStatEffect - w) * TICK_PER_SEC;

				ITOA(static_cast<int>((wStatEffect - 2) * dStamPenalty), szNumBuff[0], 10);
				ITOA(static_cast<int>((wStatEffect + 1) * dStamPenalty), szNumBuff[1], 10);
				removeBuff(BI_STRANGLE);
				addBuff(BI_STRANGLE, 1075794, 1075795, wTimerTotal, pszNumBuff, 2);
				break;
			}
			case SPELL_Vampiric_Embrace:
			{
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[1], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[2], 10);
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[3], 10);
				removeBuff(BI_VAMPIRICEMBRACE);
				addBuff(BI_VAMPIRICEMBRACE, 1060521, 1153768, wTimerEffect, pszNumBuff, 4);
				break;
			}
			case SPELL_Wraith_Form:
			{
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[1], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[2], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[3], 10);
				removeBuff(BI_WRAITHFORM);
				addBuff(BI_WRAITHFORM, 1060524, 1153829, wTimerEffect, pszNumBuff, 4);
				break;
			}
			case SPELL_Gift_of_Renewal:
			{
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				removeBuff(BI_GIFTOFRENEWAL);
				addBuff(BI_GIFTOFRENEWAL, 1075796, 1075797, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Attunement:
			{
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				removeBuff(BI_ATTUNEWEAPON);
				addBuff(BI_ATTUNEWEAPON, 1075798, 1075799, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Thunderstorm:
			{
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				removeBuff(BI_THUNDERSTORM);
				addBuff(BI_THUNDERSTORM, 1075800, 1075801, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Reaper_Form:
			{
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[1], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[2], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[3], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[4], 10);
				ITOA(pItem->m_itSpell.m_PolyDex, szNumBuff[5], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[6], 10);
				removeBuff(BI_REAPERFORM);
				addBuff(BI_REAPERFORM, 1071034, 1153781, wTimerEffect, pszNumBuff, 7);
				break;
			}
			case SPELL_Essence_of_Wind:
			{
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				removeBuff(BI_ESSENCEOFWIND);
				addBuff(BI_ESSENCEOFWIND, 1075802, 1075803, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Ethereal_Voyage:
			{
				removeBuff(BI_ETHEREALVOYAGE);
				addBuff(BI_ETHEREALVOYAGE, 1075804, 1075805, wTimerEffect);
				break;
			}
			case SPELL_Gift_of_Life:
			{
				removeBuff(BI_GIFTOFLIFE);
				addBuff(BI_GIFTOFLIFE, 1075806, 1075807, wTimerEffect);
				break;
			}
			case SPELL_Arcane_Empowerment:
			{
				ITOA(pItem->m_itSpell.m_spelllevel, szNumBuff[0], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[1], 10);
				removeBuff(BI_ARCANEEMPOWERMENT);
				addBuff(BI_ARCANEEMPOWERMENT, 1075805, 1075804, wTimerEffect, pszNumBuff, 1);
				break;
			}
			case SPELL_Stone_Form:
			{
				ITOA(-pItem->m_itSpell.m_PolyStr, szNumBuff[0], 10);
				ITOA(-pItem->m_itSpell.m_PolyDex, szNumBuff[1], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[2], 10);
				ITOA(pItem->m_itSpell.m_spellcharges, szNumBuff[3], 10);
				ITOA(pItem->m_itSpell.m_PolyStr, szNumBuff[4], 10);
				removeBuff(BI_STONEFORM);
				addBuff(BI_STONEFORM, 1080145, 1080146, wTimerEffect, pszNumBuff, 5);
				break;
			}
			default:
				break;
		}
	}
}

void CClient::addBuff(const BUFF_ICONS IconID, const DWORD dwClilocOne, const DWORD dwClilocTwo, const WORD wTime, LPCTSTR *pszArgs, size_t iArgCount)
{
	ADDTOCALLSTACK("CClient::addBuff");
	if ( !IsSetOF(OF_Buffs) || !PacketBuff::CanSendTo(m_NetState) )
		return;

	new PacketBuff(this, IconID, dwClilocOne, dwClilocTwo, wTime, pszArgs, iArgCount);
}

void CClient::removeBuff(const BUFF_ICONS IconID)
{
	ADDTOCALLSTACK("CClient::removeBuff");
	if ( !IsSetOF(OF_Buffs) || !PacketBuff::CanSendTo(m_NetState) )
		return;

	new PacketBuff(this, IconID);
}

void CClient::addDeleteErr(BYTE bCode)
{
	ADDTOCALLSTACK("CClient::addDeleteErr");
	new PacketDeleteError(this, static_cast<PacketDeleteError::Reason>(bCode));
}

void CClient::addTime(bool fCurrent)
{
	ADDTOCALLSTACK("CClient::addTime");
	// Send time. (real or game time ??? why ?)

	if ( fCurrent )
	{
		UINT64 ulCurrentTime = CServTime::GetCurrentTime().GetTimeRaw();
		new PacketGameTime(this, static_cast<BYTE>((ulCurrentTime / (60 * 60 * TICK_PER_SEC)) % 24), static_cast<BYTE>((ulCurrentTime / (60 * TICK_PER_SEC)) % 60), static_cast<BYTE>((ulCurrentTime / TICK_PER_SEC) % 60));
	}
	else
		new PacketGameTime(this);
}

void CClient::closeUIWindow(DWORD dwWindowType, const CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::closeUIWindow");
	new PacketCloseUIWindow(this, static_cast<PacketCloseUIWindow::WindowType>(dwWindowType), pObj);
}

void CClient::addObjectRemove(CGrayUID uid)
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	new PacketRemoveObject(this, uid);
}

void CClient::addObjectRemove(const CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	addObjectRemove(pObj->GetUID());
}

void CClient::addObjectRemoveMsg(CGrayUID uid)
{
	ADDTOCALLSTACK("CClient::addObjectRemoveMsg");
	// Seems this object got out of sync somehow, so remove it from client screen
	SysMessage("You can't see the target");
	addObjectRemove(uid);
}

void CClient::addRemoveAll(bool fItems, bool fChars)
{
	ADDTOCALLSTACK("CClient::addRemoveAll");
	if ( fItems )
	{
		// Remove any multi objects first, or client will hang
		CWorldSearch AreaItems(m_pChar->GetTopPoint(), UO_MAP_VIEW_RADAR);
		for (;;)
		{
			CItem *pItem = AreaItems.GetItem();
			if ( !pItem )
				break;
			addObjectRemove(pItem);
		}
	}
	if ( fChars )
	{
		CWorldSearch AreaChars(m_pChar->GetTopPoint(), m_pChar->GetSight());
		AreaChars.SetAllShow(IsPriv(PRIV_ALLSHOW));
		for (;;)
		{
			CChar *pChar = AreaChars.GetChar();
			if ( !pChar )
				break;
			if ( pChar == m_pChar )
				continue;
			addObjectRemove(pChar);
		}
	}
}

void CClient::addItem_OnGround(CItem *pItem)
{
	ADDTOCALLSTACK("CClient::addItem_OnGround");
	if ( !pItem )
		return;

	if ( PacketWorldObj::CanSendTo(m_NetState) )
		new PacketWorldObj(this, pItem);
	else
		new PacketWorldItem(this, pItem);

	// Send KR drop confirmation
	if ( PacketDropAccepted::CanSendTo(m_NetState) )
		new PacketDropAccepted(this);

	// Send item sound
	if ( pItem->IsType(IT_SOUND) )
		addSound(static_cast<SOUND_TYPE>(pItem->m_itSound.m_Sound), pItem, pItem->m_itSound.m_Repeat);

	// Send corpse clothing
	if ( !IsPriv(PRIV_DEBUG) && (pItem->GetDispID() == ITEMID_CORPSE) && CCharBase::IsPlayableID(pItem->GetCorpseType()) )
	{
		const CItemCorpse *pCorpse = dynamic_cast<const CItemCorpse *>(pItem);
		if ( pCorpse )
		{
			addContents(pCorpse, false, true);	// send all corpse items
			addContents(pCorpse, true, true);	// equip items on corpse
		}
	}

	// Send item tooltip
	addAOSTooltip(pItem);

	// Send house design version
	if ( pItem->IsType(IT_MULTI_CUSTOM) && (m_pChar->GetTopPoint().GetDist(pItem->GetTopPoint()) <= m_pChar->GetSight()) )
	{
		CItemMultiCustom *pItemMulti = dynamic_cast<CItemMultiCustom *>(pItem);
		if ( pItemMulti )
			pItemMulti->SendVersionTo(this);
	}
}

void CClient::addItem_Equipped(const CItem *pItem)
{
	ADDTOCALLSTACK("CClient::addItem_Equipped");
	ASSERT(pItem);
	if ( (pItem->GetParent() != m_pChar) && !m_pChar->CanSeeItem(pItem) )
		return;

	new PacketItemEquipped(this, pItem);

	//addAOSTooltip(pItem);		// tooltips for equipped items are handled on packet 0x78 (PacketCharacter)
}

void CClient::addItem_InContainer(const CItem *pItem)
{
	ADDTOCALLSTACK("CClient::addItem_InContainer");
	ASSERT(pItem);

	new PacketItemContainer(this, pItem);

	if ( PacketDropAccepted::CanSendTo(m_NetState) )
		new PacketDropAccepted(this);

	//addAOSTooltip(pItem);		// tooltips for items inside containers are handled on packet 0x3C (PacketItemContents)
}

void CClient::addItem(CItem *pItem)
{
	ADDTOCALLSTACK("CClient::addItem");
	if ( !pItem )
		return;

	if ( pItem->IsTopLevel() )
		addItem_OnGround(pItem);
	else if ( pItem->IsItemEquipped() )
		addItem_Equipped(pItem);
	else if ( pItem->IsItemInContainer() )
		addItem_InContainer(pItem);
}

void CClient::addContents(const CItemContainer *pContainer, bool fCorpseEquip, bool fCorpseFilter, bool fShop) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContents");
	// NOTE: We needed to send the header for this FIRST!

	if ( fCorpseEquip )
		new PacketCorpseEquipment(this, pContainer);
	else
		new PacketItemContents(this, pContainer, fShop, fCorpseFilter);
}

void CClient::addOpenGump(const CObjBase *pContainer, GUMP_TYPE gump)
{
	ADDTOCALLSTACK("CClient::addOpenGump");
	// NOTE: if pContainer has not already been sent to the client it will crash

	new PacketContainerOpen(this, pContainer, gump);
}

bool CClient::addContainerSetup(const CItemContainer *pContainer)
{
	ADDTOCALLSTACK("CClient::addContainerSetup");
	ASSERT(pContainer);
	ASSERT(pContainer->IsItem());

	CItemBase *pItemDef = pContainer->Item_GetDef();
	if ( !pItemDef )
		return false;

	GUMP_TYPE gump = pItemDef->GetContainerGumpID();
	if ( gump <= GUMP_NONE )
		return false;

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);

	addOpenGump(pContainer, gump);
	addContents(pContainer);

	LogOpenedContainer(pContainer);
	return true;
}

void CClient::LogOpenedContainer(const CItemContainer *pContainer)
{
	ADDTOCALLSTACK("CClient::LogOpenedContainer");
	if ( !pContainer )
		return;

	CObjBaseTemplate *pTopMostContainer = pContainer->GetTopLevelObj();
	DWORD dwTopMostContainerUID = pTopMostContainer->GetUID().GetPrivateUID();

	CObjBase *pTopContainer = pContainer->GetParentObj();
	DWORD dwTopContainerUID = pTopContainer ? pTopContainer->GetUID().GetPrivateUID() : dwTopMostContainerUID;

	m_openedContainers[pContainer->GetUID().GetPrivateUID()] = std::make_pair(std::make_pair(dwTopContainerUID, dwTopMostContainerUID), pTopMostContainer->GetTopPoint());
}

void CClient::addSeason(SEASON_TYPE season)
{
	ADDTOCALLSTACK("CClient::addSeason");
	if ( m_pChar->IsStatFlag(STATF_DEAD) )
		season = SEASON_Desolate;
	if ( m_Env.m_Season == season )
		return;

	m_Env.m_Season = season;
	new PacketSeason(this, season, true);

	// Client resets light level on season change, so resend light here too
	m_Env.m_Light = BYTE_MAX;
	addLight();
}

void CClient::addWeather(WEATHER_TYPE weather)
{
	ADDTOCALLSTACK("CClient::addWeather");
	ASSERT(m_pChar);

	if ( g_Cfg.m_fNoWeather )
		return;

	if ( weather == WEATHER_Default )
		weather = m_pChar->GetTopSector()->GetWeather();

	if ( m_Env.m_Weather == weather )
		return;

	m_Env.m_Weather = weather;
	new PacketWeather(this, weather, static_cast<BYTE>(Calc_GetRandVal(10, 70)), 0x10);
}

void CClient::addLight()
{
	ADDTOCALLSTACK("CClient::addLight");
	// NOTE: This could just be a flash of light.

	ASSERT(m_pChar);
	BYTE bLight = BYTE_MAX;

	if ( m_pChar->m_LocalLight )
		bLight = m_pChar->m_LocalLight;
	if ( bLight == BYTE_MAX )
		bLight = m_pChar->GetLightLevel();

	if ( m_Env.m_Light == bLight )
		return;

	m_Env.m_Light = bLight;
	new PacketGlobalLight(this, bLight);
}

void CClient::addArrowQuest(WORD x, WORD y, DWORD id)
{
	ADDTOCALLSTACK("CClient::addArrowQuest");
	new PacketArrowQuest(this, x, y, id);
}

void CClient::addMusic(MIDI_TYPE id)
{
	ADDTOCALLSTACK("CClient::addMusic");
	new PacketPlayMusic(this, id);
}

void CClient::addKick(CTextConsole *pSrc, bool fBlock)
{
	ADDTOCALLSTACK("CClient::addKick");
	ASSERT(pSrc);
	if ( !m_pAccount )
	{
		m_NetState->markReadClosed();
		return;
	}

	if ( !m_pAccount->Kick(pSrc, fBlock) )
		return;

	SysMessagef("You have been %sed by '%s'", fBlock ? "BLOCK" : "DISCONNECT", pSrc->GetName());

	if ( IsConnectTypePacket() )
		new PacketKick(this);

	m_NetState->markReadClosed();
}

void CClient::addSound(SOUND_TYPE id, const CObjBaseTemplate *pSrc, BYTE bRepeat)
{
	ADDTOCALLSTACK("CClient::addSound");
	if ( !g_Cfg.m_fGenericSounds )
		return;

	CPointMap pt;
	if ( pSrc )
	{
		pSrc = pSrc->GetTopLevelObj();
		pt = pSrc->GetTopPoint();
	}
	else
		pt = m_pChar->GetTopPoint();

	if ( (id > 0) && !bRepeat && !pSrc )
		return;

	new PacketPlaySound(this, id, bRepeat, 0, pt);
}

void CClient::addBarkUNICODE(const NCHAR *pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CClient::addBarkUNICODE");
	if ( !pszText )
		return;

	if ( !IsConnectTypePacket() )
	{
		// Need to convert back from unicode!
		//SysMessage(pszText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	new PacketMessageUNICODE(this, pszText, pSrc, wHue, mode, font, lang);
}

void CClient::addBarkLocalized(DWORD dwClilocID, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CClient::addBarkLocalized");
	if ( !IsConnectTypePacket() || (dwClilocID <= 0) )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	new PacketMessageLocalised(this, dwClilocID, pSrc, wHue, mode, font, pszArgs);
}

void CClient::addBarkLocalizedEx(DWORD dwClilocID, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affix, LPCTSTR pszAffix, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CClient::addBarkLocalizedEx");
	if ( !IsConnectTypePacket() || (dwClilocID <= 0) )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	new PacketMessageLocalisedEx(this, dwClilocID, pSrc, wHue, mode, font, affix, pszAffix, pszArgs);
}

void CClient::addBarkParse(LPCTSTR pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool fUnicode)
{
	ADDTOCALLSTACK("CClient::addBarkParse");
	if ( !pszText )
		return;

	HUE_TYPE defaultHue = HUE_TEXT_DEF;
	FONT_TYPE defaultFont = FONT_NORMAL;
	bool defaultUnicode = false;

	switch ( mode )
	{
		case TALKMODE_SYSTEM:
		{
			defaultHue = static_cast<HUE_TYPE>(g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_COLOR"));
			defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_FONT"));
			defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_UNICODE") > 0);
			break;
		}
		case TALKMODE_EMOTE:
		{
			defaultHue = static_cast<HUE_TYPE>(g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_COLOR"));
			defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_FONT"));
			defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_UNICODE") > 0);
			break;
		}
		case TALKMODE_SAY:
		{
			defaultHue = static_cast<HUE_TYPE>(g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_COLOR"));
			defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_FONT"));
			defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_UNICODE") > 0);
			break;
		}
		case TALKMODE_OBJ:
		{
			defaultHue = static_cast<HUE_TYPE>(g_Exp.m_VarDefs.GetKeyNum("MSG_DEF_COLOR"));
			defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("MSG_DEF_FONT"));
			defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("MSG_DEF_UNICODE") > 0);
			break;
		}
		case TALKMODE_ITEM:
		{
			if ( pSrc->IsChar() )
			{
				defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("CMSG_DEF_FONT"));
				defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("CMSG_DEF_UNICODE") > 0);
			}
			else
			{
				defaultHue = static_cast<HUE_TYPE>(g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_COLOR"));
				defaultFont = static_cast<FONT_TYPE>(g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_FONT"));
				defaultUnicode = (g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_UNICODE") > 0);
			}
			break;
		}
		default:
			break;
	}

	WORD wArgs[] = { static_cast<WORD>(wHue), static_cast<WORD>(font), static_cast<WORD>(fUnicode) };

	if ( *pszText == '@' )
	{
		++pszText;
		if ( *pszText == '@' )	// @@ = just a @ symbol
			goto bark_default;

		const char *s = pszText;
		pszText = strchr(s, ' ');

		if ( pszText )
		{
			for ( int i = 0; (s < pszText) && (i < 3); ++i, ++s )
			{
				if ( *s != ',' )
					wArgs[i] = static_cast<WORD>(Exp_GetLLVal(s));
			}
			++pszText;
			if ( wArgs[1] > FONT_QTY )
				wArgs[1] = static_cast<WORD>(FONT_NORMAL);
		}
	}

	if ( wArgs[0] == HUE_TEXT_DEF )
		wArgs[0] = static_cast<WORD>(defaultHue);
	if ( wArgs[1] == FONT_NORMAL )
		wArgs[1] = static_cast<WORD>(defaultFont);
	if ( wArgs[2] == 0 )
		wArgs[2] = static_cast<WORD>(defaultUnicode);

	if ( m_BarkBuffer.IsEmpty() )
		m_BarkBuffer = pszText;

	switch ( wArgs[2] )
	{
		case 3:		// extended localized message (with affixed ASCII text)
		{
			TCHAR *ppArgs[16];
			size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(m_BarkBuffer.GetPtr()), ppArgs, COUNTOF(ppArgs), ",");

			CGString sVal;
			for ( size_t i = 3; i < iArgQty; ++i )
			{
				if ( sVal.GetLength() )
					sVal += "\t";
				sVal += !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i];
			}

			addBarkLocalizedEx(static_cast<DWORD>(Exp_GetLLVal(ppArgs[0])), pSrc, static_cast<HUE_TYPE>(wArgs[0]), mode, static_cast<FONT_TYPE>(Exp_GetLLVal(ppArgs[1])), static_cast<AFFIX_TYPE>(Exp_GetLLVal(ppArgs[1])), ppArgs[2], sVal.GetPtr());
			break;
		}

		case 2:		// localized
		{
			TCHAR *ppArgs[16];
			size_t iArgQty = Str_ParseCmds(const_cast<TCHAR *>(m_BarkBuffer.GetPtr()), ppArgs, COUNTOF(ppArgs), ",");

			CGString sVal;
			for ( size_t i = 1; i < iArgQty; ++i )
			{
				if ( sVal.GetLength() )
					sVal += "\t";
				sVal += !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i];
			}

			addBarkLocalized(static_cast<DWORD>(Exp_GetLLVal(ppArgs[0])), pSrc, static_cast<HUE_TYPE>(wArgs[0]), mode, static_cast<FONT_TYPE>(wArgs[1]), sVal.GetPtr());
			break;
		}

		case 1:		// unicode
		{
			NCHAR szBuffer[MAX_TALK_BUFFER];
			CvtSystemToNUNICODE(szBuffer, COUNTOF(szBuffer), m_BarkBuffer.GetPtr(), -1);
			addBarkUNICODE(szBuffer, pSrc, static_cast<HUE_TYPE>(wArgs[0]), mode, static_cast<FONT_TYPE>(wArgs[1]), 0);
			break;
		}

		case 0:		// ASCII
		default:
		{
		bark_default:
			if ( m_BarkBuffer.IsEmpty() )
				m_BarkBuffer = pszText;

			addBark(m_BarkBuffer.GetPtr(), pSrc, static_cast<HUE_TYPE>(wArgs[0]), mode, static_cast<FONT_TYPE>(wArgs[1]));
			break;
		}
	}

	// Empty the buffer
	m_BarkBuffer.Empty();
}

void CClient::addBark(LPCTSTR pszText, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font)
{
	ADDTOCALLSTACK("CClient::addBark");
	if ( !pszText )
		return;

	if ( !IsConnectTypePacket() )
	{
		SysMessage(pszText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	new PacketMessageASCII(this, pszText, pSrc, wHue, mode, font);
}

void CClient::addObjMessage(LPCTSTR pszMsg, const CObjBaseTemplate *pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode) // The message when an item is clicked
{
	ADDTOCALLSTACK("CClient::addObjMessage");
	if ( !pszMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && (GetPrivLevel() == PLEVEL_Player) )
	{
		if ( !strcmpi(pszMsg, m_szLastObjMessage) )
			return;

		strncpy(m_szLastObjMessage, pszMsg, sizeof(m_szLastObjMessage));
		m_szLastObjMessage[sizeof(m_szLastObjMessage) - 1] = '\0';
	}

	addBarkParse(pszMsg, pSrc, wHue, mode);
}

void CClient::addEffect(EFFECT_TYPE motion, const CObjBaseTemplate *pSrc, CPointMap ptSrc, const CObjBaseTemplate *pDest, CPointMap ptDest, ITEMID_TYPE id, BYTE bSpeed, BYTE bFrames, bool fExplode, DWORD dwColor, DWORD dwRender, WORD wEffectID, WORD wExplodeID, WORD wExplodeSound, DWORD dwItemUID, BYTE bLayer)
{
	ADDTOCALLSTACK("CClient::addEffect");
	ASSERT(m_pChar);

	if ( wEffectID || wExplodeID )
		new PacketEffect(this, motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode, dwColor, dwRender, wEffectID, wExplodeID, wExplodeSound, dwItemUID, bLayer);
	else if ( dwColor || dwRender )
		new PacketEffect(this, motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode, dwColor, dwRender);
	else
		new PacketEffect(this, motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode);
}

void CClient::GetAdjustedItemID(const CChar *pChar, const CItem *pItem, ITEMID_TYPE &id, HUE_TYPE &wHue) const
{
	ADDTOCALLSTACK("CClient::GetAdjustedItemID");
	ASSERT(pChar);

	id = pItem->GetDispID();
	wHue = pItem->GetHue();
	CItemBase *pItemDef = pItem->Item_GetDef();

	if ( pItem->IsType(IT_EQ_HORSE) )
	{
		// Check the RESLEVEL of the ridden horse
		CREID_TYPE idHorse = pItem->m_itFigurine.m_ID;
		CCharBase *pCharDef = CCharBase::FindCharBase(idHorse);
		if ( pCharDef && (GetResDisp() < pCharDef->GetResLevel()) )
		{
			idHorse = static_cast<CREID_TYPE>(pCharDef->GetResDispDnId());
			wHue = pCharDef->GetResDispDnHue();

			// Display the mount item associated with the RESDISPDNID of the mount's chardef
			if ( idHorse != pItem->m_itFigurine.m_ID )
			{
				TCHAR szMountDefname[20];
				snprintf(szMountDefname, sizeof(szMountDefname), "mount_0x%x", idHorse);
				ITEMID_TYPE idMountItem = static_cast<ITEMID_TYPE>(g_Exp.m_VarDefs.GetKeyNum(szMountDefname));
				if ( idMountItem > ITEMID_NOTHING )
				{
					id = idMountItem;
					pItemDef = CItemBase::FindItemBase(id);
				}
			}
		}
	}

	if ( m_pChar->IsStatFlag(STATF_Hallucinating) )
		wHue = static_cast<HUE_TYPE>(Calc_GetRandVal(HUE_DYE_HIGH));
	else if ( pChar->IsStatFlag(STATF_Stone) )
		wHue = HUE_STONE;
	else if ( pChar->IsStatFlag(STATF_Insubstantial) )
		wHue = g_Cfg.m_iColorInvis;
	else
	{
		if ( pItemDef && (GetResDisp() < pItemDef->GetResLevel()) )
			if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pItemDef->GetResDispDnHue();

		if ( (wHue & HUE_MASK_HI) > HUE_QTY )
			wHue &= (HUE_MASK_LO|HUE_MASK_UNDERWEAR|HUE_MASK_TRANSLUCENT);
		else
			wHue &= (HUE_MASK_HI|HUE_MASK_UNDERWEAR|HUE_MASK_TRANSLUCENT);
	}

	if ( pItemDef && (GetResDisp() < pItemDef->GetResLevel()) )
		id = static_cast<ITEMID_TYPE>(pItemDef->GetResDispDnId());
}

void CClient::GetAdjustedCharID(const CChar *pChar, CREID_TYPE &id, HUE_TYPE &wHue) const
{
	ADDTOCALLSTACK("CClient::GetAdjustedCharID");
	// Some clients can't see all creature artwork and colors.
	// Try to do the translation here.

	ASSERT(m_pAccount);
	ASSERT(pChar);

	if ( IsPriv(PRIV_DEBUG) )
	{
		id = CREID_MAN;
		wHue = HUE_DEFAULT;
		return;
	}

	id = pChar->GetDispID();
	CCharBase *pCharDef = pChar->Char_GetDef();

	if ( m_pChar->IsStatFlag(STATF_Hallucinating) )
	{
		if ( pChar != m_pChar )
		{
			// Get a random creature from the artwork
			pCharDef = NULL;
			while ( !pCharDef )
			{
				id = static_cast<CREID_TYPE>(Calc_GetRandVal(CREID_EQUIP_GM_ROBE));
				if ( id != CREID_SEA_Creature )		// skip this chardef, it can crash many clients
					pCharDef = CCharBase::FindCharBase(id);
			}
		}

		wHue = static_cast<HUE_TYPE>(Calc_GetRandVal(HUE_DYE_HIGH));
	}
	else
	{
		if ( pChar->IsStatFlag(STATF_Stone) )
			wHue = HUE_STONE;
		else if ( pChar->IsStatFlag(STATF_Insubstantial) )
			wHue = g_Cfg.m_iColorInvis;
		else if ( pChar->IsStatFlag(STATF_Hidden) )
			wHue = g_Cfg.m_iColorHidden;
		else if ( pChar->IsStatFlag(STATF_Invisible) )
			wHue = g_Cfg.m_iColorInvisSpell;
		else
		{
			wHue = pChar->GetHue();
			if ( (wHue & HUE_MASK_HI) > HUE_QTY )
				wHue &= (HUE_MASK_LO|HUE_MASK_UNDERWEAR|HUE_MASK_TRANSLUCENT);
			else
				wHue &= (HUE_MASK_HI|HUE_MASK_UNDERWEAR|HUE_MASK_TRANSLUCENT);
		}
	}

	if ( pCharDef && (GetResDisp() < pCharDef->GetResLevel()) )
	{
		id = static_cast<CREID_TYPE>(pCharDef->GetResDispDnId());
		if ( pCharDef->GetResDispDnHue() != HUE_DEFAULT )
			wHue = pCharDef->GetResDispDnHue();
	}
}

void CClient::addCharMove(const CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addCharMove");
	addCharMove(pChar, pChar->GetDirFlag());
}

void CClient::addCharMove(const CChar *pChar, BYTE bDir)
{
	ADDTOCALLSTACK("CClient::addCharMove");
	// This char has just moved on screen, or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself
	new PacketCharacterMove(this, pChar, bDir);
}

void CClient::addChar(const CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addChar");
	// Full update about a char
	EXC_TRY("addChar");
	new PacketCharacter(this, pChar);

	EXC_SET("Wake sector");
	pChar->GetTopPoint().GetSector()->SetSectorWakeStatus();

	EXC_SET("Health bar color");
	addHealthBarUpdate(pChar);

	if ( pChar->m_pNPC && pChar->m_pNPC->m_bonded && pChar->IsStatFlag(STATF_DEAD) )
	{
		EXC_SET("Bonded status");
		addBondedStatus(pChar, true);
	}

	EXC_SET("AOSToolTip adding (end)");
	addAOSTooltip(pChar);

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("m_dirFace (0%x)\n", pChar->m_dirFace);
	EXC_DEBUG_END;
}

void CClient::addItemName(const CItem *pItem)
{
	ADDTOCALLSTACK("CClient::addItemName");
	// NOTE: CanSee() has already been called
	if ( !pItem )
		return;

	TCHAR szName[MAX_ITEM_NAME_SIZE];
	size_t len = strcpylen(szName, pItem->GetNameFull(IsPriv(PRIV_GM) || pItem->IsAttr(ATTR_IDENTIFIED)), sizeof(szName));

	const CContainer *pCont = dynamic_cast<const CContainer *>(pItem);
	if ( pCont )
	{
		// Don't show content info if container is a corpse on ground or another player backpack
		if ( ((pItem->GetEquipLayer() != LAYER_PACK) || (pItem->GetParentObj() == m_pChar)) && !pItem->IsType(IT_CORPSE) && !pItem->IsType(IT_BBOARD) && (len < sizeof(szName)) )
			len += snprintf(szName + len, sizeof(szName) - len, g_Cfg.GetDefaultMsg(DEFMSG_CONT_ITEMS), pCont->GetCount(), pCont->GetTotalWeight() / WEIGHT_UNITS);
	}
	else if ( pItem->IsTypeArmorWeapon() )
	{
		WORD wPercent = pItem->Armor_GetRepairPercent();
		if ( (wPercent < 50) && (m_pChar->Skill_GetAdjusted(SKILL_ARMSLORE) / 10 > wPercent) && (len < sizeof(szName)) )
			len += snprintf(szName + len, sizeof(szName) - len, " (%s)", pItem->Armor_GetRepairDesc());
	}

	const CItemContainer *pParentCont = dynamic_cast<const CItemContainer *>(pItem->GetParent());
	if ( pParentCont && pParentCont->IsType(IT_EQ_VENDOR_BOX) )
	{
		const CItemVendable *pVendItem = dynamic_cast<const CItemVendable *>(pItem);
		if ( pVendItem && (len < sizeof(szName)) )
			len += snprintf(szName + len, sizeof(szName) - len, " (%" FMTDWORD " gp)", pVendItem->GetBasePrice());
	}

	HUE_TYPE wHue = HUE_TEXT_DEF;
	const CItemCorpse *pCorpseItem = dynamic_cast<const CItemCorpse *>(pItem);
	if ( pCorpseItem )
	{
		CChar *pCharCorpse = pCorpseItem->m_uidLink.CharFind();
		if ( pCharCorpse )
			wHue = pCharCorpse->Noto_GetHue(m_pChar);
	}

	if ( IsPriv(PRIV_GM) )
	{
		if ( pItem->IsAttr(ATTR_INVIS) && (len < sizeof(szName)) )
			len += snprintf(szName + len, sizeof(szName) - len, " (invis)");
		if ( pParentCont && pParentCont->IsType(IT_EQ_VENDOR_BOX) && (len < sizeof(szName)) )
			len += snprintf(szName + len, sizeof(szName) - len, " (%hhu restock)", pItem->GetContainedLayer());

		switch ( pItem->GetType() )
		{
			case IT_SPAWN_CHAR:
			case IT_SPAWN_ITEM:
			{
				const CItemSpawn *pSpawn = dynamic_cast<const CItemSpawn *>(pItem);
				if ( pSpawn )
					len += pSpawn->GetName(szName + len);
				break;
			}
			case IT_TREE:
			case IT_GRASS:
			case IT_ROCK:
			case IT_WATER:
			{
				CResourceDef *pResDef = g_Cfg.ResourceGetDef(pItem->m_itResource.m_rid_res);
				if ( pResDef && (len < sizeof(szName)) )
					len += snprintf(szName + len, sizeof(szName) - len, " (%s)", pResDef->GetName());
				break;
			}
			default:
				break;
		}
	}

	if ( IsPriv(PRIV_DEBUG) && (len < sizeof(szName)) )
		len += snprintf(szName + len, sizeof(szName) - len, " [0%" FMTDWORDH "]", static_cast<DWORD>(pItem->GetUID()));

	if ( IsTrigUsed(TRIGGER_AFTERCLICK) || IsTrigUsed(TRIGGER_ITEMAFTERCLICK) )
	{
		CScriptTriggerArgs Args(this);
		Args.m_VarsLocal.SetStrNew("ClickMsgText", &szName[0]);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", static_cast<INT64>(wHue));

		if ( const_cast<CItem *>(pItem)->OnTrigger(ITRIG_AfterClick, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;

		LPCTSTR pszNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");
		if ( pszNewStr )
		{
			strncpy(szName, pszNewStr, sizeof(szName));
			szName[sizeof(szName) - 1] = '\0';
		}

		wHue = static_cast<HUE_TYPE>(Args.m_VarsLocal.GetKeyNum("ClickMsgHue"));
	}

	addObjMessage(szName, pItem, wHue, TALKMODE_ITEM);
}

void CClient::addCharName(const CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addCharName");
	// Single click text for a character
	ASSERT(pChar);

	TCHAR szPrefix[MAX_NAME_SIZE];
	strncpy(szPrefix, pChar->GetKeyStr("NAME.PREFIX"), sizeof(szPrefix));
	szPrefix[sizeof(szPrefix) - 1] = '\0';

	if ( !*szPrefix )
		strncat(szPrefix, pChar->Noto_GetFameTitle(), sizeof(szPrefix) - strlen(szPrefix) - 1);

	TCHAR szSuffix[MAX_NAME_SIZE];
	size_t len = strcpylen(szSuffix, pChar->GetKeyStr("NAME.SUFFIX"), sizeof(szSuffix));

	if ( pChar->m_pPlayer )
	{
		LPCTSTR pszGuildAbbrev = pChar->Guild_Abbrev(MEMORY_GUILD);
		if ( pszGuildAbbrev && (len < sizeof(szSuffix)) )
			snprintf(szSuffix + len, sizeof(szSuffix) - len, " [%s]", pszGuildAbbrev);
		else
		{
			pszGuildAbbrev = pChar->Guild_Abbrev(MEMORY_TOWN);
			if ( pszGuildAbbrev && (len < sizeof(szSuffix)) )
				snprintf(szSuffix + len, sizeof(szSuffix) - len, " [%s]", pszGuildAbbrev);
		}
	}
	else if ( g_Cfg.m_fVendorTradeTitle && (pChar->GetNPCBrain() == NPCBRAIN_HUMAN) )
	{
		LPCTSTR pszTradeTitle = pChar->GetTradeTitle();
		if ( pszTradeTitle && (len < sizeof(szSuffix)) )
			snprintf(szSuffix + len, sizeof(szSuffix) - len, " %s", pszTradeTitle);
	}

	TCHAR szName[MAX_NAME_SIZE];
	snprintf(szName, sizeof(szName), "%s%s%s", szPrefix, pChar->GetName(), szSuffix);

	HUE_TYPE wHue = pChar->Noto_GetHue(m_pChar);
	bool fAllShow = IsPriv(PRIV_DEBUG|PRIV_ALLSHOW);

	if ( g_Cfg.m_fCharTags || fAllShow )
	{
		if ( pChar->m_pNPC )
		{
			if ( pChar->IsPlayableCharacter() )
				strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_NPC), sizeof(szName) - strlen(szName) - 1);
			if ( pChar->IsStatFlag(STATF_Conjured) )
				strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SUMMONED), sizeof(szName) - strlen(szName) - 1);
			else if ( pChar->IsStatFlag(STATF_Pet) )
				strncat(szName, g_Cfg.GetDefaultMsg(pChar->m_pNPC->m_bonded ? DEFMSG_CHARINFO_BONDED : DEFMSG_CHARINFO_TAME), sizeof(szName) - strlen(szName) - 1);
		}
		if ( pChar->IsStatFlag(STATF_INVUL) && !pChar->IsStatFlag(STATF_Incognito) && !pChar->IsPriv(PRIV_PRIV_NOSHOW) )
			strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_INVUL), sizeof(szName) - strlen(szName) - 1);
		if ( pChar->IsStatFlag(STATF_Stone) )
			strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_STONE), sizeof(szName) - strlen(szName) - 1);
		if ( pChar->IsStatFlag(STATF_Freeze) )
			strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_FROZEN), sizeof(szName) - strlen(szName) - 1);
		if ( pChar->IsStatFlag(STATF_Insubstantial|STATF_Invisible|STATF_Hidden) )
			strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HIDDEN), sizeof(szName) - strlen(szName) - 1);
		if ( pChar->IsStatFlag(STATF_Hallucinating) )
			strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HALLU), sizeof(szName) - strlen(szName) - 1);

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_Spawned) )
				strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SPAWN), sizeof(szName) - strlen(szName) - 1);
			if ( IsPriv(PRIV_DEBUG) )
			{
				len = strlen(szName);
				if ( len < sizeof(szName) )
					snprintf(szName + len, sizeof(szName) - len, " [0%" FMTDWORDH "]", static_cast<DWORD>(pChar->GetUID()));
			}
		}
	}
	if ( pChar->IsPriv(PRIV_JAILED) )
		strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_JAILED), sizeof(szName) - strlen(szName) - 1);
	if ( pChar->IsDisconnected() )
		strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_LOGOUT), sizeof(szName) - strlen(szName) - 1);
	if ( (fAllShow || (pChar == m_pChar)) && pChar->IsStatFlag(STATF_Criminal) )
		strncat(szName, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_CRIMINAL), sizeof(szName) - strlen(szName) - 1);

#ifdef _DEBUG
	if ( fAllShow || (IsPriv(PRIV_GM) && (g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE)) )
#else
	if ( fAllShow )
#endif
	{
		len = strlen(szName);
		if ( len < sizeof(szName) )
			snprintf(szName + len, sizeof(szName) - len, " [%s]", pChar->Skill_GetName());
	}

	if ( IsTrigUsed(TRIGGER_AFTERCLICK) )
	{
		CScriptTriggerArgs Args(this);
		Args.m_VarsLocal.SetStrNew("ClickMsgText", szName);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", static_cast<INT64>(wHue));

		if ( const_cast<CChar *>(pChar)->OnTrigger(CTRIG_AfterClick, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;

		LPCTSTR pszNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");
		if ( pszNewStr )
		{
			strncpy(szName, pszNewStr, sizeof(szName));
			szName[sizeof(szName) - 1] = '\0';
		}

		wHue = static_cast<HUE_TYPE>(Args.m_VarsLocal.GetKeyNum("ClickMsgHue"));
	}

	addObjMessage(szName, pChar, wHue, TALKMODE_ITEM);
}

void CClient::addPlayerStart(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addPlayerStart");

	if ( m_pChar != pChar )
	{
		// This must be a CONTROL command?
		CharDisconnect();
		if ( pChar->m_pClient )		// not sure why this would happen but take care of it anyhow
			pChar->m_pClient->CharDisconnect();
		m_pChar = pChar;
		m_pChar->ClientAttach(this);
	}
	ASSERT(m_pChar->m_pPlayer);

	CItem *pLingerMemory = m_pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( pLingerMemory )
		pLingerMemory->Delete();

	CPointMap pt = m_pChar->GetTopPoint();
	CSector *pSector = pt.GetSector();
	ClearTargMode();

	new PacketPlayerStart(this);
	addMapDiff();
	m_pChar->MoveToChar(pt);	// make sure we are in active list
	m_pChar->Update(true, this);
	addReSync();
	addPlayerWarMode();
	addLoginComplete();
	addTime(true);
	if ( pSector )
		addSeason(pSector->GetSeason());
	if ( m_pChar->m_pParty )
		m_pChar->m_pParty->SendAddList(NULL);

	addSpeedMode(m_pChar->m_pPlayer->m_speedMode);
	addKRToolbar(m_pChar->m_pPlayer->m_fKRToolbarEnabled);
	resendBuffs();

	Event_SpecialMoveSelect(0);
	addSpecialMoveClear();

	if ( g_Cfg.m_iChatFlags & CHATF_GLOBALCHAT )
	{
		addGlobalChatConnect();
		addGlobalChatStatusToggle();
	}
}

void CClient::addPlayerWarMode()
{
	ADDTOCALLSTACK("CClient::addPlayerWarMode");
	new PacketWarMode(this, m_pChar);
}

void CClient::addToolTip(const CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addToolTip");
	if ( !pObj || pObj->IsChar() )
		return;

	TCHAR szName[MAX_ITEM_NAME_SIZE];
	snprintf(szName, sizeof(szName), "'%s'", pObj->GetName());

	new PacketTooltip(this, pObj, szName);
}

bool CClient::addBookOpen(CItem *pBook)
{
	ADDTOCALLSTACK("CClient::addBookOpen");
	// Word wrap is done when typed in the client (it has a var size font)
	if ( !pBook )
		return false;

	WORD wPages = 0;

	if ( !pBook->IsBookSystem() )
	{
		// User written book
		CItemMessage *pMsgItem = dynamic_cast<CItemMessage *>(pBook);
		if ( !pMsgItem )
			return false;

		if ( pMsgItem->IsBookWritable() )
			wPages = pMsgItem->GetPageCount();
	}

	if ( PacketDisplayBookNew::CanSendTo(m_NetState) )
		new PacketDisplayBookNew(this, pBook);
	else
		new PacketDisplayBook(this, pBook);

	if ( wPages > 0 )
		addBookPage(pBook, 1, wPages);

	return true;
}

void CClient::addBookPage(const CItem *pBook, WORD wPage, WORD wCount)
{
	ADDTOCALLSTACK("CClient::addBookPage");
	// ARGS:
	//  wPage = 1 based page
	if ( wPage <= 0 )
		return;
	if ( wCount < 1 )
		wCount = 1;

	new PacketBookPageContent(this, pBook, wPage, wCount);
}

BYTE CClient::Setup_FillCharList(Packet *pPacket)
{
	ADDTOCALLSTACK("CClient::Setup_FillCharList");
	// List available chars on account
	ASSERT(m_pAccount);

	size_t iQty = m_pAccount->m_Chars.GetCharCount();
	BYTE iMax = m_pAccount->GetMaxChars();
	if ( iQty > iMax )
		iQty = iMax;

	size_t iCount = 0;
	for ( size_t i = 0; i < iQty; ++i )
	{
		CGrayUID uid = m_pAccount->m_Chars.GetChar(i);
		CChar *pChar = uid.CharFind();
		if ( !pChar )
			continue;

		m_tmSetupCharList[iCount] = uid;
		pPacket->writeStringFixedASCII(pChar->GetName(), MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
		++iCount;
	}

	size_t iClientMin = 5;
	if ( m_NetState->isClientVersion(MINCLIVER_PADCHARLIST) || !m_NetState->getCryptVersion() )
		iClientMin = maximum(iQty, 5);

	for ( ; iCount < iClientMin; ++iCount )
	{
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
	}
	return static_cast<BYTE>(iCount);
}

void CClient::SetTargMode(CLIMODE_TYPE targmode, LPCTSTR pszPrompt, int iTimeout)
{
	ADDTOCALLSTACK("CClient::SetTargMode");
	// ??? Get rid of menu stuff if previous targ mode
	// Can i close a menu?
	// Cancel a cursor input
	if ( !m_pChar )
		return;

	bool fSuppressCancelMessage = false;

	switch ( GetTargMode() )
	{
		case CLIMODE_TARG_OBJ_FUNC:
		{
			if ( IsTrigUsed(TRIGGER_TARGON_CANCEL) )
			{
				CScriptTriggerArgs Args;
				Args.m_s1 = m_Targ_Text;
				if ( m_pChar->OnTrigger(CTRIG_Targon_Cancel, m_pChar, &Args) == TRIGRET_RET_TRUE )
					fSuppressCancelMessage = true;
			}
			break;
		}
		case CLIMODE_TARG_USE_ITEM:
		{
			CItem *pItemUse = m_Targ_UID.ItemFind();
			if ( pItemUse && (IsTrigUsed(TRIGGER_TARGON_CANCEL) || IsTrigUsed(TRIGGER_ITEMTARGON_CANCEL)) )
			{
				if ( pItemUse->OnTrigger(ITRIG_TARGON_CANCEL, m_pChar) == TRIGRET_RET_TRUE )
					fSuppressCancelMessage = true;
			}
			break;
		}
		case CLIMODE_TARG_SKILL_MAGERY:
		{
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_Spell);
			if ( m_pChar && pSpellDef )
			{
				CScriptTriggerArgs Args(m_tmSkillMagery.m_Spell, 0, m_Targ_PrvUID.ObjFind());

				if ( IsTrigUsed(TRIGGER_SPELLTARGETCANCEL) )
				{
					if ( m_pChar->OnTrigger(CTRIG_SpellTargetCancel, this, &Args) == TRIGRET_RET_TRUE )
					{
						fSuppressCancelMessage = true;
						break;
					}
				}

				if ( IsTrigUsed(TRIGGER_TARGETCANCEL) )
				{
					if ( m_pChar->Spell_OnTrigger(m_tmSkillMagery.m_Spell, SPTRIG_TARGETCANCEL, m_pChar, &Args) == TRIGRET_RET_TRUE )
						fSuppressCancelMessage = true;
				}
			}
			break;
		}
		case CLIMODE_TARG_SKILL:
		case CLIMODE_TARG_SKILL_HERD_DEST:
		case CLIMODE_TARG_SKILL_PROVOKE:
		case CLIMODE_TARG_SKILL_POISON:
		{
			SKILL_TYPE action = SKILL_NONE;
			switch ( GetTargMode() )
			{
				case CLIMODE_TARG_SKILL:
					action = m_tmSkillTarg.m_Skill;
					break;
				case CLIMODE_TARG_SKILL_HERD_DEST:
					action = SKILL_HERDING;
					break;
				case CLIMODE_TARG_SKILL_PROVOKE:
					action = SKILL_PROVOCATION;
					break;
				case CLIMODE_TARG_SKILL_POISON:
					action = SKILL_POISONING;
					break;
				default:
					break;
			}
			if ( m_pChar && (action != SKILL_NONE) )
			{
				if ( IsTrigUsed(TRIGGER_SKILLTARGETCANCEL) )
				{
					if ( m_pChar->Skill_OnCharTrigger(action, CTRIG_SkillTargetCancel) == TRIGRET_RET_TRUE )
					{
						fSuppressCancelMessage = true;
						break;
					}
				}
				if ( IsTrigUsed(TRIGGER_TARGETCANCEL) )
				{
					if ( m_pChar->Skill_OnTrigger(action, SKTRIG_TARGETCANCEL) == TRIGRET_RET_TRUE )
						fSuppressCancelMessage = true;
				}
			}
			break;
		}
		default:
			break;
	}

	if ( iTimeout > 0 )
		m_Targ_Timeout = CServTime::GetCurrentTime() + iTimeout;
	else
		m_Targ_Timeout.Init();

	if ( GetTargMode() == targmode )
		return;

	if ( (GetTargMode() != CLIMODE_NORMAL) && (targmode != CLIMODE_NORMAL) )
	{
		// If there's any item in LAYER_DRAGGING, remove it from view (to avoid seeing it in the cursor) and then bounce it
		CItem *pItem = m_pChar->LayerFind(LAYER_DRAGGING);
		if ( pItem )
		{
			pItem->RemoveFromView();
			m_pChar->ItemBounce(pItem);
			if ( !fSuppressCancelMessage )
				addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_2));
		}
	}

	m_Targ_Mode = targmode;
	if ( (targmode == CLIMODE_NORMAL) && !fSuppressCancelMessage )
		addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_1));
	else if ( pszPrompt && *pszPrompt )
		addSysMessage(pszPrompt);
}

void CClient::addPromptConsole(CLIMODE_TYPE mode, LPCTSTR pszPrompt, CGrayUID context1, CGrayUID context2, bool fUnicode)
{
	ADDTOCALLSTACK("CClient::addPromptConsole");

	m_Prompt_Uid = context1;
	m_Prompt_Mode = mode;

	if ( pszPrompt && *pszPrompt )
		addSysMessage(pszPrompt);

	new PacketAddPrompt(this, context1, context2, fUnicode);
}

void CClient::addTarget(CLIMODE_TYPE mode, LPCTSTR pszPrompt, bool fAllowGround, bool fCheckCrime, int iTimeout)
{
	ADDTOCALLSTACK("CClient::addTarget");
	// Send targetting cursor to client
	// Expect XCMD_Target back.
	// ??? will this be selective for us? objects only or chars only? not on the ground (statics)?

	SetTargMode(mode, pszPrompt, iTimeout);
	new PacketAddTarget(this, fAllowGround ? PacketAddTarget::Ground : PacketAddTarget::Object, mode, fCheckCrime ? PacketAddTarget::Harmful : PacketAddTarget::None);
}

void CClient::addTargetDeed(const CItem *pDeed)
{
	ADDTOCALLSTACK("CClient::addTargetDeed");
	// Place an item from a deed. Preview all the stuff

	ASSERT(m_Targ_UID == pDeed->GetUID());
	m_tmUseItem.m_pParent = pDeed->GetParent();		// cheat verify
	addTargetItems(CLIMODE_TARG_USE_ITEM, static_cast<ITEMID_TYPE>(RES_GET_INDEX(pDeed->m_itDeed.m_Type)), pDeed->GetHue());
}

bool CClient::addTargetChars(CLIMODE_TYPE mode, CREID_TYPE baseID, bool fCheckCrime, int iTimeout)
{
	ADDTOCALLSTACK("CClient::addTargetChars");
	CCharBase *pBase = CCharBase::FindCharBase(baseID);
	if ( !pBase )
		return false;

	TCHAR szPrompt[EXPRESSION_MAX_KEY_LEN];
	snprintf(szPrompt, sizeof(szPrompt), "%s %s?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_SUMMON), pBase->GetTradeName());

	addTarget(mode, szPrompt, true, fCheckCrime, iTimeout);
	return true;
}

bool CClient::addTargetItems(CLIMODE_TYPE mode, ITEMID_TYPE id, HUE_TYPE wHue, bool fAllowGround)
{
	ADDTOCALLSTACK("CClient::addTargetItems");
	// Add a list of items to place at target. Preview all the stuff

	ASSERT(m_pChar);

	LPCTSTR pszName;
	CItemBase *pItemDef;
	if ( id < ITEMID_TEMPLATE )
	{
		pItemDef = CItemBase::FindItemBase(id);
		if ( !pItemDef )
			return false;

		pszName = pItemDef->GetName();
		if ( !pszName )
			pszName = "item";

		if ( pItemDef->IsType(IT_STONE_GUILD) )
		{
			// Check if they are already in a guild first
			CItemStone *pStone = m_pChar->Guild_Find(MEMORY_GUILD);
			if ( pStone )
			{
				addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_GUILD_ALREADY_MEMBER));
				return false;
			}
		}
	}
	else
	{
		pItemDef = NULL;
		pszName = "template";
	}

	TCHAR szPrompt[EXPRESSION_MAX_KEY_LEN];
	snprintf(szPrompt, sizeof(szPrompt), "%s %s?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_PLACE), pszName);

	if ( CItemBase::IsID_Multi(id) )	// a multi we get from multi.mul
	{
		SetTargMode(mode, szPrompt);
		new PacketAddTarget(this, fAllowGround ? PacketAddTarget::Ground : PacketAddTarget::Object, mode, PacketAddTarget::None, id, wHue);
		return true;
	}

	// Preview not supported by this ver?
	addTarget(mode, szPrompt, true);
	return true;
}

void CClient::addTargetCancel()
{
	ADDTOCALLSTACK("CClient::addTargetCancel");
	// Handle target cancellation. In an ideal world the client would normally respond to
	// the cancel target packet as if the user had pressed ESC, but we shouldn't rely on
	// this happening (older clients for example don't support the cancel command and will
	// bring up a new target cursor)

	SetTargMode();
	new PacketAddTarget(this, PacketAddTarget::Object, 0, PacketAddTarget::Cancel);
}

void CClient::addCodexOfWisdom(DWORD dwTopicID, bool fForceOpen)
{
	ADDTOCALLSTACK("CClient::addCodexOfWisdom");
	// Open Codex of Wisdom

	new PacketCodexOfWisdom(this, dwTopicID, fForceOpen);
}

void CClient::addDyeOption(const CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addDyeOption");
	// Put up the hue chart for the client
	// This results in a Event_Item_Dye message. CLIMODE_DYE

	SetTargMode(CLIMODE_DYE);
	new PacketShowDyeWindow(this, pObj);
}

void CClient::addSkillWindow(SKILL_TYPE skill, bool fFromInfo)
{
	ADDTOCALLSTACK("CClient::addSkillWindow");
	// Whos skills do we want to look at?
	CChar *pChar = m_Prop_UID.CharFind();
	if ( !pChar )
		pChar = m_pChar;

	bool fAllSkills = (skill >= static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill));
	if ( !fAllSkills && ((skill <= SKILL_NONE) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill)) )
		return;

	if ( IsTrigUsed(TRIGGER_USERSKILLS) )
	{
		CScriptTriggerArgs Args(fAllSkills ? -1 : skill, fFromInfo);
		if ( m_pChar->OnTrigger(CTRIG_UserSkills, pChar, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( !fAllSkills && (skill >= SKILL_QTY) )
		return;

	new PacketSkills(this, pChar, skill);
}

void CClient::addSpecialMoveClear()
{
	ADDTOCALLSTACK("CClient::addSpecialMoveClear");
	// Clear current special move selected

	new PacketSpecialMoveClear(this);
}

void CClient::addPlayerSee(const CPointMap ptOld, bool fIgnoreSelfRegion)
{
	ADDTOCALLSTACK("CClient::addPlayerSee");
	// Adjust to my new location. What do I see here?
	// ARGS:
	//  ptOld = previous pt, used to calculate which objects will enter/exit view sight (NULL = resend everything)
	//  fIgnoreSelfRegion = ignore objects on same region, used by HS boat mouse movement feature which doesn't need to reload objects inside the boat

	int iViewDist = m_pChar->GetSight();
	CRegionBase *pCurrentCharRegion = m_pChar->GetTopPoint().GetRegion(REGION_TYPE_HOUSE);

	// Nearby items on ground
	CItem *pItem = NULL;
	int iOldDist = 0;
	unsigned int iSeeCurrent = 0;
	unsigned int iSeeMax = g_Cfg.m_iMaxItemComplexity * 30;

	CWorldSearch AreaItems(m_pChar->GetTopPoint(), UO_MAP_VIEW_RADAR);
	for (;;)
	{
		pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( fIgnoreSelfRegion && (m_pChar->GetTopPoint().GetRegion(REGION_TYPE_MULTI) == pItem->GetTopPoint().GetRegion(REGION_TYPE_MULTI)) )
			continue;

		iOldDist = ptOld.GetDist(pItem->GetTopPoint());
		if ( (iOldDist > UO_MAP_VIEW_RADAR) && pItem->IsTypeMulti() )		// incoming multi on radar view
		{
			addItem_OnGround(pItem);
			continue;
		}

		if ( (iSeeCurrent > iSeeMax) || !m_pChar->CanSee(pItem) )
			continue;

		if ( m_UseMultiSight )
		{
			if ( (((ptOld.GetRegion(REGION_TYPE_HOUSE) != pCurrentCharRegion) || (ptOld.GetDist(pItem->GetTopPoint()) > iViewDist)) && (pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_HOUSE) == pCurrentCharRegion))		// item is in same house as me
				|| (((ptOld.GetDist(pItem->GetTopPoint()) > iViewDist) && (m_pChar->GetTopPoint().GetDist(pItem->GetTopPoint()) <= iViewDist))	// item just came into view
					&& (!pItem->GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_HOUSE)		// item is not in a house (ships are ok)
						|| (pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsItem() && pItem->m_uidLink.ItemFind()->IsTypeMulti())		// item is linked to a multi
						|| pItem->IsTypeMulti()		// item is an multi
						|| pItem->Can(CAN_I_BLOCK)	// item can block movement
						|| pItem->GetKeyNum("ALWAYSSEND", true))) )	// item has TAG.ALWAYSSEND set
			{
				++iSeeCurrent;
				addItem_OnGround(pItem);
			}
		}
		else
		{
			if ( (iOldDist > iViewDist) && (m_pChar->GetTopPoint().GetDist(pItem->GetTopPoint()) <= iViewDist) )		// item just came into view
			{
				++iSeeCurrent;
				addItem_OnGround(pItem);
			}
		}
	}

	// Nearby chars
	CChar *pChar = NULL;
	iSeeCurrent = 0;
	iSeeMax = g_Cfg.m_iMaxCharComplexity * 5;

	CWorldSearch AreaChars(m_pChar->GetTopPoint(), iViewDist);
	AreaChars.SetAllShow(IsPriv(PRIV_ALLSHOW));
	for (;;)
	{
		pChar = AreaChars.GetChar();
		if ( !pChar || (iSeeCurrent > iSeeMax) )
			break;
		if ( fIgnoreSelfRegion && (m_pChar->m_pArea == pChar->m_pArea) )
			continue;
		if ( (m_pChar == pChar) || !CanSee(pChar) )
			continue;

		if ( ptOld.GetDist(pChar->GetTopPoint()) > iViewDist )
		{
			++iSeeCurrent;
			addChar(pChar);
		}
	}
}

void CClient::addPlayerView(const CPointMap ptOld, bool fFull)
{
	ADDTOCALLSTACK("CClient::addPlayerView");
	// I moved = Change my point of view. Teleport etc..

	addPlayerUpdate();

	if ( m_pChar->GetTopPoint().IsSame2D(ptOld) )
		return;		// not a real move i guess, might just have been a change in face dir

	m_Env.SetInvalid();		// must resend environ stuff

	if ( fFull )
		addPlayerSee(ptOld);
}

void CClient::addReSync()
{
	ADDTOCALLSTACK("CClient::addReSync");
	// Reload the client with all it needs
	if ( !m_pChar )
		return;

	addMap();
	addChar(m_pChar);
	addPlayerView(NULL);
	addLight();
	addWeather();
	addHealthBarInfo(m_pChar);
}

void CClient::addMap()
{
	ADDTOCALLSTACK("CClient::addMap");

	CPointMap pt = m_pChar->GetTopPoint();
	new PacketMapChange(this, static_cast<BYTE>(g_MapList.m_mapid[pt.m_map]));
}

void CClient::addMapDiff()
{
	ADDTOCALLSTACK("CClient::addMapDiff");
	// Enables map diff usage on the client. If the client is told to
	// enable diffs, and then logs back in without restarting, it will
	// continue to use the diffs even if not told to enable them - so
	// this packet should always be sent even if empty.

	new PacketEnableMapDiffs(this);
}

void CClient::addMapWaypoint(CObjBase *pObj, MAPWAYPOINT_TYPE type)
{
	ADDTOCALLSTACK("CClient::addMapWaypoint");
	// Add/remove map waypoints

	if ( type )
	{
		// Classic clients only support MAPWAYPOINT_Remove and MAPWAYPOINT_Healer
		if ( (type != MAPWAYPOINT_Healer) && !m_NetState->isClientKR() && !m_NetState->isClientEnhanced() )
			return;

		if ( PacketWaypointAdd::CanSendTo(m_NetState) )
			new PacketWaypointAdd(this, pObj, type);
	}
	else
	{
		if ( PacketWaypointRemove::CanSendTo(m_NetState) )
			new PacketWaypointRemove(this, pObj);
	}
}

void CClient::addChangeServer()
{
	ADDTOCALLSTACK("CClient::addChangeServer");

	CPointMap pt = m_pChar->GetTopPoint();
	new PacketZoneChange(this, pt);
}

void CClient::addPlayerUpdate()
{
	ADDTOCALLSTACK("CClient::addPlayerUpdate");
	// Update player character on screen (id / hue / notoriety / position / dir).
	// NOTE: This will reset client-side walk sequence to 0, so reset it on server
	// side too, to prevent client request an unnecessary 'resync' (packet 0x22)
	// to server because client seq != server seq.

	new PacketPlayerUpdate(this);
	m_NetState->m_sequence = 0;
}

void CClient::UpdateStats()
{
	ADDTOCALLSTACK("CClient::UpdateStats");
	if ( !m_fUpdateStats || !m_pChar )
		return;

	if ( m_fUpdateStats & SF_UPDATE_STATUS )
	{
		addHealthBarInfo(m_pChar);
		m_fUpdateStats = 0;
		return;
	}
	if ( m_fUpdateStats & SF_UPDATE_HITS )
	{
		addHitsUpdate(m_pChar);
		m_fUpdateStats &= ~SF_UPDATE_HITS;
	}
	if ( m_fUpdateStats & SF_UPDATE_MANA )
	{
		addManaUpdate(m_pChar);
		m_fUpdateStats &= ~SF_UPDATE_MANA;
	}
	if ( m_fUpdateStats & SF_UPDATE_STAM )
	{
		addStamUpdate(m_pChar);
		m_fUpdateStats &= ~SF_UPDATE_STAM;
	}
}

void CClient::addHealthBarInfo(CObjBase *pObj, bool fRequested)
{
	ADDTOCALLSTACK("CClient::addHealthBarInfo");
	if ( !pObj )
		return;

	if ( IsTrigUsed(TRIGGER_USERSTATS) && pObj->IsChar() )
	{
		CScriptTriggerArgs Args(0, 0, pObj);
		Args.m_iN3 = fRequested;
		if ( m_pChar->OnTrigger(CTRIG_UserStats, dynamic_cast<CTextConsole *>(pObj), &Args) == TRIGRET_RET_TRUE )
			return;
	}

	new PacketHealthBarInfo(this, pObj);
	if ( pObj == m_pChar )
	{
		m_fUpdateStats = 0;
		if ( PacketStatLocks::CanSendTo(m_NetState) )
			new PacketStatLocks(this, m_pChar);
	}
}

void CClient::addHitsUpdate(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addHitsUpdate");
	if ( !pChar )
		return;

	PacketHealthUpdate cmd(pChar, pChar == m_pChar);
	cmd.send(this);
}

void CClient::addManaUpdate(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addManaUpdate");
	if ( !pChar )
		return;

	PacketManaUpdate cmd(pChar, true);
	cmd.send(this);

	if ( pChar->m_pParty )
	{
		PacketManaUpdate cmd2(pChar, false);
		pChar->m_pParty->StatsUpdateAll(pChar, &cmd2);
	}
}

void CClient::addStamUpdate(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addStamUpdate");
	if ( !pChar )
		return;

	PacketStaminaUpdate cmd(pChar, true);
	cmd.send(this);

	if ( pChar->m_pParty )
	{
		PacketStaminaUpdate cmd2(pChar, false);
		pChar->m_pParty->StatsUpdateAll(pChar, &cmd2);
	}
}

void CClient::addHealthBarUpdate(const CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addHealthBarUpdate");
	if ( !pChar )
		return;

	if ( PacketHealthBarUpdateNew::CanSendTo(m_NetState) )
		new PacketHealthBarUpdateNew(this, pChar);
	else if ( PacketHealthBarUpdate::CanSendTo(m_NetState) )
		new PacketHealthBarUpdate(this, pChar);
}

void CClient::addBondedStatus(const CChar *pChar, bool fGhost)
{
	ADDTOCALLSTACK("CClient::addBondedStatus");
	if ( !pChar )
		return;

	new PacketBondedStatus(this, pChar, fGhost);
}

void CClient::addSpellbookOpen(CItem *pBook)
{
	ADDTOCALLSTACK("CClient::addSpellbookOpen");
	// Open the spellbook content and fill it with some data.
	// NOTE: New spellbook types needs tooltip feature enabled to display gump content.
	//		 Enhanced clients need tooltip on all spellbook types otherwise it will crash.
	//		 Clients can also crash if open spellbook gump when spellbook item is not loaded yet.

	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_SPELLBOOK) )
	{
		CScriptTriggerArgs Args(0, 0, pBook);
		if ( m_pChar->OnTrigger(CTRIG_SpellBook, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	addItem(pBook);
	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump(pBook, GUMP_NONE);

	if ( PacketSpellbookContent::CanSendTo(m_NetState) )
	{
		CItemBase *pBookDef = pBook->Item_GetDef();
		if ( pBookDef )
			new PacketSpellbookContent(this, pBook, static_cast<WORD>(pBookDef->m_ttSpellbook.m_Offset + 1));
	}
	else
		new PacketItemContents(this, pBook);
}

void CClient::addCustomSpellbookOpen(CItem *pBook, GUMP_TYPE gumpID)
{
	ADDTOCALLSTACK("CClient::addCustomSpellbookOpen");
	const CItemContainer *pContainer = dynamic_cast<CItemContainer *>(pBook);
	if ( !pContainer )
		return;

	size_t iCount = 0;
	for ( CItem *pItem = pContainer->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsType(IT_SCROLL) )
			continue;
		++iCount;
	}

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump(pBook, gumpID);
	if ( iCount > 0 )
		new PacketItemContents(this, pContainer);
}

void CClient::addScrollScript(CResourceLock &s, SCROLL_TYPE type, DWORD dwContext, LPCTSTR pszHeader)
{
	ADDTOCALLSTACK("CClient::addScrollScript");

	new PacketOpenScroll(this, s, type, dwContext, pszHeader);
}

void CClient::addScrollResource(LPCTSTR pszName, SCROLL_TYPE type, DWORD dwScrollID)
{
	ADDTOCALLSTACK("CClient::addScrollResource");

	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, RES_SCROLL, pszName) )
		return;
	addScrollScript(s, type, dwScrollID);
}

void CClient::addVendorClose(const CChar *pVendor)
{
	ADDTOCALLSTACK("CClient::addVendorClose");
	new PacketCloseVendor(this, pVendor);
}

void CClient::addShopMenuBuy(CChar *pVendor)
{
	ADDTOCALLSTACK("CClient::addShopMenuBuy");
	// Try to buy stuff that the vendor has
	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return;

	if ( !g_Cfg.m_bAllowBuySellAgent )
	{
		const CVarDefCont *pVar = m_TagDefs.GetKey("BUYSELLTIME");
		if ( pVar )
		{
			CServTime timeDelay;
			timeDelay.InitTime(pVar->GetValNum() + 3);
			if ( g_World.GetCurrentTime() < timeDelay )
			{
				pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_WAIT));
				return;
			}
		}
		m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
	}

	//OpenPacketTransaction transaction(this, PacketSend::PRI_HIGH);

	// Non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
		pVendor->NPC_Vendor_Restock(false, true);

	CItemContainer *pContainerStock = pVendor->GetContainerCreate(LAYER_VENDOR_STOCK);
	CItemContainer *pContainerExtra = pVendor->GetContainerCreate(LAYER_VENDOR_EXTRA);

	// Get item list
	addItem(pContainerStock);	// sending the full tooltip, instead of the one with just the name
	addContents(pContainerStock, false, false, true);
	addItem(pContainerExtra);

	// Get price list
	new PacketVendorBuyList(this, pVendor, pContainerStock);

	// Open gump
	addOpenGump(pVendor, GUMP_VENDOR_RECT);
	new PacketHealthBarInfo(this, m_pChar);		// update char 'gold available' value on gump
}

void CClient::addShopMenuSell(CChar *pVendor)
{
	ADDTOCALLSTACK("CClient::addShopMenuSell");
	// Player selling to vendor
	// What things do you have in your inventory that the vendor would want?
	// Should end with a returned Event_VendorSell()
	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return;

	if ( !g_Cfg.m_bAllowBuySellAgent )
	{
		const CVarDefCont *pVar = m_TagDefs.GetKey("BUYSELLTIME");
		if ( pVar )
		{
			CServTime timeDelay;
			timeDelay.InitTime(pVar->GetValNum() + 3);
			if ( g_World.GetCurrentTime() < timeDelay )
			{
				pVendor->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_ORDER_WAIT));
				return;
			}
		}
		m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
	}

	//OpenPacketTransaction transaction(this, PacketSend::PRI_LOW);

	// Non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_Pet) )
		pVendor->NPC_Vendor_Restock(false, true);

	CItemContainer *pContainerBuy = pVendor->GetContainerCreate(LAYER_VENDOR_BUYS);
	CItemContainer *pContainerStock = pVendor->GetContainerCreate(LAYER_VENDOR_STOCK);
	addItem(pContainerBuy);
	addItem(pContainerStock);

	new PacketVendorSellList(this, pVendor, m_pChar->GetContainerCreate(LAYER_PACK), pContainerBuy);
}

void CClient::addBankOpen(CChar *pChar, LAYER_TYPE layer)
{
	ADDTOCALLSTACK("CClient::addBankOpen");
	// Open bankbox of this pChar
	ASSERT(pChar);

	CItemContainer *pBankBox = pChar->GetContainerCreate(layer);
	ASSERT(pBankBox);
	addItem(pBankBox);		//client will crash client if it try to open the gump without load the item first

	pBankBox->OnOpenEvent(m_pChar, pChar);
	addContainerSetup(pBankBox);
}

void CClient::addDrawMap(CItemMap *pMap)
{
	ADDTOCALLSTACK("CClient::addDrawMap");
	// Open cartography map on client screen
	// NOTE: Clients 7.0.8.0+ can draw maps of all facets, previous clients can only draw maps of Felucca/Trammel

	if ( !pMap )
	{
	blank_map:
		addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MAP_IS_BLANK));
		return;
	}
	if ( pMap->IsType(IT_MAP_BLANK) )
		goto blank_map;

	CRectMap rect;
	rect.SetRect(pMap->m_itMap.m_left, pMap->m_itMap.m_top, pMap->m_itMap.m_right, pMap->m_itMap.m_bottom, g_MapList.m_mapid[pMap->m_itMap.m_map]);

	if ( !rect.IsValid() )
		goto blank_map;
	if ( rect.IsRectEmpty() )
		goto blank_map;

	// Open map
	if ( PacketDisplayMapNew::CanSendTo(m_NetState) )
		new PacketDisplayMapNew(this, pMap, rect);
	else
		new PacketDisplayMap(this, pMap, rect);

	// Draw pins
	addMapMode(pMap, MAPCMD_ClearPins, false);
	PacketMapPlot plot(pMap, MAPCMD_AddPin, false);
	for ( size_t i = 0; i < pMap->m_Pins.GetCount(); ++i )
	{
		plot.setPin(pMap->m_Pins[i].m_x, pMap->m_Pins[i].m_y);
		plot.send(this);
	}
}

void CClient::addMapMode(CItemMap *pMap, MAPCMD_TYPE iType, bool fEdit)
{
	ADDTOCALLSTACK("CClient::addMapMode");
	// NOTE: MAPMODE_* depends on who is looking. Multi clients could interfere with each other?
	if ( !pMap )
		return;

	pMap->m_fPlotMode = fEdit;
	new PacketMapPlot(this, pMap, iType, fEdit);
}

void CClient::addBulletinBoard(const CItemContainer *pBoard)
{
	ADDTOCALLSTACK("CClient::addBulletinBoard");
	// Open up the bulletin board and all it's messages
	if ( !pBoard )
		return;

	new PacketBulletinBoard(this, pBoard);
	addContents(pBoard);
}

void CClient::addBBoardMessage(const CItemContainer *pBoard, BULLETINBOARD_TYPE action, CGrayUID uidMsg)
{
	ADDTOCALLSTACK("CClient::addBBoardMessage");
	if ( !pBoard )
		return;

	CItemMessage *pMsgItem = dynamic_cast<CItemMessage *>(uidMsg.ItemFind());
	if ( !pBoard->IsItemInside(pMsgItem) )
		return;

	// Check if author is properly linked
	if ( !pMsgItem->m_sAuthor.IsEmpty() && !pMsgItem->m_uidLink.CharFind() )
		return pMsgItem->Delete();

	new PacketBulletinBoard(this, action, pBoard, pMsgItem);
}

void CClient::addLoginComplete()
{
	ADDTOCALLSTACK("CClient::addLoginComplete");
	new PacketLoginComplete(this);
}

void CClient::addChatSystemMessage(CHATMSG_TYPE type, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang)
{
	ADDTOCALLSTACK("CClient::addChatSystemMessage");
	new PacketChatMessage(this, type, pszName1, pszName2, lang);
}

void CClient::addGlobalChatConnect()
{
	ADDTOCALLSTACK("CClient::addGlobalChatConnect");
	// Connect on Global Chat
	if ( !m_pChar || !PacketGlobalChat::CanSendTo(m_NetState) )
		return;

	// Set Jabber ID (syntax: CharName_CharUID@ServerID)
	TCHAR szJID[22];
	snprintf(szJID, sizeof(szJID), "%.6s_%.7" FMTDWORD "@%.2hhu", m_pChar->GetName(), static_cast<DWORD>(m_pChar->GetUID()), 0);
	CGlobalChat::SetJID(szJID);

	// Send xml to client
	TCHAR szXML[105];
	snprintf(szXML, sizeof(szXML), "<iq to=\"%s\" id=\"iq_%.10" FMTDWORD "\" type=\"6\" version=\"1\" jid=\"%s\" />", CGlobalChat::GetJID(), static_cast<DWORD>(CGTime::GetCurrentTime().GetTime()), CGlobalChat::GetJID());

	new PacketGlobalChat(this, 0, PacketGlobalChat::Connect, PacketGlobalChat::InfoQuery, szXML);
	addBarkLocalized(1158413, NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM);	// Global Chat is now connected. 
}

void CClient::addGlobalChatStatusToggle()
{
	ADDTOCALLSTACK("CClient::addGlobalChatStatusToggle");
	// Toggle client visibility status (online/offline) on Global Chat
	if ( !m_pChar || !PacketGlobalChat::CanSendTo(m_NetState) )
		return;

	int iShow = static_cast<int>(CGlobalChat::IsVisible());
	TCHAR szXML[100];
	snprintf(szXML, sizeof(szXML), "<presence from=\"%s\" id=\"pres_%.10" FMTDWORD "\" name=\"%.6s\" show=\"%d\" version=\"1\" />", CGlobalChat::GetJID(), static_cast<DWORD>(CGTime::GetCurrentTime().GetTime()), m_pChar->GetName(), iShow);

	CGlobalChat::SetVisible(static_cast<bool>(iShow));
	new PacketGlobalChat(this, 0, PacketGlobalChat::Connect, PacketGlobalChat::Presence, szXML);
	addBarkLocalized(iShow ? 1158428 : 1158427, NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM);	// Global Chat Online / Global Chat Offline

	// TO-DO: also send the status change to all clients on friend list
}

void CClient::addGumpTextDisp(const CObjBase *pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText)
{
	ADDTOCALLSTACK("CClient::addGumpTextDisp");
	new PacketSignGump(this, pObj, gump, pszName, pszText);
}

void CClient::addItemMenu(CLIMODE_TYPE mode, const CMenuItem *pMenuItem, size_t iCount, CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addItemMenu");
	// We must set GetTargMode() to show what mode we are in for menu select
	// Will result in PacketMenuChoice::onReceive()
	// cmd.ItemMenu

	if ( iCount <= 0 )
		return;
	if ( !pObj )
		pObj = m_pChar;

	new PacketDisplayMenu(this, mode, pMenuItem, iCount, pObj);
	m_tmMenu.m_UID = pObj->GetUID();
	SetTargMode(mode);
}

void CClient::addCharPaperdoll(const CChar *pChar)
{
	ADDTOCALLSTACK("CClient::addCharPaperdoll");
	if ( !pChar )
		return;

	new PacketPaperdoll(this, pChar);
}

void CClient::addAOSTooltip(const CObjBase *pObj, bool fRequested, bool fShop)
{
	ADDTOCALLSTACK("CClient::addAOSTooltip");
	if ( !pObj || !PacketPropertyList::CanSendTo(m_NetState) )
		return;

	// Check if we must send the full tooltip or just the obj name
	if ( !m_TooltipEnabled && !fShop )
		return;

	// Don't send tooltips for objects out of LOS
	const CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
	if ( !pObjTop || (m_pChar->GetTopPoint().GetDist(pObjTop->GetTopPoint()) > m_pChar->GetSight() + 1) )
		return;

	// Don't send tooltips for static items
	const CItem *pItem = dynamic_cast<const CItem *>(pObj);
	if ( pItem && (pItem->Item_GetDef()->GetTFlags() & (UFLAG1_WALL|UFLAG1_BLOCK|UFLAG2_PLATFORM)) && !IsPriv(PRIV_GM) )
		return;

	PacketPropertyList *pPropertyList = pObj->GetPropertyList();
	if ( !pPropertyList || pPropertyList->hasExpired(g_Cfg.m_iTooltipCache) )
	{
		const_cast<CObjBase *>(pObj)->FreePropertyList();

		CClientTooltip *t = NULL;
		m_TooltipData.Clean(true);

		if ( !m_TooltipEnabled )
		{
			// Tooltips are disabled on server-side but client still need it to fill vendor buy/sell menu, so send just the obj name
			m_TooltipData.InsertAt(0, t = new CClientTooltip(1042971)); // ~1_NOTHING~
			t->FormatArgs("%s", pObj->GetName());
		}
		else
		{
			const CChar *pChar = dynamic_cast<const CChar *>(pObj);
			TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
			if ( IsTrigUsed(TRIGGER_CLIENTTOOLTIP) || (pItem && IsTrigUsed(TRIGGER_ITEMCLIENTTOOLTIP)) || (pChar && IsTrigUsed(TRIGGER_CHARCLIENTTOOLTIP)) )
			{
				CScriptTriggerArgs Args(const_cast<CObjBase *>(pObj));
				Args.m_iN1 = fRequested;
				tr = const_cast<CObjBase *>(pObj)->OnTrigger("@ClientTooltip", m_pChar, &Args);	// ITRIG_CLIENTTOOLTIP, CTRIG_ClientTooltip
			}

			if ( tr != TRIGRET_RET_TRUE )
			{
				DWORD dwClilocName = static_cast<DWORD>(pObj->GetDefNum("NAMELOC", true));
				if ( pItem )
				{
					if ( dwClilocName )
					{
						m_TooltipData.InsertAt(0, new CClientTooltip(dwClilocName));
					}
					else if ( (pItem->GetAmount() > 1) && (pItem->GetType() != IT_CORPSE) )
					{
						m_TooltipData.InsertAt(0, t = new CClientTooltip(1050039)); // ~1_NUMBER~ ~2_ITEMNAME~
						t->FormatArgs("%hu\t%s", pItem->GetAmount(), pObj->GetName());
					}
					else
					{
						m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045)); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
						t->FormatArgs(" \t%s\t ", pObj->GetName());
					}
				}
				else if ( pChar )
				{
					TCHAR szPrefix[MAX_NAME_SIZE];
					strncpy(szPrefix, pChar->GetKeyStr("NAME.PREFIX"), sizeof(szPrefix));
					szPrefix[sizeof(szPrefix) - 1] = '\0';

					if ( !*szPrefix )
						strncat(szPrefix, pChar->Noto_GetFameTitle(), sizeof(szPrefix) - strlen(szPrefix) - 1);

					TCHAR szSuffix[MAX_NAME_SIZE];
					size_t len = strcpylen(szSuffix, pChar->GetKeyStr("NAME.SUFFIX"), sizeof(szSuffix));

					if ( pChar->m_pPlayer )
					{
						LPCTSTR pszGuildAbbrev = pChar->Guild_Abbrev(MEMORY_GUILD);
						if ( pszGuildAbbrev && (len < sizeof(szSuffix)) )
							snprintf(szSuffix + len, sizeof(szSuffix) - len, " [%s]", pszGuildAbbrev);
						else
						{
							pszGuildAbbrev = pChar->Guild_Abbrev(MEMORY_TOWN);
							if ( pszGuildAbbrev && (len < sizeof(szSuffix)) )
								snprintf(szSuffix + len, sizeof(szSuffix) - len, " [%s]", pszGuildAbbrev);
						}
					}
					else if ( g_Cfg.m_fVendorTradeTitle && (pChar->GetNPCBrain() == NPCBRAIN_HUMAN) )
					{
						LPCTSTR pszTradeTitle = pChar->GetTradeTitle();
						if ( pszTradeTitle && (len < sizeof(szSuffix)) )
							snprintf(szSuffix + len, sizeof(szSuffix) - len, " %s", pszTradeTitle);
					}

					if ( !*szPrefix )
						strncat(szPrefix, " ", sizeof(szPrefix) - strlen(szPrefix) - 1);
					if ( !*szSuffix )
						strncat(szSuffix, " ", sizeof(szSuffix) - strlen(szSuffix) - 1);

					m_TooltipData.InsertAt(0, t = new CClientTooltip(1050045)); // ~1_PREFIX~~2_NAME~~3_SUFFIX~
					if ( dwClilocName )
						t->FormatArgs("%s\t%" FMTDWORD "\t%s", szPrefix, dwClilocName, szSuffix);
					else
						t->FormatArgs("%s\t%s\t%s", szPrefix, pObj->GetName(), szSuffix);

					// Need to find a way to get the ushort inside hues.mul for index wHue to get this working
					//HUE_TYPE wHue = pChar->Noto_GetHue(m_pChar);
					//t->FormatArgs("<basefont color=\"#%02x%02x%02x\">%s\t%s\t%s</basefont>", (wHue & 0x7C00) >> 7, (wHue & 0x3E0) >> 2, (wHue & 0x1F) << 3, szPrefix, pChar->GetName(), szSuffix); // ~1_PREFIX~~2_NAME~~3_SUFFIX~

					if ( pChar->m_pPlayer )
					{
						const CStoneMember *pGuildMember = pChar->Guild_FindMember(MEMORY_GUILD);
						if ( pGuildMember && pGuildMember->IsAbbrevOn() )
						{
							m_TooltipData.Add(t = new CClientTooltip(1042971)); // ~1_NOTHING~
							if ( pGuildMember->GetTitle()[0] )
								t->FormatArgs("%s, %s", pGuildMember->GetTitle(), pGuildMember->GetParentStone()->GetName());
							else
								t->FormatArgs("%s", pGuildMember->GetParentStone()->GetName());
						}
					}
				}

				// Some default tooltip info if RETURN 0 or no script
				if ( pChar )
				{
					// Character specific stuff
					if ( pChar->m_pPlayer )
					{
						if ( pChar->IsPriv(PRIV_GM) && !pChar->IsPriv(PRIV_PRIV_NOSHOW) )
							m_TooltipData.Add(new CClientTooltip(1018085)); // Game Master
					}
					else if ( pChar->m_pNPC )
					{
						if ( g_Cfg.m_iFeatureML & FEATURE_ML_UPDATE )
						{
							CREID_TYPE id = pChar->GetID();
							if ( (id == CREID_LLAMA_PACK) || (id == CREID_HORSE_PACK) || (id == CREID_GIANT_BEETLE) )
							{
								int iWeight = pChar->GetWeight() / WEIGHT_UNITS;
								m_TooltipData.Add(t = new CClientTooltip((iWeight > 1) ? 1072789 : 1072788)); // Weight: ~1_WEIGHT~ stones / Weight: ~1_WEIGHT~ stone
								t->FormatArgs("%d", iWeight);
							}

							if ( pChar->Skill_GetActive() == NPCACT_GUARD_TARG )
								m_TooltipData.Add(new CClientTooltip(1080078)); // guarding
						}

						if ( pChar->IsStatFlag(STATF_Conjured) && (pChar->m_pNPC->m_Brain != NPCBRAIN_GUARD) )
							m_TooltipData.Add(new CClientTooltip(1049646)); // (summoned)
						else if ( pChar->IsStatFlag(STATF_Pet) )
							m_TooltipData.Add(new CClientTooltip(pChar->m_pNPC->m_bonded ? 1049608 : 502006)); // (bonded) / (tame)
					}
				}
				else if ( pItem )
				{
					if ( pItem->IsAttr(ATTR_LOCKEDDOWN) )
						m_TooltipData.Add(new CClientTooltip(501643)); // Locked Down
					if ( pItem->IsAttr(ATTR_SECURE) )
						m_TooltipData.Add(new CClientTooltip(501644)); // Locked Down & Secured
					if ( pItem->IsAttr(ATTR_BLESSED) )
						m_TooltipData.Add(new CClientTooltip(1038021)); // Blessed
					if ( pItem->IsAttr(ATTR_CURSED) )
						m_TooltipData.Add(new CClientTooltip(1049643)); // Cursed
					if ( pItem->IsAttr(ATTR_INSURED) )
						m_TooltipData.Add(new CClientTooltip(1061682)); // <b>Insured</b>
					if ( pItem->IsAttr(ATTR_QUESTITEM) )
						m_TooltipData.Add(new CClientTooltip(1072351)); // Quest Item
					if ( pItem->IsAttr(ATTR_MAGIC) )
						m_TooltipData.Add(new CClientTooltip(3010064)); // Magic
					if ( pItem->IsAttr(ATTR_NEWBIE) )
						m_TooltipData.Add(new CClientTooltip(1042971, g_Cfg.GetDefaultMsg(DEFMSG_TOOLTIP_TAG_NEWBIE))); // ~1_NOTHING~
					if ( pItem->IsAttr(ATTR_NODROPTRADE) )
					{
						m_TooltipData.Add(new CClientTooltip(1076253)); // NO-DROP
						m_TooltipData.Add(new CClientTooltip(1076255)); // NO-TRADE
					}
					if ( pItem->IsAttr(ATTR_NOREPAIR) )
						m_TooltipData.Add(new CClientTooltip(1151782)); // cannot be repaired

					if ( !pItem->IsType(IT_CORPSE) )
					{
						const CContainer *pContainer = dynamic_cast<const CContainer *>(pItem);
						if ( pContainer || pItem->IsMovable() )
						{
							int iWeight = pItem->GetWeight() / WEIGHT_UNITS;
							if ( iWeight > 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip((iWeight == 1) ? 1072788 : 1072789)); // Weight: ~1_WEIGHT~ stone / Weight: ~1_WEIGHT~ stones
								t->FormatArgs("%d", iWeight);
							}
						}
						if ( pContainer )
						{
							if ( m_NetState->isClientVersion(MINCLIVER_ML) )
							{
								int iMaxWeight = (pItem->GetContainedLayer() == LAYER_PACK) ? g_Cfg.m_iBackpackMaxWeight + pItem->m_ModMaxWeight : pItem->m_ModMaxWeight;
								if ( iMaxWeight > 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1072241)); // Contents: ~1_COUNT~/~2_MAXCOUNT~ items, ~3_WEIGHT~/~4_MAXWEIGHT~ stones
									t->FormatArgs("%" FMTSIZE_T "\t%d\t%d\t%d", pContainer->GetCount(), MAX_ITEMS_CONT, pContainer->GetTotalWeight() / WEIGHT_UNITS, iMaxWeight / WEIGHT_UNITS);
								}
								else
								{
									m_TooltipData.Add(t = new CClientTooltip(1073841)); // Contents: ~1_COUNT~/~2_MAXCOUNT~ items, ~3_WEIGHT~ stones
									t->FormatArgs("%" FMTSIZE_T "\t%d\t%d", pContainer->GetCount(), MAX_ITEMS_CONT, pContainer->GetTotalWeight() / WEIGHT_UNITS);
								}
							}
							else
							{
								m_TooltipData.Add(t = new CClientTooltip(1050044)); // ~1_COUNT~ items, ~2_WEIGHT~ stones
								t->FormatArgs("%" FMTSIZE_T "\t%d", pContainer->GetCount(), pContainer->GetTotalWeight() / WEIGHT_UNITS);
							}

							if ( pItem->m_WeightReduction != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1072210)); // Weight reduction: ~1_PERCENTAGE~%
								t->FormatArgs("%d", pItem->m_WeightReduction);
							}
						}
					}

					const CChar *pCraftsman = static_cast<CGrayUID>(pItem->GetDefNum("CRAFTEDBY")).CharFind();
					if ( pCraftsman )
					{
						m_TooltipData.Add(t = new CClientTooltip(1050043)); // crafted by ~1_NAME~
						t->FormatArgs("%s", pCraftsman->GetName());
					}
					else
						const_cast<CItem *>(pItem)->SetDefNum("CRAFTEDBY", 0);

					if ( pItem->IsAttr(ATTR_EXCEPTIONAL) )
						m_TooltipData.Add(new CClientTooltip(1060636)); // exceptional

					if ( pItem->m_ArtifactRarity > 0 )
					{
						m_TooltipData.Add(t = new CClientTooltip(1061078)); // artifact rarity ~1_val~
						t->FormatArgs("%d", pItem->m_ArtifactRarity);
					}

					INT64 UsesRemaining = pItem->GetDefNum("USESMAX", true);
					if ( UsesRemaining > 0 )
					{
						m_TooltipData.Add(t = new CClientTooltip(1060584)); // uses remaining: ~1_val~
						t->FormatArgs("%lld", pItem->GetDefNum("USESCUR", true));
					}

					if ( pItem->IsTypeArmorWeapon() )
					{
						if ( pItem->m_DamIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060401)); // damage increase ~1_val~%
							t->FormatArgs("%d", pItem->m_DamIncrease);
						}

						if ( pItem->m_DefChanceIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060408)); // defense chance increase ~1_val~%
							t->FormatArgs("%d", pItem->m_DefChanceIncrease);
						}

						if ( pItem->m_DexterityBonus != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060409)); // dexterity bonus ~1_val~
							t->FormatArgs("%d", pItem->m_DexterityBonus);
						}

						if ( pItem->m_EnhancePotions != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060411)); // enhance potions ~1_val~%
							t->FormatArgs("%d", pItem->m_EnhancePotions);
						}

						if ( pItem->m_FasterCastRecovery != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060412)); // faster cast recovery ~1_val~
							t->FormatArgs("%d", pItem->m_FasterCastRecovery);
						}

						if ( pItem->m_FasterCasting != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060413)); // faster casting ~1_val~
							t->FormatArgs("%d", pItem->m_FasterCasting);
						}

						if ( pItem->m_HitChanceIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060415)); // hit chance increase ~1_val~%
							t->FormatArgs("%d", pItem->m_HitChanceIncrease);
						}

						if ( pItem->m_HitpointIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060431)); // hit point increase ~1_val~%
							t->FormatArgs("%d", pItem->m_HitpointIncrease);
						}

						if ( pItem->m_IntelligenceBonus != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060432)); // intelligence bonus ~1_val~
							t->FormatArgs("%d", pItem->m_IntelligenceBonus);
						}

						if ( pItem->m_LowerManaCost != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060433)); // lower mana cost ~1_val~%
							t->FormatArgs("%d", pItem->m_LowerManaCost);
						}

						if ( pItem->m_LowerReagentCost != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060434)); // lower reagent cost ~1_val~%
							t->FormatArgs("%d", pItem->m_LowerReagentCost);
						}

						if ( pItem->m_LowerRequirements != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060435)); // lower requirements ~1_val~%
							t->FormatArgs("%d", pItem->m_LowerRequirements);
						}

						if ( pItem->m_Luck != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060436)); // luck ~1_val~
							t->FormatArgs("%d", pItem->m_Luck);
						}

						if ( pItem->m_MageArmor )
							m_TooltipData.Add(new CClientTooltip(1060437)); // mage armor

						if ( pItem->m_MageWeapon != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060438)); // mage weapon -~1_val~ skill
							t->FormatArgs("%d", pItem->m_MageWeapon);
						}

						if ( pItem->m_ManaIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060439)); // mana increase ~1_val~
							t->FormatArgs("%d", pItem->m_ManaIncrease);
						}

						INT64 ManaRegeneration = pItem->GetDefNum("REGENMANA", true);
						if ( ManaRegeneration != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060440)); // mana regeneration ~1_val~
							t->FormatArgs("%lld", ManaRegeneration);
						}

						if ( pItem->m_NightSight )
							m_TooltipData.Add(new CClientTooltip(1060441)); // night sight

						if ( pItem->m_ReflectPhysicalDamage != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060442)); // reflect physical damage ~1_val~%
							t->FormatArgs("%d", pItem->m_ReflectPhysicalDamage);
						}

						INT64 StaminaRegeneration = pItem->GetDefNum("REGENSTAM", true);
						if ( StaminaRegeneration != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060443)); // stamina regeneration ~1_val~
							t->FormatArgs("%lld", StaminaRegeneration);
						}

						INT64 HitPointRegeneration = pItem->GetDefNum("REGENHITS", true);
						if ( HitPointRegeneration != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060444)); // hit point regeneration ~1_val~
							t->FormatArgs("%lld", HitPointRegeneration);
						}

						if ( pItem->m_SelfRepair != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060450)); // self repair ~1_val~
							t->FormatArgs("%d", pItem->m_SelfRepair);
						}

						if ( pItem->m_SpellChanneling )
							m_TooltipData.Add(new CClientTooltip(1060482)); // spell channeling

						if ( pItem->m_SpellDamIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060483)); // spell damage increase ~1_val~%
							t->FormatArgs("%d", pItem->m_SpellDamIncrease);
						}

						if ( pItem->m_StaminaIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060484)); // stamina increase ~1_val~
							t->FormatArgs("%d", pItem->m_StaminaIncrease);
						}

						if ( pItem->m_StrengthBonus != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060485)); // strength bonus ~1_val~
							t->FormatArgs("%d", pItem->m_StrengthBonus);
						}

						if ( pItem->m_SwingSpeedIncrease != 0 )
						{
							m_TooltipData.Add(t = new CClientTooltip(1060486)); // swing speed increase ~1_val~%
							t->FormatArgs("%d", pItem->m_SwingSpeedIncrease);
						}
					}

					// Some type specific default stuff
					switch ( pItem->GetType() )
					{
						case IT_CONTAINER_LOCKED:
						case IT_SHIP_HOLD_LOCK:
							m_TooltipData.Add(new CClientTooltip(3005142)); // Locked
							break;

						case IT_ARMOR_LEATHER:
						case IT_ARMOR:
						case IT_CLOTHING:
						case IT_SHIELD:
						{
							if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
							{
								if ( pItem->m_ResPhysicalMax != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1153735)); // physical resist ~1_val~% / ~2_symb~~3_val~% Max
									t->FormatArgs("%d\t\t%d", pItem->m_ResPhysical, pItem->m_ResPhysicalMax);
								}
								else if ( pItem->m_ResPhysical != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060448)); // physical resist ~1_val~%
									t->FormatArgs("%d", pItem->m_ResPhysical);
								}

								if ( pItem->m_ResFireMax != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1153737)); // fire resist ~1_val~% / ~2_symb~~3_val~% Max
									t->FormatArgs("%d\t\t%d", pItem->m_ResFire, pItem->m_ResFireMax);
								}
								else if ( pItem->m_ResFire != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060447)); // fire resist ~1_val~%
									t->FormatArgs("%d", pItem->m_ResFire);
								}

								if ( pItem->m_ResColdMax != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1153739)); // cold resist ~1_val~% / ~2_symb~~3_val~% Max
									t->FormatArgs("%d\t\t%d", pItem->m_ResCold, pItem->m_ResColdMax);
								}
								else if ( pItem->m_ResCold != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060445)); // cold resist ~1_val~%
									t->FormatArgs("%d", pItem->m_ResCold);
								}

								if ( pItem->m_ResPoisonMax != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1153736)); // poison resist ~1_val~% / ~2_symb~~3_val~% Max
									t->FormatArgs("%d\t\t%d", pItem->m_ResPoison, pItem->m_ResPoisonMax);
								}
								else if ( pItem->m_ResPoison != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060449)); // poison resist ~1_val~%
									t->FormatArgs("%d", pItem->m_ResPoison);
								}

								if ( pItem->m_ResEnergyMax != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1153738)); // energy resist ~1_val~% / ~2_symb~~3_val~% Max
									t->FormatArgs("%d\t\t%d", pItem->m_ResEnergy, pItem->m_ResEnergyMax);
								}
								else if ( pItem->m_ResEnergy != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060446)); // energy resist ~1_val~%
									t->FormatArgs("%d", pItem->m_ResEnergy);
								}
							}
							else
							{
								int iArmorRating = pItem->Armor_GetDefense();
								if ( iArmorRating != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1060448)); // physical resist ~1_val~%
									t->FormatArgs("%d", iArmorRating);
								}
							}

							int iStrengthRequirement = pItem->Item_GetDef()->m_ttEquippable.m_StrReq * (100 - pItem->m_LowerRequirements) / 100;
							if ( iStrengthRequirement > 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1061170)); // strength requirement ~1_val~
								t->FormatArgs("%d", iStrengthRequirement);
							}

							if ( pItem->m_itArmor.m_Hits_Max > 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
								t->FormatArgs("%hu\t%hu", pItem->m_itArmor.m_Hits_Cur, pItem->m_itArmor.m_Hits_Max);
							}

							if ( m_NetState->isClientVersion(MINCLIVER_PARRYCHANCETOOLTIP) )
							{
								if ( pItem->m_LastParryChance != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1158861)); // Last Parry Chance: ~1_val~%
									t->FormatArgs("%d", pItem->m_LastParryChance);
								}
							}
							break;
						}

						case IT_WEAPON_MACE_SMITH:
						case IT_WEAPON_MACE_SHARP:
						case IT_WEAPON_MACE_STAFF:
						case IT_WEAPON_MACE_CROOK:
						case IT_WEAPON_MACE_PICK:
						case IT_WEAPON_SWORD:
						case IT_WEAPON_FENCE:
						case IT_WEAPON_BOW:
						case IT_WEAPON_AXE:
						case IT_WEAPON_XBOW:
						case IT_WEAPON_THROWING:
						case IT_WEAPON_WHIP:
						{
							if ( pItem->m_itWeapon.m_poison_skill )
								m_TooltipData.Add(new CClientTooltip(1017383)); // poisoned

							if ( pItem->m_UseBestWeaponSkill )
								m_TooltipData.Add(new CClientTooltip(1060400)); // use best weapon skill

							if ( pItem->m_HitColdArea != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060416)); // hit cold area ~1_val~%
								t->FormatArgs("%d", pItem->m_HitColdArea);
							}

							if ( pItem->m_HitDispel != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060417)); // hit dispel ~1_val~%
								t->FormatArgs("%d", pItem->m_HitDispel);
							}

							if ( pItem->m_HitEnergyArea != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060418)); // hit energy area ~1_val~%
								t->FormatArgs("%d", pItem->m_HitEnergyArea);
							}

							if ( pItem->m_HitFireArea != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060419)); // hit fire area ~1_val~%
								t->FormatArgs("%d", pItem->m_HitFireArea);
							}

							if ( pItem->m_HitFireball != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060420)); // hit fireball ~1_val~%
								t->FormatArgs("%d", pItem->m_HitFireball);
							}

							if ( pItem->m_HitHarm != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060421)); // hit harm ~1_val~%
								t->FormatArgs("%d", pItem->m_HitHarm);
							}

							if ( pItem->m_HitLifeLeech != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060422)); // hit life leech ~1_val~%
								t->FormatArgs("%d", pItem->m_HitLifeLeech);
							}

							if ( pItem->m_HitLightning != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060423)); // hit lightning ~1_val~%
								t->FormatArgs("%d", pItem->m_HitLightning);
							}

							if ( pItem->m_HitMagicArrow != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060426)); // hit magic arrow ~1_val~%
								t->FormatArgs("%d", pItem->m_HitMagicArrow);
							}

							if ( pItem->m_HitManaDrain != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1113699)); // hit mana drain ~1_val~%
								t->FormatArgs("%d", pItem->m_HitManaDrain);
							}

							if ( pItem->m_HitManaLeech != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060427)); // hit mana leech ~1_val~%
								t->FormatArgs("%d", pItem->m_HitManaLeech);
							}

							if ( pItem->m_HitPhysicalArea != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060428)); // hit physical area ~1_val~%
								t->FormatArgs("%d", pItem->m_HitPhysicalArea);
							}

							if ( pItem->m_HitPoisonArea != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060429)); // hit poison area ~1_val~%
								t->FormatArgs("%d", pItem->m_HitPoisonArea);
							}

							if ( pItem->m_HitStaminaLeech != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060430)); // hit stamina leech ~1_val~%
								t->FormatArgs("%d", pItem->m_HitStaminaLeech);
							}

							if ( pItem->m_DamPhysical != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060403)); // physical damage ~1_val~%
								t->FormatArgs("%d", pItem->m_DamPhysical);
							}

							if ( pItem->m_DamFire != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060405)); // fire damage ~1_val~%
								t->FormatArgs("%d", pItem->m_DamFire);
							}

							if ( pItem->m_DamCold != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060404)); // cold damage ~1_val~%
								t->FormatArgs("%d", pItem->m_DamCold);
							}

							if ( pItem->m_DamPoison != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060406)); // poison damage ~1_val~%
								t->FormatArgs("%d", pItem->m_DamPoison);
							}

							if ( pItem->m_DamEnergy != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060407)); // energy damage ~1_val~%
								t->FormatArgs("%d", pItem->m_DamEnergy);
							}

							INT64 ChaosDamage = pItem->GetDefNum("DAMCHAOS", true);
							if ( ChaosDamage != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1072846)); // chaos damage ~1_val~%
								t->FormatArgs("%lld", ChaosDamage);
							}

							INT64 DirectDamage = pItem->GetDefNum("DAMDIRECT", true);
							if ( DirectDamage != 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1079978)); // direct damage: ~1_PERCENT~%
								t->FormatArgs("%lld", DirectDamage);
							}

							m_TooltipData.Add(t = new CClientTooltip(1061168)); // weapon damage ~1_val~ - ~2_val~
							t->FormatArgs("%d\t%d", pItem->m_attackBase + pItem->m_ModAr, pItem->Weapon_GetAttack(true));

							m_TooltipData.Add(t = new CClientTooltip(1061167)); // weapon speed ~1_val~
							t->FormatArgs("%hhu", pItem->GetSpeed());

							BYTE bRange = pItem->RangeL();
							if ( bRange > 1 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
								t->FormatArgs("%hhu", bRange);
							}

							int iStrengthRequirement = pItem->Item_GetDef()->m_ttEquippable.m_StrReq * (100 - pItem->m_LowerRequirements) / 100;
							if ( iStrengthRequirement > 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1061170)); // strength requirement ~1_val~
								t->FormatArgs("%d", iStrengthRequirement);
							}

							m_TooltipData.Add(new CClientTooltip((pItem->Item_GetDef()->GetEquipLayer() == LAYER_HAND2) ? 1061171 : 1061824)); // two-handed weapon : one-handed weapon

							if ( !pItem->m_UseBestWeaponSkill )
							{
								switch ( pItem->Weapon_GetSkill() )
								{
									case SKILL_SWORDSMANSHIP:	m_TooltipData.Add(new CClientTooltip(1061172));	break; // skill required: swordsmanship
									case SKILL_MACEFIGHTING:	m_TooltipData.Add(new CClientTooltip(1061173));	break; // skill required: mace fighting
									case SKILL_FENCING:			m_TooltipData.Add(new CClientTooltip(1061174));	break; // skill required: fencing
									case SKILL_ARCHERY:			m_TooltipData.Add(new CClientTooltip(1061175));	break; // skill required: archery
									case SKILL_THROWING:		m_TooltipData.Add(new CClientTooltip(1112075));	break; // skill required: throwing
									default:					break;
								}
							}

							if ( pItem->m_itWeapon.m_Hits_Max > 0 )
							{
								m_TooltipData.Add(t = new CClientTooltip(1060639)); // durability ~1_val~ / ~2_val~
								t->FormatArgs("%hu\t%hu", pItem->m_itWeapon.m_Hits_Cur, pItem->m_itWeapon.m_Hits_Max);
							}

							if ( m_NetState->isClientVersion(MINCLIVER_PARRYCHANCETOOLTIP) )
							{
								if ( pItem->m_LastParryChance != 0 )
								{
									m_TooltipData.Add(t = new CClientTooltip(1158861)); // Last Parry Chance: ~1_val~%
									t->FormatArgs("%d", pItem->m_LastParryChance);
								}
							}
							break;
						}

						case IT_WAND:
						{
							m_TooltipData.Add(t = new CClientTooltip(1060741)); // charges: ~1_val~
							t->FormatArgs("%d", pItem->m_itWeapon.m_spellcharges);
							break;
						}

						case IT_TELEPAD:
						{
							if ( IsPriv(PRIV_GM) )
							{
								m_TooltipData.Add(t = new CClientTooltip(1061114)); // Location: ~1_val~
								t->FormatArgs("%s", pItem->m_itTelepad.m_ptMark.WriteUsed());
							}
							break;
						}

						case IT_RUNE:
						{
							const CPointMap pt = pItem->m_itTelepad.m_ptMark;
							if ( !pt.IsValidXY() )
								break;

							bool fHouse = (pt.GetRegion(REGION_TYPE_MULTI) != NULL);
							switch ( pt.m_map )
							{
								case 0:		m_TooltipData.Add(t = new CClientTooltip(fHouse ? 1062452 : 1060805));	break; // ~1_val~ (Felucca)[(House)]
								case 1:		m_TooltipData.Add(t = new CClientTooltip(fHouse ? 1062453 : 1060806));	break; // ~1_val~ (Trammel)[(House)]
								case 3:		m_TooltipData.Add(t = new CClientTooltip(fHouse ? 1062454 : 1060804));	break; // ~1_val~ (Malas)[(House)]
								case 4:		m_TooltipData.Add(t = new CClientTooltip(fHouse ? 1063260 : 1063259));	break; // ~1_val~ (Tokuno Islands)[(House)]
								case 5:		m_TooltipData.Add(t = new CClientTooltip(fHouse ? 1113206 : 1113205));	break; // ~1_val~ (Ter Mur)[(House)]
								default:	m_TooltipData.Add(t = new CClientTooltip(1070722));						break; // ~1_NOTHING~
							}
							t->FormatArgs("%s%s", g_Cfg.GetDefaultMsg(DEFMSG_RUNE_NAME_MARKED), pObj->GetName());
							break;
						}

						case IT_SPELLBOOK:
						case IT_SPELLBOOK_NECRO:
						case IT_SPELLBOOK_PALA:
						case IT_SPELLBOOK_BUSHIDO:
						case IT_SPELLBOOK_NINJITSU:
						case IT_SPELLBOOK_ARCANIST:
						case IT_SPELLBOOK_MYSTIC:
						case IT_SPELLBOOK_MASTERY:
						{
							m_TooltipData.Add(t = new CClientTooltip(1042886)); // ~1_NUMBERS_OF_SPELLS~ Spells
							t->FormatArgs("%hhu", pItem->GetSpellcountInBook());
							break;
						}

						case IT_SPAWN_CHAR:
						{
							const CResourceDef *pSpawnCharDef = g_Cfg.ResourceGetDef(pItem->m_itSpawnChar.m_CharID);
							m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
							t->FormatArgs("Character\t%s", pSpawnCharDef ? pSpawnCharDef->GetResourceName() : "none");
							m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
							t->FormatArgs("%hhu", pItem->m_itSpawnChar.m_DistMax);
							m_TooltipData.Add(t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
							t->FormatArgs("%hhu\t%hu", static_cast<const CItemSpawn *>(pItem)->m_currentSpawned, pItem->GetAmount());
							m_TooltipData.Add(t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
							t->FormatArgs("Time range\t%hu min / %hu max", pItem->m_itSpawnChar.m_TimeLoMin, pItem->m_itSpawnChar.m_TimeHiMin);
							m_TooltipData.Add(t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
							t->FormatArgs("Time until next spawn\t%lld sec", pItem->GetTimerAdjusted());
							break;
						}

						case IT_SPAWN_ITEM:
						{
							const CResourceDef *pSpawnItemDef = g_Cfg.ResourceGetDef(pItem->m_itSpawnItem.m_ItemID);
							m_TooltipData.Add(t = new CClientTooltip(1060658)); // ~1_val~: ~2_val~
							t->FormatArgs("Item\t%" FMTDWORD " %s", maximum(1, pItem->m_itSpawnItem.m_pile), pSpawnItemDef ? pSpawnItemDef->GetResourceName() : "none");
							m_TooltipData.Add(t = new CClientTooltip(1061169)); // range ~1_val~
							t->FormatArgs("%hhu", pItem->m_itSpawnItem.m_DistMax);
							m_TooltipData.Add(t = new CClientTooltip(1074247)); // Live Creatures: ~1_NUM~ / ~2_MAX~
							t->FormatArgs("%hhu\t%hu", static_cast<const CItemSpawn *>(pItem)->m_currentSpawned, pItem->GetAmount());
							m_TooltipData.Add(t = new CClientTooltip(1060659)); // ~1_val~: ~2_val~
							t->FormatArgs("Time range\t%hu min / %hu max", pItem->m_itSpawnItem.m_TimeLoMin, pItem->m_itSpawnItem.m_TimeHiMin);
							m_TooltipData.Add(t = new CClientTooltip(1060660)); // ~1_val~: ~2_val~
							t->FormatArgs("Time until next spawn\t%lld sec", pItem->GetTimerAdjusted());
							break;
						}

						case IT_COMM_CRYSTAL:
						{
							const CItem *pLink = pItem->m_uidLink.ItemFind();
							m_TooltipData.Add(new CClientTooltip((pLink && pLink->IsType(IT_COMM_CRYSTAL)) ? 1060742 : 1060743)); // active / inactive
							m_TooltipData.Add(new CClientTooltip(1060745)); // broadcast
							break;
						}

						case IT_STONE_GUILD:
						{
							const CItemStone *pGuildStone = static_cast<const CItemStone *>(pItem);
							m_TooltipData.Clean(true);
							m_TooltipData.Add(t = new CClientTooltip(1041429)); // a guildstone
							m_TooltipData.Add(t = new CClientTooltip(1060802)); // Guild name: ~1_val~
							if ( pGuildStone->GetAbbrev()[0] )
								t->FormatArgs("%s [%s]", pGuildStone->GetName(), pGuildStone->GetAbbrev());
							else
								t->FormatArgs("%s", pGuildStone->GetName());
							break;
						}

						default:
							break;
					}
				}
			}
		}

		// Build tooltip hash (version)
		DWORD dwHash = 0;
		DWORD dwArgumentHash = 0;
		for ( size_t i = 0; i < m_TooltipData.GetCount(); ++i )
		{
			CClientTooltip *tipEntry = m_TooltipData.GetAt(i);
			dwHash ^= (tipEntry->m_clilocid & 0x3FFFFFF);
			dwHash ^= (tipEntry->m_clilocid >> 26) & 0x3F;

			dwArgumentHash = HashString(tipEntry->m_args, strlen(tipEntry->m_args));
			dwHash ^= (dwArgumentHash & 0x3FFFFFF);
			dwHash ^= (dwArgumentHash >> 26) & 0x3F;
		}
		dwHash |= UID_F_ITEM;

		if ( g_Cfg.m_iTooltipMode == TOOLTIPMODE_SENDVERSION )
		{
			// If client receive an tooltip with given obj UID / revision, and for some reason
			// the server delete this obj and create another one with the same UID / revision,
			// the client will show the previous cached tooltip on the new obj. To avoid this,
			// compare both tooltip hashes to check if it really got changed, and if positive,
			// send the full tooltip instead just the revision number
			if ( pObj->GetPropertyHash() != dwHash )
				fRequested = true;
		}

		// Build tooltip packet
		pPropertyList = new PacketPropertyList(pObj, const_cast<CObjBase *>(pObj)->UpdatePropertyRevision(dwHash), &m_TooltipData);

		// Store tooltip cache
		if ( m_TooltipEnabled && (g_Cfg.m_iTooltipCache > 0) )
			const_cast<CObjBase *>(pObj)->SetPropertyList(pPropertyList);
	}

	if ( !pPropertyList->isEmpty() )
	{
		switch ( g_Cfg.m_iTooltipMode )
		{
			case TOOLTIPMODE_SENDVERSION:	// send only tooltip version (client will compare the version and request the full tooltip if needed)
				if ( !fRequested && !fShop )
				{
					if ( PacketPropertyListVersion::CanSendTo(m_NetState) )
						new PacketPropertyListVersion(this, pObj, pPropertyList->getVersion());
					else
						new PacketPropertyListVersionOld(this, pObj, pPropertyList->getVersion());
					break;
				}

			case TOOLTIPMODE_SENDFULL:		// send full tooltip
			default:
				new PacketPropertyList(this, pPropertyList);
				break;
		}
	}

	// Delete the original packet, as long as it doesn't belong to the object (i.e. wasn't cached)
	if ( pPropertyList != pObj->GetPropertyList() )
		delete pPropertyList;
}

void CClient::addShowDamage(CGrayUID uid, int iDamage)
{
	ADDTOCALLSTACK("CClient::addShowDamage");
	if ( iDamage <= 0 )
		return;

	if ( PacketCombatDamage::CanSendTo(m_NetState) )
		new PacketCombatDamage(this, static_cast<WORD>(iDamage), uid);
	else if ( PacketCombatDamageOld::CanSendTo(m_NetState) )
		new PacketCombatDamageOld(this, static_cast<BYTE>(iDamage), uid);
}

void CClient::addSpeedMode(BYTE bMode)
{
	ADDTOCALLSTACK("CClient::addSpeedMode");
	new PacketSpeedMode(this, bMode);
}

void CClient::addVisualRange(BYTE bRange)
{
	ADDTOCALLSTACK("CClient::addVisualRange");
	new PacketVisualRange(this, bRange);
}

void CClient::addIdleWarning(BYTE bCode)
{
	ADDTOCALLSTACK("CClient::addIdleWarning");
	new PacketWarningMessage(this, static_cast<PacketWarningMessage::Message>(bCode));
}

void CClient::addKRToolbar(bool fEnable)
{
	ADDTOCALLSTACK("CClient::addKRToolbar");
	if ( !PacketToggleHotbar::CanSendTo(m_NetState) || (m_pAccount->GetResDisp() < RDS_KR) || (GetConnectType() != CONNECT_GAME) )
		return;

	new PacketToggleHotbar(this, fEnable);
}

///////////////////////////////////////////////////////////

void CClient::SendPacket(TCHAR *pszKey)
{
	ADDTOCALLSTACK("CClient::SendPacket");
	PacketSend *packet = new PacketSend(0, 0, PacketSend::PRI_NORMAL);
	packet->seek();

	while ( *pszKey )
	{
		if ( packet->getLength() > SCRIPT_MAX_LINE_LEN - 4 )
		{
			// We won't get here because this lenght is enforced in all scripts
			DEBUG_ERR(("SENDPACKET function exceeded max length allowed (%" FMTSIZE_T "/%d)\n", packet->getLength(), SCRIPT_MAX_LINE_LEN - 4));
			delete packet;
			return;
		}

		GETNONWHITESPACE(pszKey);

		if ( toupper(*pszKey) == 'D' )
		{
			++pszKey;
			packet->writeInt32(static_cast<DWORD>(Exp_GetLLVal(pszKey)));
		}
		else if ( toupper(*pszKey) == 'W' )
		{
			++pszKey;
			packet->writeInt16(static_cast<WORD>(Exp_GetLLVal(pszKey)));
		}
		else
		{
			if ( toupper(*pszKey) == 'B' )
				++pszKey;
			packet->writeByte(static_cast<BYTE>(Exp_GetLLVal(pszKey)));
		}
	}

	packet->trim();
	packet->push(this);
}

///////////////////////////////////////////////////////////
// Login type stuff

BYTE CClient::LogIn(LPCTSTR pszAccount, LPCTSTR pszPassword, CGString &sMsg)
{
	ADDTOCALLSTACK("CClient::LogIn");
	// Try to validate this account
	// Do not send output messages as this might be a console or web page or game client
	// NOTE: addLoginErr() will get called after this

	if ( m_pAccount )	// already logged in
		return PacketLoginError::Success;

	// Check login
	TCHAR szAccount[MAX_ACCOUNT_NAME_ENTRY];
	size_t iLenAccount = pszAccount ? strlen(pszAccount) : 0;
	if ( (iLenAccount == 0) || (iLenAccount >= MAX_ACCOUNT_NAME_ENTRY) || Str_Check(pszAccount) || (Str_GetBare(szAccount, pszAccount, sizeof(szAccount)) != iLenAccount) )
	{
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_DENIED);
		return PacketLoginError::InvalidCred;
	}

	CAccount *pAccount = g_Accounts.Account_Find(pszAccount);
	if ( !pAccount )
	{
		if ( g_Cfg.m_fAutoAccountCreation )
			pAccount = g_Accounts.Account_Create(NULL, pszAccount, pszPassword);

		if ( !pAccount )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' doesn't exist\n", GetSocketID(), pszAccount);
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_DENIED);
			return PacketLoginError::InvalidCred;
		}
	}

	// Check password
	if ( g_Cfg.m_iClientLoginMaxTries && !pAccount->CheckPasswordTries(GetPeer()) )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' exceeded password tries\n", GetSocketID(), pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_DENIED);
		return PacketLoginError::MaxPassTries;
	}

	TCHAR szPassword[MAX_ACCOUNT_PASS_ENTRY];
	size_t iLenPassword = pszPassword ? strlen(pszPassword) : 0;
	if ( (iLenPassword == 0) || (iLenPassword >= MAX_ACCOUNT_NAME_ENTRY) || Str_Check(pszPassword) || (Str_GetBare(szPassword, pszPassword, sizeof(szPassword)) != iLenPassword) || !pAccount->CheckPassword(pszPassword) )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' inserted bad password\n", GetSocketID(), pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_DENIED);
		return PacketLoginError::InvalidCred;
	}

	if ( g_Cfg.m_iClientLoginMaxTries )
		pAccount->m_PasswordTries.clear();

	// Check if account is blocked
	if ( pAccount->IsPriv(PRIV_BLOCKED) )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' is blocked\n", GetSocketID(), pAccount->GetName());
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_BLOCKED), static_cast<LPCTSTR>(g_Serv.m_sEMail));
		return PacketLoginError::Blocked;
	}

	// Check if account is in use
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient->m_pAccount == pAccount )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' already in use\n", GetSocketID(), pAccount->GetName());
			sMsg = "Account already in use.";
			return PacketLoginError::InUse;
		}
	}

	// Check privileges
	CSocketAddressIP SockAddr = GetPeer();
	if ( g_Cfg.m_iClientsMax <= 0 )
	{
		if ( !SockAddr.IsLocalAddr() )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' can't connect, only local connections allowed\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_LD);
			return PacketLoginError::MaxClients;
		}
	}
	if ( pAccount->GetPrivLevel() < PLEVEL_GM )
	{
		if ( g_Cfg.m_iClientsMax == 1 )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' can't connect, only GM accounts allowed\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_AO);
			return PacketLoginError::MaxClients;
		}
		else if ( g_Serv.StatGet(SERV_STAT_CLIENTS) > g_Cfg.m_iClientsMax )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' can't connect, server maximum clients reached\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_FULL);
			return PacketLoginError::MaxClients;
		}
	}

	// Check scripts
	pAccount->m_Last_IP = SockAddr;
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	Args.m_iN1 = GetConnectType();
	Args.m_pO1 = this;
	TRIGRET_TYPE tr;
	g_Serv.r_Call("f_onaccount_login", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_DENIED);
		return PacketLoginError::Blocked;
	}

	m_pAccount = pAccount;
	m_pAccount->OnLogin(this);
	return PacketLoginError::Success;
}

BYTE CClient::Setup_ListReq(LPCTSTR pszAccount, LPCTSTR pszPassword, bool fTest)
{
	ADDTOCALLSTACK("CClient::Setup_ListReq");
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )
		return PacketLoginError::Other;

	if ( GetTargMode() == CLIMODE_SETUP_RELAY )
		ClearTargMode();

	CGString sMsg;
	BYTE bResult = LogIn(pszAccount, pszPassword, sMsg);
	if ( bResult != PacketLoginError::Success )
	{
		if ( fTest && (bResult != PacketLoginError::Other) )
		{
			if ( !sMsg.IsEmpty() )
				SysMessage(sMsg);
		}
		return bResult;
	}

	ASSERT(m_pAccount);

	// If the last char is lingering then log back into this char instantly
	/*
	CChar *pCharLast = m_pAccount->m_uidLastChar.CharFind();
	if ( pCharLast && m_pAccount->IsMyAccountChar(pCharLast) && !pCharLast->IsDisconnected() )
		return Setup_Start(pCharLast);
	*/

	//m_NetState->detectAsyncMode();	// disabled because of instability

	if ( g_Cfg.m_bAutoResDisp )
		m_pAccount->SetAutoResDisp(this);

	UpdateFeatureFlags();
	new PacketEnableFeatures(this, m_FeatureFlags);

	UpdateCharacterListFlags();
	new PacketCharacterList(this);

	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return PacketLoginError::Success;
}

BYTE CClient::Setup_Delete(DWORD dwSlot)
{
	ADDTOCALLSTACK("CClient::Setup_Delete");
	ASSERT(m_pAccount);

	if ( dwSlot >= COUNTOF(m_tmSetupCharList) )
		return PacketDeleteError::InvalidRequest;

	CChar *pChar = m_tmSetupCharList[dwSlot].CharFind();
	if ( !m_pAccount->IsMyAccountChar(pChar) )
		return PacketDeleteError::BadPass;
	if ( !pChar->IsDisconnected() )
		return PacketDeleteError::InUse;

	// Make sure the char is at least x days old
	if ( g_Cfg.m_iMinCharDeleteTime && (-g_World.GetTimeDiff(pChar->m_timeCreate) < g_Cfg.m_iMinCharDeleteTime) )
	{
		if ( GetPrivLevel() < PLEVEL_Counsel )
			return PacketDeleteError::NotOldEnough;
	}

	CGrayUID uid = pChar->GetUID();
	LPCTSTR pszName = pChar->GetName();

	pChar->Delete(false, this);
	if ( !pChar->IsDeleted() )
		return PacketDeleteError::InvalidRequest;

	g_Log.Event(LOGM_ACCOUNTS, "%lx:Account '%s' deleted char '%s' [0%" FMTDWORDH "] on client character selection menu\n", GetSocketID(), m_pAccount->GetName(), pszName, static_cast<DWORD>(uid));
	return PacketDeleteError::Success;
}

BYTE CClient::Setup_Play(DWORD dwSlot)
{
	ADDTOCALLSTACK("CClient::Setup_Play");
	// Behavior after hit "Play Character" button
	// Mode == CLIMODE_SETUP_CHARLIST

	if ( !m_pAccount )
		return PacketLoginError::InvalidCred;
	if ( dwSlot >= COUNTOF(m_tmSetupCharList) )
		return PacketLoginError::BadChar;

	CChar *pChar = m_tmSetupCharList[dwSlot].CharFind();
	if ( !m_pAccount->IsMyAccountChar(pChar) )
		return PacketLoginError::BadChar;

	CChar *pCharLast = m_pAccount->m_uidLastChar.CharFind();
	if ( pCharLast && m_pAccount->IsMyAccountChar(pCharLast) && (m_pAccount->GetPrivLevel() <= PLEVEL_GM) && !pCharLast->IsDisconnected() && (pChar->GetUID() != pCharLast->GetUID()) )
	{
		addIdleWarning(PacketWarningMessage::CharacterInWorld);
		return PacketLoginError::InUse;
	}

	m_pAccount->m_TagDefs.SetStr("LastLogged", false, m_pAccount->m_dateLastConnect.Format(NULL));
	m_pAccount->m_dateLastConnect = CGTime::GetCurrentTime();
	return Setup_Start(pChar);
}

BYTE CClient::Setup_Start(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::Setup_Start");
	// Send character startup stuff
	ASSERT(m_pAccount);
	ASSERT(pChar);

	CharDisconnect();	// I'm already logged in as someone else ?
	m_pAccount->m_uidLastChar = pChar->GetUID();

	g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Account '%s' logged on char '%s' ('%s')\n", GetSocketID(), m_pAccount->GetName(), pChar->GetName(), GetPeerStr());

	// GMs should login with invul and without allshow flag set
	if ( GetPrivLevel() > PLEVEL_Player )
	{
		ClearPrivFlags(PRIV_ALLSHOW);
		pChar->StatFlag_Set(STATF_INVUL);
	}

	addPlayerStart(pChar);
	ASSERT(m_pChar);

	bool fNoWelcomeMsg = false;
	if ( IsTrigUsed(TRIGGER_LOGIN) )
	{
		CScriptTriggerArgs Args(fNoWelcomeMsg);
		if ( pChar->OnTrigger(CTRIG_LogIn, pChar, &Args) == TRIGRET_RET_TRUE )
		{
			m_pChar->ClientDetach();
			pChar->SetDisconnected();
			return PacketLoginError::Blocked;
		}
		fNoWelcomeMsg = (Args.m_iN1 != 0);
	}

	if ( !fNoWelcomeMsg )
	{
		addBark(SPHERE_TITLE_VER " by " SPHERE_WEBSITE, NULL, HUE_YELLOW, TALKMODE_SYSTEM);

		CVarDefCont *pVarLastLogged = m_pChar->m_pArea->m_TagDefs.GetKey("LastLogged");
		if ( pVarLastLogged )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_LOGIN_LASTLOGGED), pVarLastLogged->GetValStr());
	}

	m_pAccount->m_TagDefs.DeleteKey("LastLogged");

	if ( IsPriv(PRIV_GM_PAGE) && (g_World.m_GMPages.GetCount() > 0) )
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_GMPAGE_PENDING), static_cast<int>(g_World.m_GMPages.GetCount()), g_Cfg.m_cCommandPrefix);

	if ( g_Serv.m_timeShutdown.IsTimeValid() )
		addBarkParse(g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_SHUTDOWN_SOON), NULL, HUE_RED, TALKMODE_SYSTEM);

	CItem *pMurderMemory = pChar->LayerFind(LAYER_FLAG_Murders);
	if ( pMurderMemory )
	{
		// Murder decay timer should only count when player is online, so activate the timer when it connect
		pMurderMemory->SetTimeout(static_cast<INT64>(pMurderMemory->m_itEqMurderCount.m_Decay_Balance) * TICK_PER_SEC);
	}
	else
	{
		// If there's no murder memory found, check if a new memory should be created
		pChar->Noto_Murder();
	}

	// Don't login on the water, bring us to nearest shore (unless I can swim)
	if ( !IsPriv(PRIV_GM) && !m_pChar->Char_GetDef()->Can(CAN_C_SWIM) && m_pChar->IsSwimming() )
	{
		int i = 0;
		int iDist = 1;
		int iDistNew = 0;
		for ( ; i < 20; ++i )
		{
			iDistNew = iDist + 20;
			for ( int iDir = DIR_N; iDir < DIR_QTY; ++iDir )	// try in all directions
			{
				if ( m_pChar->MoveToValidSpot(static_cast<DIR_TYPE>(iDir), iDistNew, iDist) )
				{
					i = 100;
					break;
				}
			}
			iDist = iDistNew;
		}
		addSysMessage(g_Cfg.GetDefaultMsg((i < 100) ? DEFMSG_MSG_REGION_WATER_1 : DEFMSG_MSG_REGION_WATER_2));
	}
	return PacketLoginError::Success;
}
