#include "graysvr.h"	// predef header
#include "../graysvr/CPathFinder.h"
#include "../network/receive.h"

enum NV_TYPE
{
	NV_BUY,
	NV_BYE,
	NV_FLEE,
	NV_GOTO,
	NV_HIRE,
	NV_LEAVE,
	NV_PETRETRIEVE,
	NV_PETSTABLE,
	NV_RESTOCK,
	NV_RUN,
	NV_RUNTO,
	NV_SELL,
	NV_SHRINK,
	NV_TRAIN,
	NV_WALK,
	NV_QTY
};

LPCTSTR const CCharNPC::sm_szVerbKeys[NV_QTY + 1] =
{
	"BUY",
	"BYE",
	"FLEE",
	"GOTO",
	"HIRE",
	"LEAVE",
	"PETRETRIEVE",
	"PETSTABLE",
	"RESTOCK",
	"RUN",
	"RUNTO",
	"SELL",
	"SHRINK",
	"TRAIN",
	"WALK",
	NULL
};

bool CChar::NPC_OnVerb(CScript &s, CTextConsole *pSrc)		// execute command from script
{
	ADDTOCALLSTACK("CChar::NPC_OnVerb");
	if ( !m_pNPC )
		return false;

	switch ( FindTableSorted(s.GetKey(), CCharNPC::sm_szVerbKeys, COUNTOF(CCharNPC::sm_szVerbKeys) - 1) )
	{
		case NV_BUY:
		{
			CChar *pCharSrc = pSrc->GetChar();
			if ( pCharSrc && pCharSrc->m_pClient )
				pCharSrc->m_pClient->addShopMenuBuy(this);
			return true;
		}
		case NV_BYE:
		{
			Skill_Start(SKILL_NONE);
			m_Act_Targ.InitUID();
			return true;
		}
		case NV_FLEE:
		case NV_LEAVE:
		{
			m_atFlee.m_iStepsMax = s.GetArgVal();
			if ( !m_atFlee.m_iStepsMax )
				m_atFlee.m_iStepsMax = 20;

			m_atFlee.m_iStepsCurrent = 0;
			Skill_Start(NPCACT_FLEE);
			return true;
		}
		case NV_GOTO:
		{
			m_Act_p = g_Cfg.GetRegionPoint(s.GetArgStr());
			Skill_Start(NPCACT_GOTO);
			return true;
		}
		case NV_HIRE:
			return NPC_OnHireHear(pSrc->GetChar());
		case NV_PETRETRIEVE:
			return NPC_StablePetRetrieve(pSrc->GetChar());
		case NV_PETSTABLE:
			return NPC_StablePetSelect(pSrc->GetChar());
		case NV_RESTOCK:
			return NPC_Vendor_Restock(true, (s.GetArgVal() != 0));
		case NV_RUN:
		{
			m_Act_p = GetTopPoint();
			m_Act_p.Move(GetDirStr(s.GetArgRaw()));
			NPC_WalkToPoint(true);
			return true;
		}
		case NV_RUNTO:
		{
			m_Act_p = g_Cfg.GetRegionPoint(s.GetArgStr());
			Skill_Start(NPCACT_RUNTO);
			return true;
		}
		case NV_SELL:
		{
			CChar *pCharSrc = pSrc->GetChar();
			if ( pCharSrc && pCharSrc->m_pClient )
				pCharSrc->m_pClient->addShopMenuSell(this);
			return true;
		}
		case NV_SHRINK:
		{
			CChar *pCharSrc = pSrc->GetChar();
			if ( !NPC_IsOwnedBy(pCharSrc) )
				return false;

			CItem *pItem = NPC_Shrink();
			if ( pCharSrc && (pCharSrc != this) )
			{
				if ( pItem )
					pCharSrc->m_Act_Targ = pItem->GetUID();
				if ( s.GetArgVal() )
					pCharSrc->ItemBounce(pItem);
			}
			return (pItem != NULL);
		}
		case NV_TRAIN:
			return NPC_OnTrainHear(pSrc->GetChar(), s.HasArgs()? s.GetArgStr() : NULL);
		case NV_WALK:
		{
			m_Act_p = GetTopPoint();
			m_Act_p.Move(GetDirStr(s.GetArgRaw()));
			NPC_WalkToPoint(false);
			return true;
		}
		default:
			return false;
	}
}

const LAYER_TYPE CChar::sm_VendorLayers[] =		// static
{
	LAYER_VENDOR_STOCK,
	LAYER_VENDOR_EXTRA,
	LAYER_VENDOR_BUYS
};

bool CChar::NPC_Vendor_Restock(bool fForce, bool fFillStock)
{
	ADDTOCALLSTACK("CChar::NPC_Vendor_Restock");
	// Restock this NPC vendor
	if ( !NPC_IsVendor() || IsStatFlag(STATF_Pet) )
		return false;

	bool fDeleteStock = true;
	if ( !fForce && m_pNPC->m_timeRestock.IsTimeValid() )
	{
		// Restock occurs every 10 minutes of inactivity (unless region tag specifies different time)
		INT64 iRestockTimer = 10 * 60 * TICK_PER_SEC;
		if ( m_pArea )
		{
			if ( m_pArea->m_TagDefs.GetKey("NoRestock") )
				fDeleteStock = false;
			else
			{
				CVarDefCont *pVar = m_pArea->m_TagDefs.GetKey("RestockVendors");
				if ( pVar )
					iRestockTimer = pVar->GetValNum();
			}
		}

		if ( m_TagDefs.GetKey("NoRestock") )
			fDeleteStock = false;

		if ( fDeleteStock )
			fDeleteStock = (CServTime::GetCurrentTime().GetTimeDiff(m_pNPC->m_timeRestock) > iRestockTimer);
	}

	if ( fDeleteStock )
	{
		m_pNPC->m_timeRestock.Init();
		for ( size_t i = 0; i < COUNTOF(sm_VendorLayers); ++i )
		{
			CItemContainer *pCont = GetContainerCreate(sm_VendorLayers[i]);
			if ( pCont )
				pCont->Empty();
		}
	}

	if ( fFillStock )
	{
		if ( !m_pNPC->m_timeRestock.IsTimeValid() )		// invalid timer means that containers are waiting to be filled
		{
			if ( IsTrigUsed(TRIGGER_NPCRESTOCK) )
				ReadScriptTrig(Char_GetDef(), CTRIG_NPCRestock, true);

			// Restock vendor money as well
			CItemContainer *pCont = GetContainer(LAYER_BANKBOX);
			if ( pCont )
				pCont->Restock();
		}
		m_pNPC->m_timeRestock.SetCurrentTime();
	}
	return true;
}

bool CChar::NPC_StablePetSelect(CChar *pCharPlayer)
{
	ADDTOCALLSTACK("CChar::NPC_StablePetSelect");
	// Player is trying to stable an pet on stable master NPC
	if ( !m_pNPC || (m_pNPC->m_Brain != NPCBRAIN_ANIMAL_TRAINER) || !pCharPlayer || !pCharPlayer->m_pClient )
		return false;

	CItemContainer *pBank = GetContainerCreate(LAYER_BANKBOX);
	if ( pBank->GetCount() >= MAX_ITEMS_CONT )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_FULL));
		return false;
	}

	int iPetCount = 0;
	for ( CItem *pItem = pBank->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsType(IT_FIGURINE) && (pItem->m_uidLink == pCharPlayer->GetUID()) )
			++iPetCount;
	}

	int iPetMax;
	if ( m_TagDefs.GetKey("MAXPLAYERPETS") )
		iPetMax = static_cast<int>(m_TagDefs.GetKeyNum("MAXPLAYERPETS"));
	else
	{
		WORD wSkillTaming = pCharPlayer->Skill_GetAdjusted(SKILL_TAMING);
		WORD wSkillAnimalLore = pCharPlayer->Skill_GetAdjusted(SKILL_ANIMALLORE);
		WORD wSkillVeterinary = pCharPlayer->Skill_GetAdjusted(SKILL_VETERINARY);
		WORD wSkillSum = wSkillTaming + wSkillAnimalLore + wSkillVeterinary;

		if ( wSkillSum >= 2400 )
			iPetMax = 5;
		else if ( wSkillSum >= 2000 )
			iPetMax = 4;
		else if ( wSkillSum >= 1600 )
			iPetMax = 3;
		else
			iPetMax = 2;

		if ( wSkillTaming >= 1000 )
			iPetMax += (wSkillTaming - 900) / 10;
		if ( wSkillAnimalLore >= 1000 )
			iPetMax += (wSkillAnimalLore - 900) / 10;
		if ( wSkillVeterinary >= 1000 )
			iPetMax += (wSkillVeterinary - 900) / 10;
	}

	if ( iPetCount >= iPetMax )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TOOMANY));
		return false;
	}

	pCharPlayer->m_pClient->m_Targ_PrvUID = GetUID();
	pCharPlayer->m_pClient->addTarget(CLIMODE_TARG_PET_STABLE, g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG));
	return true;
}

bool CChar::NPC_StablePetRetrieve(CChar *pCharPlayer)
{
	ADDTOCALLSTACK("CChar::NPC_StablePetRetrieve");
	// Player is trying to retrieve an stabled pet on stable master NPC
	if ( !m_pNPC || (m_pNPC->m_Brain != NPCBRAIN_ANIMAL_TRAINER) || !pCharPlayer || !pCharPlayer->m_pClient )
		return false;

	int iPetCount = 0;
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContainerCreate(LAYER_BANKBOX)->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_FIGURINE) && (pItem->m_uidLink == pCharPlayer->GetUID()) )
		{
			if ( !pCharPlayer->Use_Figurine(pItem) )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_CLAIM_FOLLOWER), pItem->GetName());
				Speak(pszMsg);
				return true;
			}
			pItem->Delete();
			++iPetCount;
		}
	}

	Speak(g_Cfg.GetDefaultMsg((iPetCount > 0) ? DEFMSG_NPC_STABLEMASTER_CLAIM : DEFMSG_NPC_STABLEMASTER_CLAIM_NOPETS));
	return true;
}

void CChar::NPC_ActStart_SpeakTo(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_ActStart_SpeakTo");
	// NPC is speaking with this char
	if ( !m_pNPC || !pChar )
		return;

	m_Act_Targ = pChar->GetUID();
	m_atTalk.m_WaitCount = 20;
	m_atTalk.m_HearUnknown = 0;

	Skill_Start((pChar->Stat_GetAdjusted(STAT_FAME) > 7000) ? NPCACT_TALK_FOLLOW : NPCACT_TALK);
	SetTimeout(3 * TICK_PER_SEC);
	UpdateDir(pChar);
}

void CChar::NPC_OnHear(LPCTSTR pszCmd, CChar *pSrc, bool fAllPets)
{
	ADDTOCALLSTACK("CChar::NPC_OnHear");
	// NPC is hearing someone speak nearby
	if ( !m_pNPC || !pSrc )
		return;

	// Pets always have a basic set of actions
	if ( NPC_OnHearPetCmd(pszCmd, pSrc, fAllPets) || !NPC_CanSpeak() )
		return;

	// Check if NPC is too busy to talk
	SKILL_TYPE skill = Skill_GetActive();
	if ( (skill == NPCACT_TALK) || (skill == NPCACT_TALK_FOLLOW) )
	{
		if ( (m_Act_Targ != pSrc->GetUID()) && NPC_Act_Talk() )		// NPC is already talking with someone else
		{
			CChar *pCharOld = m_Act_Targ.CharFind();
			if ( pCharOld )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_INTERRUPT), pCharOld->GetName(), pSrc->GetName());
				Speak(pszMsg);
			}
		}
	}

	CItemMemory *pMemory = Memory_FindObjTypes(pSrc, MEMORY_SPEAK);
	if ( !pMemory )
	{
		// NPC heard this char for the first time
		if ( IsTrigUsed(TRIGGER_NPCHEARGREETING) )
		{
			if ( OnTrigger(CTRIG_NPCHearGreeting, pSrc) == TRIGRET_RET_TRUE )
				return;
		}

		pMemory = Memory_AddObjTypes(pSrc, MEMORY_SPEAK);
		if ( pMemory )
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
	}

	// Do the scripts want the NPC to take some action based on this speech?
	TALKMODE_TYPE mode = TALKMODE_SAY;
	for ( size_t i = 0; i < m_pNPC->m_Speech.GetCount(); ++i )
	{
		CResourceLink *pLink = m_pNPC->m_Speech[i];
		if ( !pLink )
			continue;

		CResourceLock s;
		if ( !pLink->ResourceLock(s) || !pLink->HasTrigger(XTRIG_UNKNOWN) )
			continue;

		TRIGRET_TYPE tr = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( (tr == TRIGRET_RET_FALSE) || (tr == TRIGRET_ENDIF) )
			continue;
		if ( (tr == TRIGRET_RET_DEFAULT) && (skill == Skill_GetActive()) )
			NPC_ActStart_SpeakTo(pSrc);		// start speaking with this char
		return;
	}

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	for ( size_t i = 0; i < pCharDef->m_Speech.GetCount(); ++i )
	{
		CResourceLink *pLink = pCharDef->m_Speech[i];
		if ( !pLink )
			continue;

		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;

		TRIGRET_TYPE tr = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( (tr == TRIGRET_RET_FALSE) || (tr == TRIGRET_ENDIF) )
			continue;
		if ( (tr == TRIGRET_RET_DEFAULT) && (skill == Skill_GetActive()) )
			NPC_ActStart_SpeakTo(pSrc);		// start speaking with this char
		return;
	}

	// Speak near healer NPCs will make them note the char faster (for resurrect purposes)
	if ( pSrc->IsStatFlag(STATF_DEAD) && (m_pNPC->m_Brain == NPCBRAIN_HEALER) )
	{
		if ( NPC_LookAtChar(pSrc) )
			return;
	}

	// NPC can't understand what this char is speaking
	if ( IsTrigUsed(TRIGGER_NPCHEARUNKNOWN) )
	{
		if ( OnTrigger(CTRIG_NPCHearUnknown, pSrc) == TRIGRET_RET_TRUE )
			return;
	}

	skill = Skill_GetActive();
	if ( (skill == NPCACT_TALK) || (skill == NPCACT_TALK_FOLLOW) )
	{
		int iMaxUnk = (GetDist(pSrc) > 4) ? 1 : 4;
		if ( m_atTalk.m_HearUnknown++ > iMaxUnk )
			Skill_Start(SKILL_NONE);	// say goodbye
	}
}

WORD CChar::NPC_GetTrainValue(CChar *pCharSrc, SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::NPC_GetTrainValue");
	if ( !pCharSrc )
		return 0;

	CVarDefCont *pVar = GetKey("OVERRIDE.TRAINSKILLMAXPERCENT", false);
	WORD wTrainVal = IMULDIV(pVar ? static_cast<WORD>(pVar->GetValNum()) : g_Cfg.m_iTrainSkillPercent, Skill_GetBase(skill), 100);

	pVar = GetKey("OVERRIDE.TRAINSKILLMAX", false);
	WORD wTrainMax = minimum(pVar ? static_cast<WORD>(pVar->GetValNum()) : g_Cfg.m_iTrainSkillMax, pCharSrc->Skill_GetMax(skill, true));
	if ( wTrainVal > wTrainMax )
		wTrainVal = wTrainMax;

	return maximum(0, wTrainVal - pCharSrc->Skill_GetBase(skill));
}

bool CChar::NPC_OnTrainPay(CChar *pCharSrc, CItemMemory *pMemory, CItem *pGold)
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainPay");
	if ( !pCharSrc || !pMemory || !pGold )
		return false;

	SKILL_TYPE skill = static_cast<SKILL_TYPE>(pMemory->m_itEqMemory.m_Skill);
	if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) || (pCharSrc->Skill_GetLock(skill) != SKILLLOCK_UP) )
	{
		pCharSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_SKILLRAISE));
		return false;
	}

	CVarDefCont *pVar = GetKey("OVERRIDE.TRAINSKILLCOST", false);
	WORD wSkillCost = pVar ? static_cast<WORD>(pVar->GetValNum()) : g_Cfg.m_iTrainSkillCost;
	if ( wSkillCost < 1 )
		wSkillCost = 1;

	WORD wSkillTrain = NPC_GetTrainValue(pCharSrc, skill);
	if ( pCharSrc->Skill_GetBase(skill) >= wSkillTrain )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_ALREADYKNOW));
		return false;
	}

	WORD wTrainCost = wSkillTrain * wSkillCost;
	if ( pGold->GetAmount() < wTrainCost )
	{
		// Received less gold, so teach less
		wSkillTrain = pGold->GetAmount() / wSkillCost;
	}

	DWORD wSkillTotal = pCharSrc->GetSkillTotal() + wSkillTrain;
	DWORD wSkillCap = pCharSrc->Skill_GetSumMax();
	if ( wSkillTotal > wSkillCap )
	{
		WORD wDecreaseTotal = 0;
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		{
			SKILL_TYPE skillCheck = static_cast<SKILL_TYPE>(i);
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skillCheck) || (pCharSrc->Skill_GetLock(skillCheck) != SKILLLOCK_DOWN) )
				continue;

			wDecreaseTotal += minimum(pCharSrc->Skill_GetBase(skillCheck), wSkillTotal - wSkillCap);
			if ( wDecreaseTotal >= wSkillTotal - wSkillCap )
				break;
		}

		if ( wDecreaseTotal > 0 )
		{
			for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
			{
				SKILL_TYPE skillCheck = static_cast<SKILL_TYPE>(i);
				if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skillCheck) || (pCharSrc->Skill_GetLock(skillCheck) != SKILLLOCK_DOWN) )
					continue;

				WORD wSkillVal = pCharSrc->Skill_GetBase(skillCheck);
				WORD wDecreaseVal = minimum(wSkillVal, wDecreaseTotal);
				pCharSrc->Skill_SetBase(skillCheck, wSkillVal - wDecreaseVal);

				wDecreaseTotal -= wDecreaseVal;
				if ( wDecreaseTotal <= 0 )
					break;
			}
		}
		else
		{
			pCharSrc->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_SKILLCAP), static_cast<int>(wSkillCap) / 10);
			return false;
		}
	}

	if ( pGold->GetAmount() > wTrainCost )
	{
		// Received more gold, so give refund
		CItem *pRefund = pGold->UnStackSplit(wTrainCost, pCharSrc);
		if ( pRefund )
			pCharSrc->ItemBounce(pRefund, false);
	}
	pGold->Delete();
	GetContainerCreate(LAYER_BANKBOX)->m_itEqBankBox.m_Check_Amount += wTrainCost;

	Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_SUCCESS));
	pCharSrc->Skill_SetBase(skill, pCharSrc->Skill_GetBase(skill) + wSkillTrain);
	pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;
	return true;
}

bool CChar::NPC_OnTrainHear(CChar *pCharSrc, LPCTSTR pszCmd)
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainHear");
	// Check the NPC is capable of teaching
	if ( !m_pNPC || !pCharSrc || (m_pNPC->m_Brain < NPCBRAIN_HUMAN) || (m_pNPC->m_Brain > NPCBRAIN_ANIMAL_TRAINER) || (m_pNPC->m_Brain == NPCBRAIN_GUARD) )
		return false;

	if ( Memory_FindObjTypes(pCharSrc, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_ENEMY));
		return true;
	}

	if ( pszCmd )
	{
		// If an specific skill name is given, look for this specific skill
		SKILL_TYPE skill = SKILL_NONE;
		for ( unsigned int i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		{
			SKILL_TYPE skillCheck = static_cast<SKILL_TYPE>(i);
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skillCheck) )
				continue;

			if ( FindStrWord(pszCmd, g_Cfg.GetSkillKey(skillCheck)) != 0 )
			{
				skill = skillCheck;
				break;
			}
		}

		if ( !IsSkillBase(skill) || (Skill_GetBase(skill) <= 0) )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_CANTTEACH));
			return true;
		}

		WORD wSkillTrain = NPC_GetTrainValue(pCharSrc, skill);
		if ( pCharSrc->Skill_GetBase(skill) >= wSkillTrain )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_ALREADYKNOW));
			return true;
		}

		CVarDefCont *pVar = GetKey("OVERRIDE.TRAINSKILLCOST", false);
		WORD wSkillCost = pVar ? pVar->GetValNum() : g_Cfg.m_iTrainSkillCost;
		WORD wTrainCost = wSkillTrain * wSkillCost;

		CItemMemory *pMemory = Memory_AddObjTypes(pCharSrc, MEMORY_SPEAK);
		if ( !pMemory )
			return false;

		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_TRAIN;
		pMemory->m_itEqMemory.m_Skill = static_cast<WORD>(skill);

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE_FULL), static_cast<int>(wTrainCost));
		Speak(pszMsg);
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE_LESS));
		return true;
	}

	// Show all skills available to train
	TCHAR *pszSkillList = Str_GetTemp();
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		SKILL_TYPE skillCheck = static_cast<SKILL_TYPE>(i);
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(skillCheck) )
			continue;
		if ( Skill_GetBase(skillCheck) <= 0 )
			continue;

		WORD wSkillTrain = NPC_GetTrainValue(pCharSrc, skillCheck);
		if ( wSkillTrain <= 0 )
			continue;

		if ( *pszSkillList )
			strcat(pszSkillList, ", ");
		strncat(pszSkillList, g_Cfg.GetSkillKey(skillCheck), MAX_TALK_BUFFER - 1);
	}

	if ( *pszSkillList )
	{
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_TEACH), pszSkillList);
		Speak(pszMsg);
	}
	else
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_NOTHING));
	return true;
}

void CChar::NPC_OnNoticeSnoop(CChar *pCharThief, CChar *pCharMark)
{
	ADDTOCALLSTACK("CChar::NPC_OnNoticeSnoop");
	if ( !m_pNPC || !pCharThief || (pCharMark != this) )
		return;

	if ( NPC_CanSpeak() )
	{
		static LPCTSTR const sm_szTextSnoop[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SNOOPED_1),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SNOOPED_2),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SNOOPED_3),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SNOOPED_4)
		};
		Speak(sm_szTextSnoop[Calc_GetRandVal(COUNTOF(sm_szTextSnoop))]);
	}

	if ( !Calc_GetRandVal(4) )
	{
		// Flee from the thief
		m_Act_Targ = pCharThief->GetUID();
		m_atFlee.m_iStepsMax = 20;
		m_atFlee.m_iStepsCurrent = 0;
		Skill_Start(NPCACT_FLEE);
	}
}

int CChar::NPC_WalkToPoint(bool fRun)
{
	ADDTOCALLSTACK("CChar::NPC_WalkToPoint");
	// Move toward target point (which should be already set)
	// RETURN:
	//  0 = char is already here
	//  1 = char took the step
	//  2 = char can't take this step right now (obstacle)
	if ( Can(CAN_C_NONMOVER) )
		return 0;

	EXC_TRY("NPC_WalkToPoint");
	CPointMap ptMe = GetTopPoint();
	DIR_TYPE dir = ptMe.GetDir(m_Act_p);
	if ( dir >= DIR_QTY )
		return 0;		// char is already in the spot

	int iDex = Stat_GetAdjusted(STAT_DEX);
	if ( iDex <= 0 )
		return 2;		// char can't move now

	// Use pathfinding
	EXC_SET("NPC_AI_PATH");
	bool fUsePathfinding = false;
	if ( NPC_GetAiFlags() & NPC_AI_PATH )
	{
		NPC_Pathfinding();

		// Walk the saved path
		CPointMap ptLocal;
		ptLocal.m_x = m_pNPC->m_nextX[0];
		ptLocal.m_y = m_pNPC->m_nextY[0];
		ptLocal.m_map = ptMe.m_map;

		// No steps available yet (or pathfinding not usable in this situation), so use default movement
		if ( (ptLocal.m_x > 0) && (ptLocal.m_y > 0) )
		{
			fUsePathfinding = true;
			if ( ptMe.GetDist(ptLocal) != 1 )
			{
				// The next step is too far away, pathfinding route has become invalid
				m_pNPC->m_nextPt.InitPoint();
				m_pNPC->m_nextX[0] = 0;
				m_pNPC->m_nextY[0] = 0;
			}
			else
			{
				// Update heading to the way char need to go
				dir = ptMe.GetDir(ptLocal);
				ASSERT((dir > DIR_INVALID) && (dir < DIR_QTY));

				EXC_TRYSUB("Array Shift");
				for ( int i = 0; i < (MAX_NPC_PATH_STORAGE_SIZE - 1); ++i )
				{
					m_pNPC->m_nextX[i] = m_pNPC->m_nextX[i + 1];
					m_pNPC->m_nextY[i] = m_pNPC->m_nextY[i + 1];
				}
				m_pNPC->m_nextX[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				m_pNPC->m_nextY[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				EXC_CATCHSUB("NPCAI");
			}
		}
	}

	EXC_SET("Non-advanced Pathfinding");
	CCharBase *pCharDef = Char_GetDef();
	ptMe.Move(dir);
	if ( !CanMoveWalkTo(ptMe, true, false, dir) )
	{
		// Try to step around it
		int iDiff = 0;
		int iRand = Calc_GetRandVal(100);
		if ( iRand < 30 )	// do nothing
		{
			// While pathfinding, char should keep trying to find new ways to our destination
			if ( fUsePathfinding )
			{
				SetTimeout(TICK_PER_SEC);	// wait a moment before try to find a new route
				return 1;
			}
			return 2;
		}

		if ( iRand < 35 )
			iDiff = 4;
		else if ( iRand < 40 )
			iDiff = 3;
		else if ( iRand < 65 )
			iDiff = 2;
		else
			iDiff = 1;

		if ( iRand & 1 )
			iDiff = -iDiff;

		CPointMap ptFirstTry = ptMe;
		ptMe = GetTopPoint();
		dir = GetDirTurn(dir, iDiff);
		ptMe.Move(dir);
		if ( !CanMoveWalkTo(ptMe, true, false, dir) )
		{
			bool fClearedWay = false;

			// Check if char can move objects placed on the way
			if ( (NPC_GetAiFlags() & NPC_AI_MOVEOBSTACLES) && pCharDef->Can(CAN_C_USEHANDS) && !IsStatFlag(STATF_DEAD|STATF_Freeze|STATF_Stone) )
			{
				int iInt = Stat_GetAdjusted(STAT_INT);
				if ( iInt > iRand )
				{
					CPointMap pt;
					for ( int i = 0; i < 2; ++i )
					{
						pt = (!i) ? ptMe : ptFirstTry;

						CWorldSearch AreaItems(pt);
						for (;;)
						{
							CItem *pItem = AreaItems.GetItem();
							if ( !pItem )
								break;
							if ( abs(pItem->GetTopZ() - ptMe.m_z) > 5 )
								continue;

							if ( CanMove(pItem) && CanCarry(pItem) )
							{
								// Move item to char position
								pItem->MoveToUpdate(GetTopPoint());
								fClearedWay = true;
								break;
							}
						}

						if ( fClearedWay || (iInt < iRand * 3) )	// if path is not cleared but char is clever enough, it should try to move in the first step it was trying to move to
							break;
					}

					if ( fClearedWay && (pt == ptFirstTry) )
					{
						dir = GetTopPoint().GetDir(m_Act_p);
						ASSERT((dir > DIR_INVALID) && (dir < DIR_QTY));
						if ( dir >= DIR_QTY )
							fClearedWay = false;
					}
				}
			}
			if ( !fClearedWay )
			{
				// While pathfinding, char should keep trying to find new ways to our destination
				if ( fUsePathfinding )
				{
					SetTimeout(TICK_PER_SEC);	// wait a moment before try to find a new route
					return 1;
				}
				return 2;
			}
		}
	}

	EXC_SET("Finishing Move Action");
	ASSERT((dir > DIR_INVALID) && (dir < DIR_QTY));
	m_dirFace = dir;
	if ( fRun && (!pCharDef->Can(CAN_C_RUN|CAN_C_FLY) || (Stat_GetVal(STAT_DEX) <= 1)) )
		fRun = false;

	CPointMap ptOld = GetTopPoint();
	StatFlag_Mod(STATF_Fly, fRun);
	CheckRevealOnMove();
	MoveToChar(ptMe);

	EXC_SET("Check Location");
	if ( CheckLocation() == TRIGRET_RET_FALSE )		// check if char stepped on item/teleport
	{
		SetTopPoint(ptOld);		// char already moved, so move back to previous location
		return 2;
	}

	EXC_SET("Move Update");
	UpdateMove(ptOld);

	EXC_SET("Set Timeout");
	CVarDefCont *pVar = GetKey("OVERRIDE.MOVERATE", true);
	INT64 iMoveRate = pVar ? pVar->GetValNum() : pCharDef->m_iMoveRate;
	INT64 iNextTick;

	if ( fRun )
	{
		if ( IsStatFlag(STATF_Pet) && (iDex < 75) )		// pets run a bit faster
			iDex = 75;
		iNextTick = (TICK_PER_SEC / 4) + Calc_GetRandLLVal((100 - (iDex * iMoveRate) / 100) / 5);
	}
	else
		iNextTick = TICK_PER_SEC + Calc_GetRandLLVal((100 - (iDex * iMoveRate) / 100) / 3);

	if ( iNextTick < 1 )
		iNextTick = 1;
	else if ( iNextTick > 50 )
		iNextTick = 50;

	SetTimeout(iNextTick);
	EXC_CATCH;
	return 1;
}

bool CChar::NPC_LookAtCharGuard(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharGuard");
	// Guard NPC is looking at char
	if ( !pChar || (pChar == this) )
		return false;
	if ( !m_pArea || !pChar->m_pArea || !pChar->m_pArea->IsGuarded() )
		return false;
	if ( pChar->IsStatFlag(STATF_INVUL|STATF_DEAD) || pChar->IsPriv(PRIV_GM|PRIV_JAILED) )
		return false;
	if ( !(pChar->IsStatFlag(STATF_Criminal) || (g_Cfg.m_fGuardsOnMurderers && pChar->Noto_IsEvil())) )
		return false;

	if ( g_Cfg.m_fGuardsInstantKill && (GetTopDist3D(pChar) > CalcFightRange(m_uidWeapon.ItemFind())) )
		Spell_Teleport(pChar->GetTopPoint(), false, false);

	if ( NPC_CanSpeak() && !Calc_GetRandVal(3) )
	{
		static LPCTSTR const sm_szGuardStrike[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GUARD_STRIKE_1),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GUARD_STRIKE_2),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GUARD_STRIKE_3),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GUARD_STRIKE_4),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_GUARD_STRIKE_5)
		};
		Speak(sm_szGuardStrike[Calc_GetRandVal(COUNTOF(sm_szGuardStrike))]);
	}

	Fight_Attack(pChar, true);
	return true;
}

bool CChar::NPC_LookAtCharMonster(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharMonster");
	// Monster NPC is looking at char
	if ( !m_pNPC || !pChar )
		return false;

	if ( Fight_IsActive() && (m_Act_Targ == pChar->GetUID()) )	// already attacking this char
		return false;
	int iMotivation = NPC_GetAttackMotivation(pChar);
	if ( (iMotivation <= 0) || (iMotivation < m_pNPC->m_Act_Motivation) )
		return false;
	if ( !Fight_Attack(pChar) )
		return false;

	m_pNPC->m_Act_Motivation = static_cast<BYTE>(iMotivation);
	return true;
}

bool CChar::NPC_LookAtCharHuman(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHuman");
	// Human NPC is looking at char
	if ( !m_pNPC || !pChar || pChar->IsStatFlag(STATF_DEAD) )
		return false;

	bool fCriminal = (pChar->IsStatFlag(STATF_Criminal) || (pChar->Noto_IsEvil() && g_Cfg.m_fGuardsOnMurderers) || pChar->NPC_IsMonster());
	if ( !fCriminal )
		return false;

	// Yell for guards if NPC see someone evil
	if ( NPC_CanSpeak() && m_pArea->IsGuarded() && !Calc_GetRandVal(3) )
	{
		Speak(g_Cfg.GetDefaultMsg(pChar->m_pPlayer ? DEFMSG_NPC_GENERIC_SEECRIM : DEFMSG_NPC_GENERIC_SEEMONS));
		CallGuards(pChar);

		if ( !IsStatFlag(STATF_War) )
		{
			// Flee from the threat
			m_Act_Targ = pChar->GetUID();
			m_atFlee.m_iStepsMax = 20;
			m_atFlee.m_iStepsCurrent = 0;
			Skill_Start(NPCACT_FLEE);
			m_pNPC->m_Act_Motivation = 80;
			return true;
		}
	}
	return false;
}

bool CChar::NPC_LookAtCharHealer(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHealer");
	// Healer NPC is looking at char
	if ( !pChar || !pChar->IsStatFlag(STATF_DEAD) || (pChar->m_pNPC && pChar->m_pNPC->m_bonded) )
		return false;

	UpdateDir(pChar);

	if ( GetDist(pChar) > 3 )
	{
		if ( Calc_GetRandVal(5) )
			return false;
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RANGE));
		return true;
	}
	else if ( pChar->IsStatFlag(STATF_Insubstantial) )
	{
		if ( Calc_GetRandVal(5) )
			return false;
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_MANIFEST));
		return true;
	}

	NOTO_TYPE notoHealer = Noto_GetFlag(this);
	NOTO_TYPE notoTarg = pChar->Noto_GetFlag(this);

	if ( (notoHealer != NOTO_CRIMINAL) && (notoTarg == NOTO_CRIMINAL) )
	{
		static LPCTSTR const sm_szHealerRefuseCriminals[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_CRIM_1),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_CRIM_2),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_CRIM_3)
		};

		if ( Calc_GetRandVal(5) )
			return false;
		Speak(sm_szHealerRefuseCriminals[Calc_GetRandVal(COUNTOF(sm_szHealerRefuseCriminals))]);
		return true;
	}

	if ( (notoHealer == NOTO_GOOD) && (notoTarg == NOTO_EVIL) )
	{
		static LPCTSTR const sm_szHealerRefuseEvils[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_EVIL_1),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_EVIL_2),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_EVIL_3)
		};

		if ( Calc_GetRandVal(5) )
			return false;
		Speak(sm_szHealerRefuseEvils[Calc_GetRandVal(COUNTOF(sm_szHealerRefuseEvils))]);
		return true;
	}

	if ( ((notoHealer == NOTO_CRIMINAL) || (notoHealer == NOTO_EVIL)) && (notoTarg == NOTO_GOOD) )
	{
		static LPCTSTR const sm_szHealerRefuseGoods[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_GOOD_1),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_GOOD_2),
			g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_REF_GOOD_3)
		};

		if ( Calc_GetRandVal(5) )
			return false;
		Speak(sm_szHealerRefuseGoods[Calc_GetRandVal(COUNTOF(sm_szHealerRefuseGoods))]);
		return true;
	}

	// Attempt to resurrect
	static LPCTSTR const sm_szHealer[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RES_1),
		g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RES_2),
		g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RES_3),
		g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RES_4),
		g_Cfg.GetDefaultMsg(DEFMSG_NPC_HEALER_RES_5)
	};

	Speak(sm_szHealer[Calc_GetRandVal(COUNTOF(sm_szHealer))]);
	UpdateAnimate(ANIM_CAST_AREA);
	if ( !pChar->OnSpellEffect(SPELL_Resurrection, this, 1000, NULL) )
		Speak(g_Cfg.GetDefaultMsg(Calc_GetRandVal(2) ? DEFMSG_NPC_HEALER_FAIL_1 : DEFMSG_NPC_HEALER_FAIL_2));

	return true;
}

bool CChar::NPC_LookAtItem(CItem *pItem)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtItem");
	// NPC is looking at item
	if ( !Can(CAN_C_USEHANDS) || !CanSee(pItem) )
		return false;

	int iDist = GetTopDist(pItem);
	int iWantThisItem = NPC_WantThisItem(pItem);

	if ( IsTrigUsed(TRIGGER_NPCLOOKATITEM) )
	{
		if ( !pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN|ATTR_SECURE) )
		{
			CScriptTriggerArgs Args(iDist, iWantThisItem, pItem);
			TRIGRET_TYPE tr = OnTrigger(CTRIG_NPCLookAtItem, this, &Args);
			if ( tr == TRIGRET_RET_TRUE )
				return true;
			else if ( tr == TRIGRET_RET_FALSE )
				return false;

			iWantThisItem = static_cast<int>(Args.m_iN2);
		}
	}

	if ( NPC_GetAiFlags() & NPC_AI_LOOTING )
	{
		// Loot nearby items on ground
		if ( iWantThisItem > Calc_GetRandVal(100) )
		{
			m_Act_p = pItem->GetTopPoint();
			m_Act_Targ = pItem->GetUID();
			NPC_Act_Looting();
			return true;
		}

		// Loot nearby corpses
		if ( pItem->IsType(IT_CORPSE) )
		{
			if ( Memory_FindObj(pItem) )	// already looted this corpse
				return false;

			m_Act_p = pItem->GetTopPoint();
			m_Act_Targ = pItem->GetUID();
			NPC_Act_Looting();
			return true;
		}
	}

	// Check nearby doors
	if ( pItem->IsType(IT_DOOR) )
	{
		if ( !Calc_GetRandVal(2) || (iDist > 1) )
			return false;

		// Try to open it
		if ( !Use_Item(pItem) )
			return false;

		// Walk through it
		CPointMap pt = GetTopPoint();
		pt.MoveN(GetDir(pItem), 2);
		if ( CanMoveWalkTo(pt) )
		{
			m_Act_p = pt;
			NPC_WalkToPoint();
			return true;
		}
	}
	return false;
}

bool CChar::NPC_LookAtChar(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_LookAtChar");
	// NPC is looking at char
	// RETURN:
	//  true = start new action
	if ( !m_pNPC || !pChar || (pChar == this) || !CanSeeLOS(pChar, LOS_NB_WINDOWS) )
		return false;

	if ( IsTrigUsed(TRIGGER_NPCLOOKATCHAR) )
	{
		TRIGRET_TYPE tr = OnTrigger(CTRIG_NPCLookAtChar, pChar);
		if ( tr == TRIGRET_RET_TRUE )
			return true;
		else if ( tr == TRIGRET_RET_FALSE )
			return false;
	}

	if ( NPC_IsOwnedBy(pChar, false) )
	{
		// Follow its owner (default action)
		m_Act_Targ = pChar->GetUID();
		m_pNPC->m_Act_Motivation = 50;
		Skill_Start((Skill_GetActive() == NPCACT_FOLLOW_TARG) ? NPCACT_FOLLOW_TARG : NPCACT_GUARD_TARG);
		return true;
	}

	if ( IsStatFlag(STATF_DEAD) )
		return false;

	// Try to speak with players
	if ( pChar->m_pPlayer && !IsStatFlag(STATF_War) && ((Skill_GetActive() == SKILL_NONE) || (Skill_GetActive() == NPCACT_WANDER)) && !Memory_FindObjTypes(pChar, MEMORY_SPEAK) )
	{
		if ( IsTrigUsed(TRIGGER_NPCSEENEWPLAYER) )
		{
			if ( OnTrigger(CTRIG_NPCSeeNewPlayer, pChar) != TRIGRET_RET_TRUE )
			{
				CItemMemory *pMemory = Memory_AddObjTypes(pChar, MEMORY_SPEAK);
				if ( pMemory )
					pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
			}
		}
	}

	switch ( m_pNPC->m_Brain )
	{
		case NPCBRAIN_GUARD:
			return NPC_LookAtCharGuard(pChar);
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_BERSERK:
		case NPCBRAIN_DRAGON:
			return NPC_LookAtCharMonster(pChar);
		case NPCBRAIN_HEALER:
			if ( NPC_LookAtCharHealer(pChar) )
				return true;
			// fall through
		case NPCBRAIN_BANKER:
		case NPCBRAIN_VENDOR:
		case NPCBRAIN_ANIMAL_TRAINER:
		case NPCBRAIN_ANIMAL:
		case NPCBRAIN_HUMAN:
			return NPC_LookAtCharHuman(pChar);
		default:
			return false;
	}
}

bool CChar::NPC_LookAround(bool fCheckItems)
{
	ADDTOCALLSTACK("CChar::NPC_LookAround");
	// NPC is looking around for items/chars
	// It may be doing something already, so check current action motivation level
	// RETURN:
	//  true = found something better to do
	if ( !m_pNPC )
		return false;

	CSector *pSector = GetTopSector();
	if ( !pSector )
		return false;

	if ( (m_pNPC->m_Brain == NPCBRAIN_BERSERK) || !Calc_GetRandVal(6) )
		SoundChar(CRESND_RAND);		// make some random noise

	int iViewSight = GetSight();

	if ( !Char_GetDef()->Can(CAN_C_SWIM|CAN_C_WALK|CAN_C_FLY|CAN_C_HOVER|CAN_C_RUN) || IsStatFlag(STATF_Freeze|STATF_Stone) )
	{
		// Don't look too far it NPC can't move and can't cast spells at distance
		if ( !NPC_FightMayCast() )
			iViewSight = 2;
	}
	else
	{
		if ( !NPC_CheckWalkHere(GetTopPoint(), m_pArea) )
		{
			// NPC stepped on fire/trap/etc and should run away
			m_Act_p = GetTopPoint();
			m_Act_p.Move(static_cast<DIR_TYPE>(Calc_GetRandVal(DIR_QTY)));
			NPC_WalkToPoint(true);
			return true;
		}
	}

	// If sector is too complex, lower the view range to avoid waste too much server performance
	if ( pSector->GetCharComplexity() > g_Cfg.m_iMaxCharComplexity / 2 )
		iViewSight /= 4;

	// Check nearby chars
	CChar *pChar = NULL;
	CWorldSearch AreaChars(GetTopPoint(), iViewSight);
	for (;;)
	{
		pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )
			continue;
		if ( NPC_LookAtChar(pChar) )
			return true;
	}

	// Check nearby items
	if ( !fCheckItems && (Stat_GetAdjusted(STAT_INT) > 10) && !IsSkillBase(Skill_GetActive()) && !Calc_GetRandVal(4) )
		fCheckItems = true;

	if ( fCheckItems )
	{
		CItem *pItem = NULL;
		CWorldSearch AreaItems(GetTopPoint(), iViewSight);
		for (;;)
		{
			pItem = AreaItems.GetItem();
			if ( !pItem )
				break;
			if ( abs(pItem->GetTopZ() - GetTopZ()) > 5 )
				continue;
			if ( NPC_LookAtItem(pItem) )
				return true;
		}
	}
	return false;
}

void CChar::NPC_Act_Wander()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Wander");
	// NPCACT_WANDER
	// Just wander aimlessly
	if ( Can(CAN_C_NONMOVER) )
		return;

	if ( !Calc_GetRandVal(7 + (Stat_GetVal(STAT_DEX) / 30)) )
	{
		// Stop wandering
		Skill_Start(SKILL_NONE);
		return;
	}
	else if ( Calc_GetRandVal(2) )
	{
		if ( NPC_LookAround() )
			return;
	}

	// Go back to home point when walk too far
	m_Act_p = GetTopPoint();
	m_Act_p.Move(GetDirTurn(m_dirFace, 1 - Calc_GetRandVal(3)));

	if ( m_pNPC->m_Home_Dist_Wander && m_ptHome.IsValidPoint() )
	{
		if ( m_Act_p.GetDist(m_ptHome) > m_pNPC->m_Home_Dist_Wander )
		{
			Skill_Start(NPCACT_GO_HOME);
			return;
		}
	}

	NPC_WalkToPoint();
}

void CChar::NPC_Act_Guard()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Guard");
	// NPCACT_GUARD_TARG
	// Protect the target if it's in a fight (m_Act_Targ)
	if ( !m_pNPC )
		return;

	CChar *pChar = m_Act_Targ.CharFind();
	if ( pChar && (pChar != this) && CanSee(pChar) )
	{
		if ( pChar->Fight_IsActive() )
		{
			if ( Fight_Attack(pChar->m_Fight_Targ.CharFind()) )
				return;
		}
	}

	// Target is not fighting, so just follow him
	NPC_Act_Follow();
}

bool CChar::NPC_Act_Follow(bool fFlee, int iMaxDist, bool fMoveAway)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Follow");
	// NPCACT_FOLLOW_TARG
	// Follow the target (m_Act_Targ / m_Fight_Targ)
	// RETURN:
	//  true = keep following
	//  false = can't follow anymore, give up
	if ( Can(CAN_C_NONMOVER) )
		return false;

	EXC_TRY("NPC_Act_Follow")
	CChar *pChar = Fight_IsActive() ? m_Fight_Targ.CharFind() : m_Act_Targ.CharFind();
	if ( !pChar )
	{
		Skill_Start(SKILL_NONE);
		return false;
	}

	EXC_SET("Trigger");
	if ( IsTrigUsed(TRIGGER_NPCACTFOLLOW) )
	{
		CScriptTriggerArgs Args(fFlee, iMaxDist, fMoveAway);
		TRIGRET_TYPE tr = OnTrigger(CTRIG_NPCActFollow, pChar, &Args);
		if ( tr == TRIGRET_RET_TRUE )
			return false;
		else if ( tr == TRIGRET_RET_FALSE )
			return true;

		fFlee = static_cast<bool>(Args.m_iN1);
		iMaxDist = static_cast<int>(Args.m_iN2);
		fMoveAway = static_cast<bool>(Args.m_iN3);
	}

	EXC_SET("CanSee");
	if ( CanSee(pChar) )
	{
		// Keep following the target if can see it
		m_Act_p = pChar->GetTopPoint();
	}
	else
	{
		// Can't see the target anymore, so eventually stop following it
		if ( fFlee || !Calc_GetRandVal(1 + ((100 - Stat_GetAdjusted(STAT_INT)) / 20)) )
			return false;
	}

	EXC_SET("Distance checks");
	int iDist = GetTopPoint().GetDist(m_Act_p);
	if ( iDist > UO_MAP_VIEW_RADAR )
		return false;

	if ( fMoveAway )
	{
		if ( iDist < iMaxDist )
			fFlee = true;	// start moving away
	}
	else
	{
		if ( fFlee && (iDist >= iMaxDist) )
			return false;
		if ( iDist <= iMaxDist )
			return true;
	}

	EXC_SET("Fleeing");
	if ( fFlee )
	{
		CPointMap ptOld = m_Act_p;
		m_Act_p = GetTopPoint();
		m_Act_p.Move(GetDirTurn(m_Act_p.GetDir(ptOld), 4 + 1 - Calc_GetRandVal(3)));
		NPC_WalkToPoint(iDist > 3);
		m_Act_p = ptOld;	// last known point of the target
		return true;
	}

	EXC_SET("WalkToPoint");
	NPC_WalkToPoint(IsStatFlag(STATF_War) || (iDist > 3));
	return true;

	EXC_CATCH;
	return false;
}

bool CChar::NPC_FightArchery(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_FightArchery");
	// Check if NPC can fight using archery
	// RETURN:
	//  false = switch to melee combat
	if ( !pChar || !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_RANGED) )
		return false;

	CItem *pWeapon = m_uidWeapon.ItemFind();
	int iMinDist = pWeapon ? pWeapon->RangeH() : g_Cfg.m_iArcheryMinDist;
	int iMaxDist = pWeapon ? pWeapon->RangeL() : g_Cfg.m_iArcheryMaxDist;
	if ( !iMaxDist || ((iMinDist == 0) && (iMaxDist == 1)) )
		iMaxDist = g_Cfg.m_iArcheryMaxDist;
	if ( !iMinDist )
		iMinDist = g_Cfg.m_iArcheryMinDist;

	int iDist = GetTopDist(pChar);
	if ( iDist < iMinDist )
	{
		// Target is too close, switch to melee combat
		return false;
	}
	else if ( iDist > iMaxDist )
	{
		// Target is too far, try to get closer
		NPC_Act_Follow(false, iMaxDist, false);
	}
	return true;
}

void CChar::NPC_GetAllSpellbookSpells()
{
	ADDTOCALLSTACK("CChar::NPC_GetAllSpellbookSpells");
	// Add spells found on spellbooks to the NPC internal spell list

	// Search on hands
	for ( CItem *pBook = GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
	{
		if ( pBook->IsTypeSpellbook() )
			NPC_AddSpellsFromBook(pBook);
	}

	// Search on backpack
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( pPack )
	{
		for ( CItem *pBook = pPack->GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
		{
			if ( pBook->IsTypeSpellbook() )
				NPC_AddSpellsFromBook(pBook);
		}
	}
}

void CChar::NPC_AddSpellsFromBook(CItem *pBook)
{
	ADDTOCALLSTACK("CChar::NPC_AddSpellsFromBook");
	CItemBase *pBookDef = pBook->Item_GetDef();
	if ( !pBookDef )
		return;

	DWORD dwMin = pBookDef->m_ttSpellbook.m_Offset + 1;
	DWORD dwMax = pBookDef->m_ttSpellbook.m_Offset + pBookDef->m_ttSpellbook.m_MaxSpells;

	SPELL_TYPE spell = SPELL_NONE;
	for ( DWORD i = dwMin; i <= dwMax; ++i )
	{
		spell = static_cast<SPELL_TYPE>(i);
		if ( pBook->IsSpellInBook(spell) )
			m_pNPC->Spells_Add(spell);
	}
}

bool CChar::NPC_FightMagery(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NPC_FightMagery");
	// Check if NPC can fight using magery
	// RETURN:
	//  false = switch to melee combat

	if ( !pChar || !NPC_FightMayCast(false) )		// not checking skill here since it will do a search later and it's an expensive function
		return false;

	int iSpellCount = m_pNPC->Spells_GetCount();
	if ( iSpellCount < 1 )
		return false;

	int iDist = GetTopDist(pChar);
	if ( (iDist < 3) || (iDist > GetSight()) )
		return false;

	// Chance to cast
	int iInt = Stat_GetBase(STAT_INT);
	int iMana = Stat_GetVal(STAT_INT);
	int iChance = ((iMana >= (iInt / 2)) ? iMana : (iInt - iMana));
	if ( Calc_GetRandVal(iChance) < iInt / 4 )
	{
		// Failed, but NPC could be casting next time
		if ( (iMana > iInt / 3) && Calc_GetRandVal(iInt) )
		{
			if ( iDist < 4 )
			{
				// Target is too close, try walk away a bit
				NPC_Act_Follow(false, 4, true);
			}
			else if ( iDist > 8 )
			{
				// Target is too far, try to get closer
				NPC_Act_Follow(false, 0, false);
			}
			return true;
		}
		return false;
	}

	CObjBase *pSrc = this;
	CObjBase *pTarg = pChar;
	int iRandSpell = Calc_GetRandVal2(0, iSpellCount - 1);		// spells are stored on a zero-based vector
	int iSkill = SKILL_MAGERY;
	int iSkillReq = 0;

	CItem *pWand = LayerFind(LAYER_HAND1);
	if ( pWand && pWand->IsType(IT_WAND) )
	{
		if ( pWand->m_itWeapon.m_spellcharges > 0 )
		{
			SPELL_TYPE spell = static_cast<SPELL_TYPE>(pWand->m_itWeapon.m_spell);
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
			if ( pSpellDef )
			{
				pSrc = pWand;		// set pWand as SRC to make @SpellCast trigger use it as ARGO
				if ( NPC_FightCast(pTarg, pWand, spell) )
					goto BeginCast;
			}
		}

		// Wand is out of charge or doesn't have a valid spell, so unequip it
		ItemBounce(pWand);
	}

	for ( ; iRandSpell < iSpellCount; ++iRandSpell )
	{
		SPELL_TYPE spell = m_pNPC->Spells_GetAt(static_cast<size_t>(iRandSpell));
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
		if ( !pSpellDef )
			continue;

		if ( !pSpellDef->GetPrimarySkill(&iSkill, &iSkillReq) )
			iSkill = SKILL_MAGERY;
		if ( Skill_GetBase(static_cast<SKILL_TYPE>(iSkill)) < iSkillReq )
			continue;

		if ( NPC_FightCast(pTarg, this, spell) )
			goto BeginCast;
	}

	// Can't cast any spell, switch to melee combat
	return false;

BeginCast:
	Reveal();
	m_Act_Targ = pTarg->GetUID();
	m_Act_TargPrv = pSrc->GetUID();
	m_Act_p = pTarg->GetTopPoint();
	return Skill_Start(static_cast<SKILL_TYPE>(iSkill));
}

bool CChar::NPC_FightCast(CObjBase *&pTarg, CObjBase *pSrc, SPELL_TYPE &spell)
{
	ADDTOCALLSTACK("CChar::NPC_FightCast");
	// NPC can fight using magery, so check if it can cast the spell

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pTarg || !pSpellDef || pSpellDef->IsSpellType(SPELLFLAG_PLAYERONLY) )
		return false;

	if ( !Spell_CanCast(spell, true, pSrc, false) )
		return false;

	if ( !pSpellDef->IsSpellType(SPELLFLAG_HARM) )
	{
		if ( pSpellDef->IsSpellType(SPELLFLAG_GOOD) )
		{
			if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pTarg->IsChar() )
			{
				// Help itself or friends when needed
				CChar *pFriend[4];
				pFriend[0] = this;
				pFriend[1] = pFriend[2] = pFriend[3] = NULL;
				int iFriendIndex = 1;

				CChar *pTarget = pTarg->GetUID().CharFind();
				if ( NPC_GetAiFlags() & NPC_AI_COMBAT )
				{
					CWorldSearch AreaChars(GetTopPoint(), GetSight());
					for (;;)
					{
						pTarget = AreaChars.GetChar();
						if ( !pTarget )
							break;

						CItemMemory *pMemory = pTarget->Memory_FindObj(pTarg);
						if ( pMemory && pMemory->IsMemoryTypes(MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY) )
						{
							pFriend[iFriendIndex++] = pTarget;
							if ( iFriendIndex >= 4 )
								break;
						}
					}
				}

				// Can't cast this on self, so cast it on friends
				if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF) )
				{
					pFriend[0] = pFriend[1];
					pFriend[1] = pFriend[2];
					pFriend[2] = pFriend[3];
					pFriend[3] = NULL;
				}

				bool fSpellSuits = false;
				for ( iFriendIndex = 0; iFriendIndex < 4; ++iFriendIndex )
				{
					pTarget = pFriend[iFriendIndex];
					if ( !pTarget )
						break;

					// Check if the target need that
					switch ( spell )
					{
						// Healing is top priority
						case SPELL_Heal:
						case SPELL_Great_Heal:
							if ( pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 3 )
								fSpellSuits = true;
							break;
						case SPELL_Gift_of_Renewal:
							if ( pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 2 )
								fSpellSuits = true;
							break;

						// Then cure poison
						case SPELL_Cure:
							if ( pTarget->LayerFind(LAYER_FLAG_Poison) )
								fSpellSuits = true;
							break;

						// Buffs are coming now
						case SPELL_Reactive_Armor:
							if ( !pTarget->LayerFind(LAYER_SPELL_Reactive) )
								fSpellSuits = true;
							break;
						case SPELL_Protection:
							if ( !pTarget->LayerFind(LAYER_SPELL_Protection) )
								fSpellSuits = true;
							break;
						case SPELL_Magic_Reflect:
							if ( !pTarget->LayerFind(LAYER_SPELL_Magic_Reflect) )
								fSpellSuits = true;
							break;
						case SPELL_Bless:
							if ( !pTarget->LayerFind(LAYER_SPELL_STATS) )
								fSpellSuits = true;
							break;
					}

					if ( fSpellSuits )
						break;

					// If it's a buff, check if target is already under this spell effect
					LAYER_TYPE layer = pSpellDef->m_idLayer;
					if ( (layer != LAYER_NONE) && !pTarget->LayerFind(layer) )
					{
						fSpellSuits = true;
						break;
					}

				}
				if ( fSpellSuits )
				{
					pTarg = pTarget;
					m_atMagery.m_Spell = spell;
					return true;
				}
				return false;
			}
			else if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_ITEM) )
			{
				// Spell is good, but must be targeted at an item
				if ( spell == SPELL_Immolating_Weapon )
					pTarg = m_uidWeapon.ObjFind();
			}
			if ( pSpellDef->IsSpellType(SPELLFLAG_HEAL) )
			{
				// No spells added ATM until they are created, good example spell to here = SPELL_Healing_Stone
				if ( spell == SPELL_Healing_Stone )
				{
					CItem *pStone = GetBackpackItem(ITEMID_HEALING_STONE);
					if ( pStone )
					{
						if ( (pStone->m_itNormal.m_morep.m_z == 0) && (Stat_GetVal(STAT_STR) < static_cast<int>(pStone->m_itNormal.m_more2)) && (pStone->m_itNormal.m_more1 >= pStone->m_itNormal.m_more2) )
						{
							Use_Obj(pStone, false);
							return true;
						}
						return false;	// target already have a stone
					}
				}

				pTarg = this;
				m_atMagery.m_Spell = spell;
				return true;
			}
		}
		else if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
		{
			m_atMagery.m_Spell = spell;
			return true;
		}
	}
	m_atMagery.m_Spell = spell;
	return true;
}

CChar *CChar::NPC_FightFindBestTarget()
{
	ADDTOCALLSTACK("CChar::NPC_FightFindBestTarget");
	// Find the best target to attack
	// Switch to this new target even if NPC is already attacking someone
	if ( !m_pNPC )
		return NULL;

	CChar *pTarg = NULL;
	if ( m_lastAttackers.size() )
	{
		int iClosest = INT_MAX;
		INT64 iThreat = 0;
		CChar *pChar = NULL;
		bool fRangedSkill = g_Cfg.IsSkillFlag(Fight_GetWeaponSkill(), SKF_RANGED);

		for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
		{
			LastAttackers &refAttacker = m_lastAttackers.at(i);
			pChar = refAttacker.charUID.CharFind();
			if ( !pChar )
				continue;
			if ( !pChar->Fight_IsAttackable() )
				continue;

			int iDist = GetDist(pChar);
			if ( fRangedSkill && ((iDist < g_Cfg.m_iArcheryMinDist) || (iDist > g_Cfg.m_iArcheryMaxDist)) )
				continue;
			if ( !CanSeeLOS(pChar) )
				continue;

			if ( (NPC_GetAiFlags() & NPC_AI_THREAT) && (refAttacker.threat > iThreat) )
			{
				// This char has more threat than current target, so switch to this target
				pTarg = pChar;
				iClosest = iDist;
				iThreat = refAttacker.threat;
			}
			else if ( iDist < iClosest )
			{
				// This char is closer than current target, so switch to this target
				pTarg = pChar;
				iClosest = iDist;
			}
		}

		if ( pTarg )
			return pTarg;
	}

	// Can't find another target, so check if NPC can keep attacking current target
	pTarg = m_Fight_Targ.CharFind();
	if ( pTarg )
		return pTarg;

	return NULL;
}

void CChar::NPC_Act_Fight()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Fight");
	// NPC is in a fight
	// Do some combat stuff (attack, defend, run, etc)
	if ( !m_pNPC || !Fight_IsActive() )
		return;

	// Periodically look around
	if ( !IsStatFlag(STATF_Pet) || (m_pNPC->m_Brain == NPCBRAIN_BERSERK) )
	{
		int iObservant = (130 - Stat_GetAdjusted(STAT_INT)) / 20;
		if ( !Calc_GetRandVal(2 + maximum(0, iObservant)) )
		{
			if ( NPC_LookAround() )
				return;
		}
	}

	CChar *pChar = m_Fight_Targ.CharFind();
	if ( !pChar )	// target is not valid anymore
		return;

	int iDist = GetDist(pChar);
	int iMotivation = NPC_GetAttackMotivation(pChar);
	bool fSkipHardcoded = false;

	if ( IsTrigUsed(TRIGGER_NPCACTFIGHT) )
	{
		CScriptTriggerArgs Args(iDist, iMotivation);
		TRIGRET_TYPE tr = OnTrigger(CTRIG_NPCActFight, pChar, &Args);
		if ( tr == TRIGRET_RET_TRUE )
			return;
		else if ( tr == TRIGRET_RET_FALSE )
			fSkipHardcoded = true;
		else if ( tr == TRIGRET_RET_DEFAULT )
		{
			SKILL_TYPE skill = static_cast<SKILL_TYPE>(Args.m_VarsLocal.GetKeyNum("skill"));
			if ( skill )
			{
				if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
					m_atMagery.m_Spell = static_cast<SPELL_TYPE>(Args.m_VarsLocal.GetKeyNum("spell"));

				Skill_Start(skill);
				return;
			}
		}

		iDist = static_cast<int>(Args.m_iN1);
		iMotivation = static_cast<int>(Args.m_iN2);
	}

	if ( !IsStatFlag(STATF_Pet) && (iMotivation < 0) )
	{
		// NPC is not motivated to keep fighting, so run away
		m_atFlee.m_iStepsMax = 20;
		m_atFlee.m_iStepsCurrent = 0;
		Skill_Start(NPCACT_FLEE);
		return;
	}

	// Check if NPC have any special ability
	if ( !fSkipHardcoded && (Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX)) && CanSeeLOS(pChar, LOS_NB_WINDOWS) )
	{
		int iRangeMin = 2;
		int iRangeMax = 8;

		// Breath fire
		if ( (m_pNPC->m_Brain == NPCBRAIN_DRAGON) && (iDist >= iRangeMin) && (iDist <= iRangeMax) )
		{
			Skill_Start(NPCACT_BREATH);
			return;
		}

		// Throw objects at the target
		if ( GetDefKey("THROWOBJ", true) )
		{
			CVarDefCont *pVar = GetDefKey("THROWRANGE", true);
			if ( pVar )
			{
				INT64 piArgs[2];
				size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pVar->GetValStr()), piArgs, COUNTOF(piArgs));
				switch ( iQty )
				{
					case 1:
						iRangeMax = static_cast<int>(piArgs[0]);
						break;
					case 2:
						iRangeMin = static_cast<int>(piArgs[0]);
						iRangeMax = static_cast<int>(piArgs[1]);
						break;
				}
			}
			if ( (iDist >= iRangeMin) && (iDist <= iRangeMax) )
			{
				Skill_Start(NPCACT_THROWING);
				return;
			}
		}
	}

	// Check if should do ranged combat
	if ( NPC_FightMagery(pChar) )
		return;
	if ( NPC_FightArchery(pChar) )
		return;

	// Otherwise follow the target for melee combat
	NPC_Act_Follow(false, CalcFightRange(m_uidWeapon.ItemFind()));
}

bool CChar::NPC_Act_Talk()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Talk");
	// NPCACT_TALK
	// NPCACT_TALK_FOLLOW
	// RETURN:
	//  true = keep waiting
	//  false = do something else (go idle)

	CChar *pChar = m_Act_Targ.CharFind();
	if ( !pChar )
		return false;

	int iDist = GetTopDist3D(pChar);
	if ( m_ptHome.GetDist3D(pChar->GetTopPoint()) > m_pNPC->m_Home_Dist_Wander )
		return false;
	if ( !CanSee(pChar) )
		return false;

	if ( (Skill_GetActive() == NPCACT_TALK_FOLLOW) && (iDist > 3) )
	{
		// Try to move closer
		if ( !NPC_Act_Follow(false, 3, false) )
			return false;
	}

	if ( m_atTalk.m_WaitCount <= 1 )
	{
		if ( NPC_CanSpeak() )
		{
			static LPCTSTR const sm_szGenericGoneMsg[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_GONE_1),
				g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_GONE_2)
			};

			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, sm_szGenericGoneMsg[Calc_GetRandVal(COUNTOF(sm_szGenericGoneMsg))], pChar->GetName());
			Speak(pszMsg);
		}
		return false;
	}

	--m_atTalk.m_WaitCount;
	return true;
}

void CChar::NPC_Act_GoHome()
{
	ADDTOCALLSTACK("CChar::NPC_Act_GoHome");
	// NPCACT_GO_HOME
	if ( !m_pNPC )
		return;

	// Periodically look around
	if ( !Calc_GetRandVal(2) )
	{
		if ( NPC_LookAround() )
			return;
	}

	if ( !m_ptHome.IsValidPoint() || !GetTopPoint().IsValidPoint() || (GetTopPoint().GetDist(m_ptHome) < m_pNPC->m_Home_Dist_Wander) )
	{
		Skill_Start(SKILL_NONE);
		return;
	}

	if ( g_Cfg.m_iLostNPCTeleport )
	{
		int iDist = m_ptHome.GetDist(GetTopPoint());
		if ( (iDist > g_Cfg.m_iLostNPCTeleport) && (iDist > m_pNPC->m_Home_Dist_Wander) )
		{
			if ( IsTrigUsed(TRIGGER_NPCLOSTTELEPORT) )
			{
				CScriptTriggerArgs Args(iDist);
				if ( OnTrigger(CTRIG_NPCLostTeleport, this, &Args) == TRIGRET_RET_TRUE )
				{
					Skill_Start(SKILL_NONE);
					return;
				}
			}
			Spell_Teleport(m_ptHome, true, false);
		}
	}

	m_Act_p = m_ptHome;
	if ( !NPC_WalkToPoint() )
	{
		Skill_Start(SKILL_NONE);
		return;
	}
}

void CChar::NPC_Act_Looting()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Looting");
	// NPC found something interesting to loot
	// m_Act_Targ = UID of the item/corpse to loot

	if ( !m_pNPC || (m_pNPC->m_Brain != NPCBRAIN_MONSTER) || !Can(CAN_C_USEHANDS) || IsStatFlag(STATF_Conjured|STATF_Pet) || (m_TagDefs.GetKeyNum("DEATHFLAGS") & DEATH_NOCORPSE) )
		return;
	if ( m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) )
		return;

	CItem *pItem = m_Act_Targ.ItemFind();
	if ( !pItem )
		return;

	if ( GetDist(pItem) > 2 )
	{
		// Item is too far to loot, move closer first
		NPC_WalkToPoint();
		return;
	}

	CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pItem);
	if ( pCorpse && (pCorpse->GetCount() > 0) )		// if it's a corpse, pick a random item inside it
		pItem = pCorpse->GetAt(Calc_GetRandVal(pCorpse->GetCount()));

	if ( !CanTouch(pItem) || !CanMove(pItem) || !CanCarry(pItem) )
	{
		// Can't pick this item, so make the NPC ignore it next time
		CItem *pMemory = Memory_AddObjTypes(pItem, MEMORY_SPEAK);
		if ( pMemory )
		{
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_IGNORE;
			if ( pItem->IsAttr(ATTR_DECAY) && pItem->IsTimerSet() )
				pMemory->SetTimeout(pItem->GetTimerDiff());
		}
		return;
	}

	if ( IsTrigUsed(TRIGGER_NPCSEEWANTITEM) )
	{
		CScriptTriggerArgs Args(pItem);
		if ( OnTrigger(CTRIG_NPCSeeWantItem, this, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( pCorpse )
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_LOOT_RUMMAGE), HUE_TEXT_DEF, TALKMODE_EMOTE);

	ItemBounce(pItem, false);
}

void CChar::NPC_Act_Flee()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Flee");
	// NPCACT_FLEE
	// Flee from the threat

	if ( m_atFlee.m_iStepsCurrent++ < m_atFlee.m_iStepsMax )
	{
		if ( NPC_Act_Follow(true, m_atFlee.m_iStepsMax) )
			return;
	}

	// Can't flee anymore, look for something new to do
	Skill_Start(SKILL_NONE);
}

void CChar::NPC_Act_RunTo(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_RunTo");
	// NPCACT_RUNTO
	// Run to point (m_Act_p)

	int iResult = NPC_WalkToPoint(true);
	if ( iResult == 0 )
	{
		// Destination reached, look for something new to do
		NPC_Act_Idle();
	}
	else if ( iResult == 2 )
	{
		// Can't run there, check if NPC should keep trying or give up
		if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )
		{
			if ( iDist > GetTopPoint().GetDist(m_Act_p) )
				iDist = GetTopPoint().GetDist(m_Act_p);
			else
				--iDist;

			if ( iDist )
			{
				NPC_Act_RunTo(iDist);
				return;
			}
		}
		else if ( m_Act_p.IsValidPoint() && IsPlayableCharacter() && !IsStatFlag(STATF_Freeze|STATF_Stone) )
		{
			Spell_Teleport(m_Act_p, true, false);
			return;
		}
		NPC_Act_Idle();
	}
}

void CChar::NPC_Act_GoTo(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_GoTo");
	// NPCACT_GOTO
	// Walk to point (m_Act_p)

	int iResult = NPC_WalkToPoint();
	if ( iResult == 0 )
	{
		// Destination reached, look for something new to do
		NPC_Act_Idle();
	}
	else if ( iResult == 2 )
	{
		// Can't run there, check if NPC should keep trying or give up
		if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )
		{
			if ( iDist > GetTopPoint().GetDist(m_Act_p) )
				iDist = GetTopPoint().GetDist(m_Act_p);
			else
				--iDist;

			if ( iDist )
			{
				NPC_Act_GoTo(iDist);
				return;
			}
		}
		else if ( m_Act_p.IsValidPoint() && IsPlayableCharacter() && !IsStatFlag(STATF_Freeze|STATF_Stone) )
		{
			Spell_Teleport(m_Act_p, true, false);
			return;
		}
		NPC_Act_Idle();
	}
}

bool CChar::NPC_Act_Food()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Food");
	// NPC is searching for food

	int iFoodLevel = Food_GetLevelPercent();
	if ( iFoodLevel > 40 )		// not hungry
		return false;

	m_pNPC->m_Act_Motivation = static_cast<BYTE>((50 - (iFoodLevel / 2)));
	WORD wEatAmount = 1;

	// Check if NPC have some food on backpack first
	if ( NPC_GetAiFlags() & NPC_AI_INTFOOD )
	{
		CItemContainer *pPack = GetContainer(LAYER_PACK);
		if ( pPack )
		{
			for ( CItem *pFood = pPack->GetContentHead(); pFood != NULL; pFood = pFood->GetNext() )
			{
				if ( !pFood->IsType(IT_FOOD) )
					continue;

				wEatAmount = Food_CanEat(pFood);
				if ( wEatAmount > 0 )
				{
					Use_EatQty(pFood, wEatAmount);
					return true;
				}
			}
		}
	}

	CItem *pCrop = NULL;
	CItem *pClosestFood = NULL;
	int iClosestFood = INT_MAX;
	int iSearchDist = (GetSight() * (100 - iFoodLevel)) / 100;

	// Search for food nearby
	CWorldSearch AreaItems(GetTopPoint(), iSearchDist);
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;
		if ( abs(pItem->GetTopZ() - GetTopZ()) > 5 )
			continue;
		if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) )
			continue;

		if ( (pItem->IsType(IT_CROPS) || pItem->IsType(IT_FOLIAGE)) && pItem->Item_GetDef()->m_ttNormal.m_tData3 )
		{
			// Eat this if can't find anything better
			pCrop = pItem;
			continue;
		}

		wEatAmount = Food_CanEat(pItem);
		if ( wEatAmount > 0 )
		{
			int iDist = GetDist(pItem);
			if ( iDist < iClosestFood )
			{
				pClosestFood = pItem;
				iClosestFood = iDist;
			}
		}
	}

	// Check if can eat the food found
	if ( pClosestFood )
	{
		if ( iClosestFood < 2 )
		{
			// Food is closer, just eat it
			EatAnim(pClosestFood->GetName(), static_cast<int>(pClosestFood->ConsumeAmount(wEatAmount)));
			if ( !pClosestFood->GetAmount() )
				pClosestFood->Plant_CropReset();	// set growth timer if this is a plant
			return true;
		}
		else if ( NPC_GetAiFlags() & NPC_AI_INTFOOD )
		{
			// Food is too far, must walk toward it first
			switch ( Skill_GetActive() )
			{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_FLEE:
				case NPCACT_GO_HOME:
				{
					CPointMap pt = pClosestFood->GetTopPoint();
					if ( CanMoveWalkTo(pt) )
					{
						m_Act_p = pt;
						Skill_Start(NPCACT_GOTO);
						return true;
					}
					break;
				}
			}
		}
	}

	// Check if can eat crops
	if ( pCrop && (GetDist(pCrop) < 3) )
	{
		if ( Use_Item(pCrop) )
			return true;
	}

	// Check if can eat grass
	if ( GetNPCBrain() == NPCBRAIN_ANIMAL )
	{
		RESOURCE_ID_BASE rid = RESOURCE_ID(RES_TYPEDEF, IT_GRASS);
		if ( Char_GetDef()->m_FoodType.ContainsResourceID(rid) )
		{
			CItem *pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && (pResBit->GetTopPoint().m_z == GetTopPoint().m_z) )
			{
				EatAnim("grass", static_cast<int>(pResBit->ConsumeAmount(10)) / 10);
				pResBit->SetTimeout(10 * 60 * TICK_PER_SEC);
				Skill_Start(NPCACT_FOOD);
				SetTimeout(5 * TICK_PER_SEC);
				return true;
			}
		}
	}
	return false;
}

void CChar::NPC_Act_Idle()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Idle");
	// NPC is idle and should try to take some action
	m_pNPC->m_Act_Motivation = 0;

	// Search for food
	if ( NPC_GetAiFlags() & (NPC_AI_FOOD|NPC_AI_INTFOOD) )
	{
		if ( NPC_Act_Food() )
			return;
	}

	// Look for items/chars around
	if ( NPC_LookAround() )
		return;

	// Make guards outside guarded region walk back to guarded region
	if ( (m_pNPC->m_Brain == NPCBRAIN_GUARD) && !m_pArea->IsGuarded() && m_ptHome.IsValidPoint() )
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	// Some creatures can do special actions
	if ( (Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX)) && !Calc_GetRandVal(3) )
	{
		if ( IsTrigUsed(TRIGGER_NPCSPECIALACTION) )
		{
			if ( OnTrigger(CTRIG_NPCSpecialAction, this) == TRIGRET_RET_TRUE )
				return;
		}
		UpdateStatVal(STAT_DEX, -(5 + Calc_GetRandVal(5)));
	}

	// Periodically fly/land
	if ( Can(CAN_C_FLY) && !Calc_GetRandVal(15) )
	{
		ToggleFlying();
		return;
	}

	// Periodically go home
	if ( m_ptHome.IsValidPoint() && !Calc_GetRandVal(15) )
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	// Periodically use Hiding skill
	if ( !IsStatFlag(STATF_Hidden) && (Skill_GetBase(SKILL_HIDING) > 30) && !Calc_GetRandVal(15 - Skill_GetBase(SKILL_HIDING) / 100) && !m_pArea->IsGuarded() )
	{
		Skill_Start(SKILL_HIDING);
		return;
	}

	// Start wandering around
	if ( Calc_GetRandVal(100 - Stat_GetAdjusted(STAT_DEX)) < 25 )
	{
		Skill_Start(NPCACT_WANDER);
		return;
	}

	// Just stand here for a bit
	Skill_Start(SKILL_NONE);
	SetTimeout(static_cast<INT64>(Calc_GetRandVal2(1, 20)) * TICK_PER_SEC);
}

bool CChar::NPC_OnReceiveItem(CChar *pCharSrc, CItem *pItem)
{
	ADDTOCALLSTACK("CChar::NPC_OnReceiveItem");
	// NPC is receiving an item from player
	// RETURN:
	//  true = accept item
	//  false = refuse item
	if ( !m_pNPC || !pCharSrc )
		return false;

	CScriptTriggerArgs Args(pItem);
	if ( IsTrigUsed(TRIGGER_RECEIVEITEM) )
	{
		if ( OnTrigger(CTRIG_ReceiveItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( NPC_IsOwnedBy(pCharSrc, false) )
	{
		if ( NPC_IsVendor() )
		{
			if ( pItem->IsType(IT_GOLD) )
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_MONEY));
				NPC_OnHirePayMore(pItem);
			}
			else
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_SELL));
				GetContainerCreate(LAYER_VENDOR_STOCK)->ContentAdd(pItem);
			}
			return true;
		}

		if ( Food_CanEat(pItem) )
		{
			if ( Use_Eat(pItem, pItem->GetAmount()) )
			{
				if ( !pItem->IsDeleted() )		// if the NPC don't eat the full stack, bounce back the remaining stack on player backpack
					pCharSrc->ItemBounce(pItem);

				if ( NPC_CanSpeak() )
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_TY));
				return true;
			}
			else
			{
				if ( NPC_CanSpeak() )
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_NO));
				return false;
			}
		}

		if ( pCharSrc->IsPriv(PRIV_GM) )
			return ItemBounce(pItem);

		if ( !CanCarry(pItem) )
		{
			if ( NPC_CanSpeak() )
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_WEAK));
			return false;
		}

		// Place item on backpack
		CItemContainer *pPack = GetContainer(LAYER_PACK);
		if ( !pPack )
			return false;
		pPack->ContentAdd(pItem);
		return true;
	}

	if ( pItem->IsType(IT_GOLD) )
	{
		CItemMemory *pMemory = Memory_FindObj(pCharSrc);
		if ( pMemory )
		{
			switch ( pMemory->m_itEqMemory.m_Action )
			{
				case NPC_MEM_ACT_SPEAK_TRAIN:
					return NPC_OnTrainPay(pCharSrc, pMemory, pItem);
				case NPC_MEM_ACT_SPEAK_HIRE:
					return NPC_OnHirePay(pCharSrc, pMemory, pItem);
			}
		}

		if ( m_pNPC->m_Brain == NPCBRAIN_BANKER )
		{
			CItemContainer *pBank = pCharSrc->GetContainerCreate(LAYER_BANKBOX);
			if ( !pBank )
				return false;

			if ( NPC_CanSpeak() )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_BANKER_DEPOSIT), pItem->GetAmount());
				Speak(pszMsg);
			}

			if ( pCharSrc->m_pClient )
				pCharSrc->m_pClient->addSound(pItem->GetDropSound(pBank));

			if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
			{
				pCharSrc->m_virtualGold += pItem->GetAmount();
				pItem->Delete();
			}
			else
				pBank->ContentAdd(pItem);
			return true;
		}
	}

	// Dropping item on vendor means quick sell
	if ( NPC_IsVendor() && !IsStatFlag(STATF_Pet) )
	{
		if ( pCharSrc->m_pClient )
		{
			VendorItem item;
			item.m_serial = pItem->GetUID();
			item.m_amount = pItem->GetAmount();
			pCharSrc->m_pClient->Event_VendorSell(this, &item, 1);
		}
		return false;
	}

	if ( !NPC_WantThisItem(pItem) )
	{
		if ( IsTrigUsed(TRIGGER_NPCREFUSEITEM) )
		{
			if ( OnTrigger(CTRIG_NPCRefuseItem, pCharSrc, &Args) != TRIGRET_RET_TRUE )
			{
				pCharSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_DONTWANT));
				return false;
			}
		}
		else
			return false;
	}

	if ( IsTrigUsed(TRIGGER_NPCACCEPTITEM) )
	{
		if ( OnTrigger(CTRIG_NPCAcceptItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	// Place item on backpack
	CItemContainer *pPack = GetContainer(LAYER_PACK);
	if ( !pPack )
		return false;
	pPack->ContentAdd(pItem);
	return true;
}

void CChar::NPC_OnTickAction()
{
	ADDTOCALLSTACK("CChar::NPC_OnTickAction");
	// NPC action timer has expired, check what action it should do next
	if ( !m_pNPC || !m_pArea )
		return;

	EXC_TRY("NPC_OnTickAction");
	SKILL_TYPE skill = Skill_GetActive();
	if ( g_Cfg.IsSkillFlag(skill, SKF_SCRIPTED) )
	{
		// Skip hardcoded actions on scripted skills
	}
	else if ( g_Cfg.IsSkillFlag(skill, SKF_FIGHT) )
	{
		EXC_SET("fighting");
		NPC_Act_Fight();
	}
	else if ( g_Cfg.IsSkillFlag(skill, SKF_MAGIC) )
	{
		EXC_SET("fighting-magic");
		NPC_Act_Fight();
	}
	else
	{
		switch ( skill )
		{
			case SKILL_NONE:
			{
				EXC_SET("idle");
				NPC_Act_Idle();
				break;
			}
			case SKILL_HIDING:
			case SKILL_STEALTH:
			{
				EXC_SET("look around");
				if ( NPC_LookAround() )
					break;
				if ( Calc_GetRandVal(Skill_GetBase(SKILL_HIDING) / 10) )	// stay hidden for some time
					break;
				EXC_SET("idle");
				NPC_Act_Idle();
				break;
			}
			case NPCACT_FOLLOW_TARG:
			{
				EXC_SET("look at char");
				NPC_LookAtChar(m_Act_Targ.CharFind());
				EXC_SET("follow");
				NPC_Act_Follow();
				break;
			}
			case NPCACT_STAY:
			{
				EXC_SET("stay");
				break;
			}
			case NPCACT_GOTO:
			{
				EXC_SET("goto");
				NPC_Act_GoTo();
				break;
			}
			case NPCACT_WANDER:
			{
				EXC_SET("wander");
				NPC_Act_Wander();
				break;
			}
			case NPCACT_LOOKING:
			{
				EXC_SET("look around");
				if ( NPC_LookAround(true) )
					break;
				EXC_SET("idle");
				NPC_Act_Idle();
				break;
			}
			case NPCACT_FLEE:
			{
				EXC_SET("flee");
				NPC_Act_Flee();
				break;
			}
			case NPCACT_TALK:
			case NPCACT_TALK_FOLLOW:
			{
				EXC_SET("talk");
				if ( !NPC_Act_Talk() )
				{
					EXC_SET("idle");
					NPC_Act_Idle();
				}
				break;
			}
			case NPCACT_GUARD_TARG:
			{
				EXC_SET("guard");
				NPC_Act_Guard();
				break;
			}
			case NPCACT_GO_HOME:
			{
				EXC_SET("go home");
				NPC_Act_GoHome();
				break;
			}
			case NPCACT_FOOD:
			{
				EXC_SET("food");
				if ( NPC_GetAiFlags() & NPC_AI_INTFOOD )
				{
					if ( !NPC_Act_Food() )
						Skill_Start(SKILL_NONE);
				}
				break;
			}
			case NPCACT_RUNTO:
			{
				EXC_SET("runto");
				NPC_Act_RunTo();
				break;
			}
			default:
			{
				if ( !IsSkillBase(skill) )		// this should never happen
					Skill_Start(SKILL_NONE);
				break;
			}
		}
	}

	EXC_SET("timer expired");
	if ( IsTimerExpired() && IsStatFlag(STATF_War) && !(IsSetCombatFlags(COMBAT_PREHIT) && (m_atFight.m_Swing_State == WAR_SWING_SWINGING)) )	// was not reset? PREHIT forces timer to be 0, so it get's defaulted here breaking NPC's speed when PREHIT is enabled. Must not check in this case.
	{
		// Set timer for next tick
		INT64 iTimeout = maximum(0, (150 - Stat_GetAdjusted(STAT_DEX)) / 2);
		SetTimeout(TICK_PER_SEC + Calc_GetRandLLVal2(iTimeout / 2, iTimeout));
	}

	// Periodically restock vendors
	if ( NPC_IsVendor() )
		NPC_Vendor_Restock();

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' [0%lx]\n", GetName(), static_cast<DWORD>(GetUID()));
	EXC_DEBUG_END;
}

void CChar::NPC_Pathfinding()
{
	ADDTOCALLSTACK("CChar::NPC_Pathfinding");
	// NPC is trying to find an path to walk
	if ( !m_pNPC )
		return;

	CPointMap pt = GetTopPoint();

	// Check if pathfind is really needed
	EXC_TRY("Pathfinding");
	EXC_SET("pre-checking");
	int iInt = (NPC_GetAiFlags() & NPC_AI_ALWAYSINT) ? 300 : Stat_GetAdjusted(STAT_INT);
	if ( iInt < 75 )	// too dumb
		return;
	if ( m_pNPC->m_nextPt == m_Act_p )	// path to this point is already saved
		return;
	if ( !m_Act_p.IsValidPoint() )		// invalid point
		return;
	if ( m_Act_p.m_map != pt.m_map )	// can't walk between different maps
		return;
	if ( (m_Act_p.m_x == pt.m_x) && (m_Act_p.m_y == pt.m_y) )	// same spot
		return;
	int iDist = pt.GetDist(m_Act_p);
	if ( iDist < 2 )					// too close
		return;
	if ( iDist >= MAX_NPC_PATH_STORAGE_SIZE / 2 )		// too far
		return;
	if ( (pt.m_x <= MAX_NPC_PATH_STORAGE_SIZE / 2) || (pt.m_y <= MAX_NPC_PATH_STORAGE_SIZE / 2) || (pt.m_x >= g_MapList.GetX(pt.m_map) - (MAX_NPC_PATH_STORAGE_SIZE / 2)) || (pt.m_y >= g_MapList.GetY(pt.m_map) - (MAX_NPC_PATH_STORAGE_SIZE / 2)) )		// pathfind is buggy near map edges, so don't use it there
		return;
	if ( (Calc_GetRandVal(300) > iInt) && m_pNPC->m_nextX[0] )		// need 300 int at least to pathfind, but always search if this is a first step
		return;

	// Clear saved steps list
	EXC_SET("clearing last steps");
#ifdef _WIN32
	memset(m_pNPC->m_nextX, 0, sizeof(m_pNPC->m_nextX));
	memset(m_pNPC->m_nextY, 0, sizeof(m_pNPC->m_nextY));
#else
	for ( int i = 0; i < MAX_NPC_PATH_STORAGE_SIZE; ++i )
	{
		m_pNPC->m_nextX[i] = 0;
		m_pNPC->m_nextY[i] = 0;
	}
#endif

	// Proceed with the pathfinding
	EXC_SET("filling the map");
	CPathFinder path(this, m_Act_p);

	EXC_SET("searching the path");
	if ( path.FindPath() == PATH_NONEXISTENT )
		return;

	EXC_SET("saving found path");
	CPointMap ptNext;
	for ( size_t i = 1; (i != path.LastPathSize()) && (i < MAX_NPC_PATH_STORAGE_SIZE); ++i )	// start on i = 1 because 0 is the current position
	{
		ptNext = path.ReadStep(i);
		m_pNPC->m_nextX[i - 1] = ptNext.m_x;
		m_pNPC->m_nextY[i - 1] = ptNext.m_y;
	}
	m_pNPC->m_nextPt = m_Act_p;
	path.ClearLastPath();	// use explicitly when using one CPathFinder object for more NPCs
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' point '%d,%d,%d,%d' [0%lx]\n", GetName(), pt.m_x, pt.m_y, pt.m_z, pt.m_map, static_cast<DWORD>(GetUID()));
	EXC_DEBUG_END;
}

void CChar::NPC_ExtraAI()
{
	ADDTOCALLSTACK("CChar::NPC_ExtraAI");
	EXC_TRY("ExtraAI");
	if ( !m_pNPC || (GetNPCBrain() != NPCBRAIN_HUMAN) )
		return;

	EXC_SET("init");
	if ( IsTrigUsed(TRIGGER_NPCACTION) )
	{
		if ( OnTrigger(CTRIG_NPCAction, this) == TRIGRET_RET_TRUE )
			return;
	}

	// Equip weapons if possible
	EXC_SET("weapon/shield");
	if ( IsStatFlag(STATF_War) )
	{
		CItem *pWeapon = LayerFind(LAYER_HAND1);
		if ( !pWeapon || !pWeapon->IsTypeWeapon() )
			ItemEquipWeapon(false);

		CItem *pShield = LayerFind(LAYER_HAND2);
		if ( !pShield || !pShield->IsTypeArmor() )
		{
			CItemContainer *pPack = GetContainer(LAYER_PACK);
			if ( pPack )
			{
				pShield = pPack->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_SHIELD));
				if ( pShield )
					ItemEquip(pShield);
			}
		}
		return;
	}

	// Equip lightsource at night time
	EXC_SET("light source");
	CItem *pLightSource = LayerFind(LAYER_HAND2);
	CSector *pSector = GetTopPoint().GetSector();
	if ( pSector && pSector->IsDark() )
	{
		if ( !pLightSource || !pLightSource->IsType(IT_LIGHT_LIT) )
		{
			pLightSource = ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_LIGHT_OUT));
			if ( pLightSource )
			{
				ItemEquip(pLightSource);
				Use_Obj(pLightSource, false);
			}
		}
	}
	else
	{
		if ( pLightSource && (pLightSource->IsType(IT_LIGHT_LIT) || pLightSource->IsType(IT_LIGHT_OUT)) )
			ItemBounce(pLightSource, false);
	}

	EXC_CATCH;
}
