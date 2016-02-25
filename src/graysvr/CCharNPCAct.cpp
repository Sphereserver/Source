// Actions specific to an NPC.
#include "graysvr.h"	// predef header.
#include "../graysvr/CPathFinder.h"
#include "../network/receive.h"

//////////////////////////
// CChar

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

LPCTSTR const CCharNPC::sm_szVerbKeys[NV_QTY+1] =
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

bool CChar::NPC_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CChar::NPC_OnVerb");
	// Stuff that only NPC's do.
	if ( !m_pNPC )
		return false;

	CChar * pCharSrc = pSrc->GetChar();

	switch ( FindTableSorted( s.GetKey(), CCharNPC::sm_szVerbKeys, COUNTOF(CCharNPC::sm_szVerbKeys)-1 ))
	{
	case NV_BUY:
	{
		// Open up the buy dialog.
		if ( pCharSrc == NULL || !pCharSrc->IsClient())
			return false;

		CClient * pClientSrc = pCharSrc->GetClient();
		ASSERT(pClientSrc != NULL);
		if ( !pClientSrc->addShopMenuBuy(this) )
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NO_GOODS));
		else
			pClientSrc->m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
		break;
	}
	case NV_BYE:
		Skill_Start( SKILL_NONE );
		m_Act_Targ.InitUID();
		break;
	case NV_LEAVE:
	case NV_FLEE:
		// Short amount of fleeing.
		m_atFlee.m_iStepsMax = s.GetArgVal();	// how long should it take to get there.
		if ( ! m_atFlee.m_iStepsMax )
			m_atFlee.m_iStepsMax = 20;

		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
		break;
	case NV_GOTO:
		m_Act_p = g_Cfg.GetRegionPoint( s.GetArgStr());
		Skill_Start( NPCACT_GOTO );
		break;
	case NV_HIRE:
		return NPC_OnHireHear( pCharSrc);
	case NV_PETRETRIEVE:
		return( NPC_StablePetRetrieve( pCharSrc ));
	case NV_PETSTABLE:
		return( NPC_StablePetSelect( pCharSrc ));
	case NV_RESTOCK:	// individual restock command.
		return NPC_Vendor_Restock(true, s.GetArgVal() != 0);
	case NV_RUN:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( true );
		break;
	case NV_RUNTO:
		m_Act_p = g_Cfg.GetRegionPoint( s.GetArgStr());
		Skill_Start( NPCACT_RUNTO );
		break;
	case NV_SELL:
	{
		// Open up the sell dialog.
		if ( pCharSrc == NULL || !pCharSrc->IsClient() )
			return false;

		CClient * pClientSrc = pCharSrc->GetClient();
		ASSERT(pClientSrc != NULL);
		if ( ! pClientSrc->addShopMenuSell( this ))
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NOTHING_BUY));
		else
			pClientSrc->m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
		break;
	}
	case NV_SHRINK:
		{
			// we must own it.
			if ( ! NPC_IsOwnedBy( pCharSrc ))
				return( false );
			CItem * pItem = NPC_Shrink(); // this delete's the char !!!
			if ( pItem )
				pCharSrc->m_Act_Targ = pItem->GetUID();
			if (s.GetArgStr())
				pCharSrc->ItemBounce(pItem);
		
			return( pItem != NULL );
		}
	case NV_TRAIN:
		return( NPC_OnTrainHear( pCharSrc, s.GetArgStr()));
	case NV_WALK:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( false );
		break;
	default:
		// Eat all the CClient::sm_szVerbKeys and CCharPlayer::sm_szVerbKeys verbs ?
		//if ( FindTableSorted(s.GetKey(), CClient::sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1) < 0 )
		return false;
	}
	return true;
}

const LAYER_TYPE CChar::sm_VendorLayers[] = // static
{
	LAYER_VENDOR_STOCK, LAYER_VENDOR_EXTRA, LAYER_VENDOR_BUYS,
};

bool CChar::NPC_Vendor_Restock(bool bForce, bool bFillStock)
{
	ADDTOCALLSTACK("CChar::NPC_Vendor_Restock");
	// Restock this NPC char.
	// Then Set the next restock time for this .

	if ( m_pNPC == NULL )
		return false;

	// Make sure that we're a vendor and not a pet
	if ( IsStatFlag(STATF_Pet) || !NPC_IsVendor() )
		return false;

	bool bRestockNow = true;

	if ( !bForce && m_pNPC->m_timeRestock.IsTimeValid() )
	{
		// Restock occurs every 10 minutes of inactivity (unless
		// region tag specifies different time)
		CRegionWorld *region = GetRegion();
		INT64 restockIn = 10 * 60 * TICK_PER_SEC;
		if( region != NULL )
		{
			CVarDefCont *vardef = region->m_TagDefs.GetKey("RestockVendors");
			if( vardef != NULL )
				restockIn = vardef->GetValNum();
			if ( region->m_TagDefs.GetKey("NoRestock") != NULL )
				bRestockNow = false;
		}
		if ( m_TagDefs.GetKey("NoRestock") != NULL )
			bRestockNow = false;
		
		if (bRestockNow)
			bRestockNow = ( CServTime::GetCurrentTime().GetTimeDiff(m_pNPC->m_timeRestock) > restockIn );
	}

	// At restock the containers are actually emptied
	if ( bRestockNow )
	{
		m_pNPC->m_timeRestock.Init();

		for ( size_t i = 0; i < COUNTOF(sm_VendorLayers); ++i )
		{
			CItemContainer *pCont = GetBank(sm_VendorLayers[i]);
			if ( !pCont )
				return false;

			pCont->Empty();
		}
	}

	if ( bFillStock )
	{
		// An invalid restock time means that the containers are
		// waiting to be filled
		if ( !m_pNPC->m_timeRestock.IsTimeValid() )
		{
			if ( IsTrigUsed(TRIGGER_NPCRESTOCK) )
			{
				CCharBase *pCharDef = Char_GetDef();
				ReadScriptTrig(pCharDef, CTRIG_NPCRestock, true);
			}

			//	we need restock vendor money as well
			GetBank()->Restock();
		}

		// remember that the stock was filled (or considered up-to-date)
		m_pNPC->m_timeRestock.SetCurrentTime();
	}
	return true;
}

bool CChar::NPC_StablePetSelect( CChar * pCharPlayer )
{
	ADDTOCALLSTACK("CChar::NPC_StablePetSelect");
	// I am a stable master.
	// I will stable a pet for the player.

	if ( pCharPlayer == NULL )
		return( false );
	if ( ! pCharPlayer->IsClient())
		return( false );

	// Might have too many pets already ?
	int iCount = 0;
	CItemContainer * pBank = GetBank();
	if ( pBank->GetCount() >= MAX_ITEMS_CONT )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_FULL ) );
		return( false );
	}

	// Calculate the max limit of pets that the NPC can hold for the player
	double iSkillTaming = pCharPlayer->Skill_GetAdjusted(SKILL_TAMING);
	double iSkillAnimalLore = pCharPlayer->Skill_GetAdjusted(SKILL_ANIMALLORE);
	double iSkillVeterinary = pCharPlayer->Skill_GetAdjusted(SKILL_VETERINARY);
	double iSkillSum = iSkillTaming + iSkillAnimalLore + iSkillVeterinary;

	int iPetMax;
	if ( iSkillSum >= 240.0 )
		iPetMax = 5;
	else if ( iSkillSum >= 200.0 )
		iPetMax = 4;
	else if ( iSkillSum >= 160.0 )
		iPetMax = 3;
	else
		iPetMax = 2;

	if ( iSkillTaming >= 100.0 )
		iPetMax += (int)((iSkillTaming - 90.0) / 10);

	if ( iSkillAnimalLore >= 100.0 )
		iPetMax += (int)((iSkillAnimalLore - 90.0) / 10);

	if ( iSkillVeterinary >= 100.0 )
		iPetMax += (int)((iSkillVeterinary - 90.0) / 10);

	if ( m_TagDefs.GetKey("MAXPLAYERPETS") )
		iPetMax = static_cast<int>(m_TagDefs.GetKeyNum("MAXPLAYERPETS"));

	for ( CItem *pItem = pBank->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsType(IT_FIGURINE) && pItem->m_uidLink == pCharPlayer->GetUID() )
			iCount++;
	}
	if ( iCount >= iPetMax )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TOOMANY ) );
		return( false );
	}

	pCharPlayer->m_pClient->m_Targ_PrvUID = GetUID();
	pCharPlayer->m_pClient->addTarget( CLIMODE_TARG_PET_STABLE, g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG ) );
	return( true );
}

bool CChar::NPC_StablePetRetrieve( CChar * pCharPlayer )
{
	ADDTOCALLSTACK("CChar::NPC_StablePetRetrieve");
	// Get pets for this person from my inventory.
	// May want to put up a menu ???

	if ( !m_pNPC || m_pNPC->m_Brain != NPCBRAIN_STABLE )
		return false;

	int iCount = 0;
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetBank()->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_FIGURINE) && pItem->m_uidLink == pCharPlayer->GetUID() )
		{
			if ( !pCharPlayer->Use_Figurine(pItem) )
			{
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_CLAIM_FOLLOWER), pItem->GetName());
				Speak(pszTemp);
				return true;
			}

			pItem->Delete();
			iCount++;
		}
	}

	Speak(g_Cfg.GetDefaultMsg((iCount > 0) ? DEFMSG_NPC_STABLEMASTER_CLAIM : DEFMSG_NPC_STABLEMASTER_CLAIM_NOPETS));
	return true;
}

void CChar::NPC_ActStart_SpeakTo( CChar * pSrc )
{
	ADDTOCALLSTACK("CChar::NPC_ActStart_SpeakTo");
	// My new action is that i am speaking to this person.
	// Or just update the amount of time i will wait for this person.
	m_Act_Targ = pSrc->GetUID();
	m_atTalk.m_WaitCount = 20;
	m_atTalk.m_HearUnknown = 0;

	Skill_Start( ( pSrc->Stat_GetAdjusted(STAT_FAME) > 7000 ) ? NPCACT_TALK_FOLLOW : NPCACT_TALK );
	SetTimeout(3*TICK_PER_SEC);
	UpdateDir(pSrc);
}

void CChar::NPC_OnHear( LPCTSTR pszCmd, CChar * pSrc, bool fAllPets )
{
	ADDTOCALLSTACK("CChar::NPC_OnHear");
	// This CChar has heard you say something.
	if ( !m_pNPC || !pSrc )
		return;

	// Pets always have a basic set of actions.
	if ( NPC_OnHearPetCmd(pszCmd, pSrc, fAllPets) || !NPC_CanSpeak() )
		return;

	// What where we doing ?
	// too busy to talk ?

	switch ( Skill_GetActive())
	{
		case SKILL_BEGGING: // busy begging. (hack)
			if ( !g_Cfg.IsSkillFlag( SKILL_BEGGING, SKF_SCRIPTED ) )
				return;
			break;
		case NPCACT_TALK:
		case NPCACT_TALK_FOLLOW:
			// Was NPC talking to someone else ?
			if ( m_Act_Targ != pSrc->GetUID())
			{
				if ( NPC_Act_Talk() )
				{
					CChar * pCharOld = m_Act_Targ.CharFind();
					if (pCharOld != NULL)
					{
						TCHAR * z = Str_GetTemp();
						sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_INTERRUPT), pCharOld->GetName(), pSrc->GetName());
						Speak(z);
					}
				}
			}
			break;
		default:
			break;
	}

	// I've heard them for the first time.
	CItemMemory * pMemory = Memory_FindObjTypes( pSrc, MEMORY_SPEAK );
	if ( pMemory == NULL )
	{
		// This or CTRIG_SeeNewPlayer will be our first contact with people.
		if ( IsTrigUsed(TRIGGER_NPCHEARGREETING) )
		{
			if ( OnTrigger( CTRIG_NPCHearGreeting, pSrc ) == TRIGRET_RET_TRUE )
				return;
		}

		// record that we attempted to speak to them.
		pMemory = Memory_AddObjTypes(pSrc, MEMORY_SPEAK);
		if ( pMemory )
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
	}

	// Do the scripts want me to take some action based on this speech.
	SKILL_TYPE skill = m_Act_SkillCurrent;

	TALKMODE_TYPE mode = TALKMODE_SAY;
	for ( size_t i = 0; i < m_pNPC->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = m_pNPC->m_Speech[i];
		if ( !pLink )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) || !pLink->HasTrigger(XTRIG_UNKNOWN) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef != NULL);
	for ( size_t i = 0; i < pCharDef->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = pCharDef->m_Speech[i];
		if ( !pLink )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	// hard code some default reactions.
	if ( m_pNPC->m_Brain == NPCBRAIN_HEALER	|| Skill_GetBase( SKILL_SPIRITSPEAK ) >= 1000 )
	{
		if ( NPC_LookAtChar( pSrc, 1 ))
			return;
	}

	// can't figure you out.
	if ( IsTrigUsed(TRIGGER_NPCHEARUNKNOWN) )
	{
		if ( OnTrigger( CTRIG_NPCHearUnknown, pSrc ) == TRIGRET_RET_TRUE )
			return;
	}

	if ( Skill_GetActive() == NPCACT_TALK || Skill_GetActive() == NPCACT_TALK_FOLLOW )
	{
		++ m_atTalk.m_HearUnknown;
		int iMaxUnk = 4;
		if ( GetDist( pSrc ) > 4 )
			iMaxUnk = 1;
		if ( m_atTalk.m_HearUnknown > iMaxUnk )
		{
			Skill_Start( SKILL_NONE ); // say good by
		}
	}
}

int CChar::NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill )
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainCheck");
	// Can we train in this skill ?
	// RETURN: Amount of skill we can train.
	//

	if ( !IsSkillBase(Skill) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_DUNNO_1));
		return 0;
	}

	int iSkillSrcVal = pCharSrc->Skill_GetBase(Skill);
	int iSkillVal = Skill_GetBase(Skill);
	int iTrainVal = NPC_GetTrainMax(pCharSrc, Skill) - iSkillSrcVal;

	// Train npc skill cap
	int iMaxDecrease = 0;
	if ( (pCharSrc->GetSkillTotal() + iTrainVal) > pCharSrc->Skill_GetMax(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) )
	{	
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
				continue;

			if ( pCharSrc->Skill_GetLock(static_cast<SKILL_TYPE>(i)) == SKILLLOCK_DOWN )
				iMaxDecrease += pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
		}
		iMaxDecrease = minimum(iTrainVal, iMaxDecrease);
	}
	else
	{
		iMaxDecrease = iTrainVal;
	}

	LPCTSTR pszMsg;
	if ( iSkillVal <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_2 );
	}
	else if ( iSkillSrcVal > iSkillVal )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_3 );
	}
	else if ( iMaxDecrease <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_4 );
	}
	else
	{
		return( iMaxDecrease );
	}

	char	*z = Str_GetTemp();
	sprintf(z, pszMsg, g_Cfg.GetSkillKey(Skill));
	Speak(z);
	return 0;
}

bool CChar::NPC_OnTrainPay(CChar *pCharSrc, CItemMemory *pMemory, CItem * pGold)
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainPay");
	SKILL_TYPE skill = static_cast<SKILL_TYPE>(pMemory->m_itEqMemory.m_Skill);
	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_FORGOT));
		return false;
	}

	int iTrainCost = NPC_OnTrainCheck(pCharSrc, skill) * g_Cfg.m_iTrainSkillCost;
	if (( iTrainCost <= 0 ) || !pGold )
		return false;

	Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_SUCCESS ) );

	// Consume as much money as we can train for.
	if ( pGold->GetAmount() < iTrainCost )
	{
		iTrainCost = pGold->GetAmount();
	}
	else if ( pGold->GetAmount() == iTrainCost )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_1 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;
	}
	else
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_2 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;

		// Give change back.
		pGold->UnStackSplit( iTrainCost, pCharSrc );
	}
	GetPackSafe()->ContentAdd( pGold );	// take my cash.

	// Give credit for training.
	NPC_TrainSkill( pCharSrc, skill, iTrainCost );
	return( true );
}

bool CChar::NPC_TrainSkill( CChar * pCharSrc, SKILL_TYPE skill, int toTrain )
{
	ADDTOCALLSTACK("CChar::NPC_TrainSkill");
	int iTrain = toTrain;
	if ( (pCharSrc->GetSkillTotal() + toTrain) > pCharSrc->Skill_GetMax(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) )
	{	
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
				continue;

			if ( toTrain < 1 )
			{
				pCharSrc->Skill_SetBase(skill, iTrain + pCharSrc->Skill_GetBase(skill));
				break;
			}

			if ( pCharSrc->Skill_GetLock(static_cast<SKILL_TYPE>(i)) == SKILLLOCK_DOWN )
			{
				if ( pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i)) > toTrain )
				{
					pCharSrc->Skill_SetBase(static_cast<SKILL_TYPE>(i), pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i)) - toTrain);
					toTrain = 0;
				}
				else
				{
					toTrain -= pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
					pCharSrc->Skill_SetBase(static_cast<SKILL_TYPE>(i), 0);
				}
			}
		}
	}
	else
	{
		pCharSrc->Skill_SetBase(skill, iTrain + pCharSrc->Skill_GetBase(skill));
	}

	return true;
}


bool CChar::NPC_OnTrainHear( CChar * pCharSrc, LPCTSTR pszCmd )
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainHear");
	// We are asking for training ?

	if ( ! m_pNPC )
		return( false );

	// Check the NPC is capable of teaching
	if ( (m_pNPC->m_Brain < NPCBRAIN_HUMAN) || (m_pNPC->m_Brain > NPCBRAIN_STABLE) || (m_pNPC->m_Brain == NPCBRAIN_GUARD) )
		return( false );

	// Check the NPC isn't busy fighting
	if ( Memory_FindObjTypes( pCharSrc, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_ENEMY ) );
		return true;
	}

	// Did they mention a skill name i recognize ?
	TemporaryString pszMsg;

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
			continue;

		LPCTSTR pSkillKey = g_Cfg.GetSkillKey(static_cast<SKILL_TYPE>(i));
		if ( FindStrWord( pszCmd, pSkillKey ) <= 0)
			continue;

		// Can we train in this ?
		int iTrainCost = NPC_OnTrainCheck(pCharSrc, static_cast<SKILL_TYPE>(i)) * g_Cfg.m_iTrainSkillCost;
		if ( iTrainCost <= 0 )
			return true;

		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE), iTrainCost, static_cast<LPCTSTR>(pSkillKey));
		Speak(pszMsg);
		CItemMemory * pMemory = Memory_AddObjTypes( pCharSrc, MEMORY_SPEAK );
		if ( pMemory )
		{
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_TRAIN;
			pMemory->m_itEqMemory.m_Skill = static_cast<WORD>(i);
		}
		return true;
	}

	// What can he teach me about ?
	// Just tell them what we can teach them or set up a memory to train.
	strcpy( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_1 ) );

	LPCTSTR pPrvSkill = NULL;

	size_t iCount = 0;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
			continue;

		int iDiff = NPC_GetTrainMax(pCharSrc, static_cast<SKILL_TYPE>(i)) - pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
		if ( iDiff <= 0 )
			continue;

		if ( iCount > 6 )
		{
			pPrvSkill = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_2 );
			break;
		}
		if ( iCount > 1 )
		{
			strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_3 ) );
		}
		if ( pPrvSkill )
		{
			strcat( pszMsg, pPrvSkill );
		}

		pPrvSkill = g_Cfg.GetSkillKey(static_cast<SKILL_TYPE>(i));
		iCount++;
	}

	if ( iCount == 0 )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_3 ) );
		return true;
	}
	if ( iCount > 1 )
	{
		strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_4 ) );
	}

	strcat( pszMsg, pPrvSkill );
	strcat( pszMsg, "." );
	Speak( pszMsg );
	return( true );
}

void CChar::NPC_OnNoticeSnoop( CChar * pCharThief, CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::NPC_OnNoticeSnoop");
	if ( !m_pNPC )
		return;

	// start making them angry at you.
	static UINT const sm_szTextSnoop[] =
	{
		DEFMSG_NPC_GENERIC_SNOOPED_1,
		DEFMSG_NPC_GENERIC_SNOOPED_2,
		DEFMSG_NPC_GENERIC_SNOOPED_3,
		DEFMSG_NPC_GENERIC_SNOOPED_4
	};

	if ( pCharMark != this )	// not me so who cares.
		return;

	if ( NPC_CanSpeak())
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szTextSnoop[ Calc_GetRandVal( COUNTOF( sm_szTextSnoop )) ]));
	}
	if ( ! Calc_GetRandVal(4))
	{
		m_Act_Targ = pCharThief->GetUID();
		m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
	}
}

int CChar::NPC_WalkToPoint( bool fRun )
{
	ADDTOCALLSTACK("CChar::NPC_WalkToPoint");
	// Move toward my target .
	//
	// RETURN:
	//  0 = we are here.
	//  1 = took the step.
	//  2 = can't take this step right now. (obstacle)
	if (Can(CAN_C_NONMOVER))
		return 0;

	int			iDex = Stat_GetAdjusted(STAT_DEX);
	int			iInt = Stat_GetAdjusted(STAT_INT);
	CPointMap	pMe = GetTopPoint();
	CPointMap	pTarg = m_Act_p;
	DIR_TYPE	Dir = pMe.GetDir(pTarg);
	bool		bUsePathfinding = false;
	CCharBase	*pCharDef = Char_GetDef();		

	EXC_TRY("NPC_WalkToPoint");
	if ( Dir >= DIR_QTY ) return 0;		// we are already in the spot
	if ( iDex <= 0 ) return 2;			// we cannot move now

	EXC_SET("NPC_AI_PATH");
	//	Use pathfinding
	if ( NPC_GetAiFlags() & NPC_AI_PATH )
	{
		NPC_Pathfinding();

		//	walk the saved path
		CPointMap local;
		local.m_x = m_pNPC->m_nextX[0];
		local.m_y = m_pNPC->m_nextY[0];
		local.m_map = pMe.m_map;
			// no steps available yet, or pathfinding not usable in this situation
			// so, use default movements

		if (local.m_x > 0 && local.m_y > 0)
		{

			bUsePathfinding = true;

			if ( pMe.GetDist(local) != 1 )
			{
				// The next step is too far away, pathfinding route has become invalid
				m_pNPC->m_nextPt.InitPoint();
				m_pNPC->m_nextX[0] = 0;
				m_pNPC->m_nextY[0] = 0;
			}
			else
			{
				// Update our heading to the way we need to go
				Dir = pMe.GetDir(local);
				ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);

				EXC_TRYSUB("Array Shift");
				//	also shift the steps array
				for ( int j = 0; j < (MAX_NPC_PATH_STORAGE_SIZE - 1); ++j )
				{
					m_pNPC->m_nextX[j] = m_pNPC->m_nextX[j+1];
					m_pNPC->m_nextY[j] = m_pNPC->m_nextY[j+1];
				}
				m_pNPC->m_nextX[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				m_pNPC->m_nextY[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				EXC_CATCHSUB("NPCAI");
			}
		}
	}

	EXC_SET("Non-Advanced pathfinding");
	pMe.Move( Dir );
	if ( ! CanMoveWalkTo(pMe, true, false, Dir ) )
	{
		CPointMap	ptFirstTry = pMe;

		// try to step around it ?
		int iDiff = 0;
		int iRand = Calc_GetRandVal( 100 );
		if ( iRand < 30 )	// do nothing.
		{
			// whilst pathfinding we should keep trying to find new ways to our destination
			if ( bUsePathfinding == true )
			{
				SetTimeout( TICK_PER_SEC ); // wait a moment before finding a new route
				return( 1 );
			}
			return( 2 );
		}

		if ( iRand < 35 ) iDiff = 4;	// 5
		else if ( iRand < 40 ) iDiff = 3;	// 10
		else if ( iRand < 65 ) iDiff = 2;
		else iDiff = 1;
		if ( iRand & 1 ) iDiff = -iDiff;
		pMe = GetTopPoint();
		Dir = GetDirTurn( Dir, iDiff );
		pMe.Move( Dir );
		if ( ! CanMoveWalkTo(pMe, true, false, Dir ))
		{
			bool	bClearedWay = false;
			// Some object in my way that i could move ? Try to move it.
			if ( !pCharDef->Can(CAN_C_USEHANDS) || IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) ) ;		// i cannot use hands or i am frozen, so cannot move objects
			else if (( NPC_GetAiFlags()&NPC_AI_MOVEOBSTACLES ) && ( iInt > iRand ))
			{
				int			i;
				CPointMap	point;
				for ( i = 0; i < 2; i++ )
				{
					if ( !i ) point = pMe;
					else point = ptFirstTry;

					//	Scan point for items that could be moved by me and move them to my position
					CWorldSearch	AreaItems(point);
					for (;;)
					{
						CItem	*pItem = AreaItems.GetItem();
						if ( !pItem ) break;
						else if ( abs(pItem->GetTopZ() - pMe.m_z) > 3 ) continue;		// item is too high
						else if ( !pItem->Item_GetDef()->Can(CAN_I_BLOCK) ) continue;	// this item not blocking me
						else if ( !CanMove(pItem) || !CanCarry(pItem) ) bClearedWay = false;
						else
						{
							//	move this item to the position I am currently in
							pItem->MoveToUpdate(GetTopPoint());
							bClearedWay = true;
							break;
						}
					}

					if ( bClearedWay ) break;
					//	If not cleared the way still, but I am still clever enough
					//	I should try to move in the first step I was trying to move to
					else if ( iInt < iRand*3 ) break;
				}

				//	we have just cleared our way
				if ( bClearedWay )
				{
					if ( point == ptFirstTry )
					{
						Dir = GetTopPoint().GetDir(m_Act_p);
						ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);
						if (Dir >= DIR_QTY)
							bClearedWay = false;
					}
				}
			}
			if ( !bClearedWay )
			{
				// whilst pathfinding we should keep trying to find new ways to our destination
				if ( bUsePathfinding == true )
				{
					SetTimeout( TICK_PER_SEC ); // wait a moment before finding a new route
					return 1;
				}
				return 2;
			}
		}
	}

	EXC_SET("Finishing Move Action a");
	//Finish Move Action

	// ??? Make sure we are not facing a wall.
	ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);
	m_dirFace = Dir;	// Face this direction.
	if ( fRun && ( ! pCharDef->Can(CAN_C_RUN|CAN_C_FLY) || Stat_GetVal(STAT_DEX) <= 1 ))
		fRun = false;

	EXC_SET("StatFlag");
	StatFlag_Mod(STATF_Fly, fRun);

	EXC_SET("Old Top Point");
	CPointMap ptOld = GetTopPoint();

	EXC_SET("Reveal");
	CheckRevealOnMove();

	EXC_SET("MoveToChar");
	MoveToChar(pMe);

	EXC_SET("Check Location");
	if ( CheckLocation(false) == TRIGRET_RET_FALSE )	// check if I stepped on any item/teleport
	{
		SetTopPoint(ptOld);		// we already moved, so move back to previous location
		return 2;
	}

	EXC_SET("Move Update");
	UpdateMove(ptOld);

	EXC_SET("Speed counting");
	// How fast can they move.
	INT64 iTickNext;

	// TAG.OVERRIDE.MOVERATE
	INT64 tTick;
	CVarDefCont * pValue = GetKey("OVERRIDE.MOVERATE", true);
	if (pValue)
		tTick = pValue->GetValNum();	//Taking value from tag.override.moverate
	else
		tTick = pCharDef->m_iMoveRate;	//no tag.override.moverate, we get default moverate (created from ini's one).
	// END TAG.OVERRIDE.MOVERATE
	if (fRun)
	{
		if (IsStatFlag(STATF_Pet))	// pets run a little faster.
		{
			if (iDex < 75)
				iDex = 75;
		}
		iTickNext = TICK_PER_SEC / 4 + Calc_GetRandLLVal((100 - (iDex*tTick) / 100) / 5) * TICK_PER_SEC / 10;
	}
	else
		iTickNext = TICK_PER_SEC + Calc_GetRandLLVal((100 - (iDex*tTick) / 100) / 3) * TICK_PER_SEC / 10;

	if (iTickNext < 1)
		iTickNext = 1;
	else if (iTickNext > 50)
		iTickNext = 50;

	SetTimeout(iTickNext);
	EXC_CATCH;
	return 1;
}

bool CChar::NPC_LookAtCharGuard( CChar * pChar, bool bFromTrigger )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharGuard");
	// Does the guard hate the target ?

	//	do not waste time on invul+dead, non-criminal and jailed chars
	if ( !pChar || pChar->IsStatFlag(STATF_INVUL|STATF_DEAD) || pChar->IsPriv(PRIV_JAILED) || !bFromTrigger )	//|| !pChar->Noto_IsCriminal()
		return false;

	static UINT const sm_szSpeakGuardJeer[] =
	{
		DEFMSG_NPC_GUARD_THREAT_1,
		DEFMSG_NPC_GUARD_THREAT_2,
		DEFMSG_NPC_GUARD_THREAT_3,
		DEFMSG_NPC_GUARD_THREAT_4,
		DEFMSG_NPC_GUARD_THREAT_5
	};

	if ( ! pChar->m_pArea->IsGuarded())
	{
		// At least jeer at the criminal.
		if ( Calc_GetRandVal(10))
			return( false );

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(sm_szSpeakGuardJeer[ Calc_GetRandVal( COUNTOF( sm_szSpeakGuardJeer )) ]), pChar->GetName());
		Speak(pszMsg);
		UpdateDir(pChar);
		return false;
	}

	static UINT const sm_szSpeakGuardStrike[] =
	{
		DEFMSG_NPC_GUARD_STRIKE_1,
		DEFMSG_NPC_GUARD_STRIKE_2,
		DEFMSG_NPC_GUARD_STRIKE_3,
		DEFMSG_NPC_GUARD_STRIKE_4,
		DEFMSG_NPC_GUARD_STRIKE_5
	};

	if ( GetTopDist3D(pChar) > 1 )
	{
		if ( g_Cfg.m_fGuardsInstantKill || pChar->Skill_GetBase(SKILL_MAGERY) )
			Spell_Teleport(pChar->GetTopPoint(), false, false);

		// If we got intant kill guards enabled, allow the guards to swing immidiately
		if ( g_Cfg.m_fGuardsInstantKill )
		{
			pChar->Stat_SetVal(STAT_STR, 1);
			Fight_Hit(pChar);
		}
	}
	if ( !IsStatFlag(STATF_War) || m_Act_Targ != pChar->GetUID() )
	{
		Speak(g_Cfg.GetDefaultMsg(sm_szSpeakGuardStrike[Calc_GetRandVal(COUNTOF(sm_szSpeakGuardStrike))]));
		Fight_Attack(pChar);
		Attacker_SetThreat(Attacker_GetID(pChar),10000);
	}
	return true;
}

bool CChar::NPC_LookAtCharMonster( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharMonster");
	// return:
	//   true = take new action.
	//   false = continue with any previous action.
	//  motivation level =
	//  0 = not at all.
	//  100 = definitly.
	//

	if ( !m_pNPC )
		return false;
	int iFoodLevel = Food_GetLevelPercent();

	// Attacks those not of my kind.
	if ( ! Noto_IsCriminal() && iFoodLevel > 40 )		// I am not evil ?
	{
		return NPC_LookAtCharHuman( pChar );
	}

	// Attack if i am stronger.
	// or i'm just stupid.
	int iActMotivation = NPC_GetAttackMotivation( pChar );
	if ( iActMotivation <= 0 )
		return( false );
	if ( Fight_IsActive() && m_Act_Targ == pChar->GetUID())	// same targ.
		return( false );
	if ( iActMotivation < m_pNPC->m_Act_Motivation )
		return( false );

	int iDist = GetTopDist3D( pChar );
	if ( IsStatFlag( STATF_Hidden ) &&
		! NPC_FightMayCast() &&
		iDist > 1 )
		return false;	// element of suprise.

	if ( Fight_Attack( pChar ) == false )
		return false;
	m_pNPC->m_Act_Motivation = static_cast<unsigned char>(iActMotivation);
	return true;
}

bool CChar::NPC_LookAtCharHuman( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHuman");
	if ( !m_pNPC || pChar->IsStatFlag(STATF_DEAD) )
		return false;

	if ( Noto_IsEvil())		// I am evil.
	{
		// Attack others if we are evil.
		return( NPC_LookAtCharMonster( pChar ));
	}

	if (( ! pChar->Noto_IsEvil() &&  g_Cfg.m_fGuardsOnMurderers) && (! pChar->IsStatFlag( STATF_Criminal ))) 	// not interesting.
		return( false );

	// Yell for guard if we see someone evil.
	if ( NPC_CanSpeak() &&
		m_pArea->IsGuarded() &&
		! Calc_GetRandVal( 3 ))
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
			return( NPC_LookAtCharGuard( pChar ));

		Speak( pChar->IsStatFlag( STATF_Criminal) ?
			 g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_SEECRIM ) :
			g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_SEEMONS ) );

		// Find a guard.
		CallGuards( pChar );
		if ( IsStatFlag( STATF_War ))
			return( false );

		// run away like a coward.
		m_Act_Targ = pChar->GetUID();
		m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
		m_pNPC->m_Act_Motivation = 80;
		return true;
	}

	// Attack an evil creature ?

	return( false );
}

bool CChar::NPC_LookAtCharHealer( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHealer");
	if ( !pChar->IsStatFlag(STATF_DEAD) || (pChar->m_pNPC && pChar->m_pNPC->m_bonded) )
		return false;

	static LPCTSTR const sm_szHealerRefuseEvils[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_3 )
	};
	static LPCTSTR const sm_szHealerRefuseCriminals[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_3 )
	};
	static LPCTSTR const sm_szHealerRefuseGoods[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_3 )
	};
	static LPCTSTR const sm_szHealer[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_5 )
	};

	UpdateDir( pChar );

	LPCTSTR pszRefuseMsg;

	int iDist = GetDist( pChar );
	if ( pChar->IsStatFlag( STATF_Insubstantial ))
	{
		pszRefuseMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_MANIFEST );
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if ( iDist > 3 )
	{
		if ( Calc_GetRandVal(5))
			return false;
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RANGE ) );
		return( true );
	}

	// What noto is this char to me ?
	bool ImEvil = Noto_IsEvil();
	bool ImNeutral = Noto_IsNeutral();
	NOTO_TYPE NotoThem = pChar->Noto_GetFlag( this, true );

	if ( !IsStatFlag( STATF_Criminal ) && NotoThem == NOTO_CRIMINAL )
	{
		pszRefuseMsg = sm_szHealerRefuseCriminals[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseCriminals )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if (( !ImNeutral && !ImEvil) && NotoThem >= NOTO_NEUTRAL )
	{
		pszRefuseMsg = sm_szHealerRefuseEvils[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseEvils )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if (( ImNeutral || ImEvil ) && NotoThem == NOTO_GOOD )
	{
		pszRefuseMsg = sm_szHealerRefuseGoods[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseGoods )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	// Attempt to res.
	Speak( sm_szHealer[ Calc_GetRandVal( COUNTOF( sm_szHealer )) ] );
	UpdateAnimate( ANIM_CAST_AREA );
	if ( ! pChar->OnSpellEffect( SPELL_Resurrection, this, 1000, NULL ))
	{
		if ( Calc_GetRandVal(2))
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_1 ) );
		else
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_2 ) );
	}

	return true;
}

// I might want to go pickup this item ?
bool CChar::NPC_LookAtItem( CItem * pItem, int iDist )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtItem");

	// Checking if I can use hands (Can flags can be now changed in a single character, they are inheriting CCharBase::m_can automatically on CChar's constructor, no need to check CCharBase anymore).
	if ( !Can(CAN_C_USEHANDS) || !CanSee(pItem) )
		return false;

	int iWantThisItem = NPC_WantThisItem(pItem);
	if ( IsTrigUsed(TRIGGER_NPCLOOKATITEM) )
	{
		if ( IsTrigUsed(TRIGGER_NPCLOOKATITEM) && !pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN|ATTR_SECURE) )
		{

			CScriptTriggerArgs	Args(iDist, iWantThisItem, pItem);
			switch ( OnTrigger(CTRIG_NPCLookAtItem, this, &Args) )
			{
				case  TRIGRET_RET_TRUE:		return true;
				case  TRIGRET_RET_FALSE:	return false;
				default:					break;
			}
			iWantThisItem = static_cast<int>(Args.m_iN2);
		}
	}

	// Loot nearby items on ground
	if ( iWantThisItem > Calc_GetRandVal(100) )
	{
		m_Act_Targ = pItem->GetUID();
		NPC_Act_Looting();
		return true;
	}

	// Loot nearby corpses
	if ( pItem->IsType(IT_CORPSE) && (NPC_GetAiFlags() & NPC_AI_LOOTING) && (Memory_FindObj(pItem) == NULL) )
	{
		m_Act_Targ = pItem->GetUID();
		NPC_Act_Looting();
		return true;
	}

	// Check for doors we can open
	if ( pItem->IsType(IT_DOOR) && GetDist(pItem) <= 1 && CanTouch(pItem) && !Calc_GetRandVal(2) )
	{
		if ( pItem->IsDoorOpen() )	// door is already open
			return false;

		UpdateDir(pItem);
		if ( !Use_Item(pItem) )	// try to open it
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



bool CChar::NPC_LookAtChar( CChar * pChar, int iDist )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtChar");
	// I see a char.
	// Do I want to do something to this char (more that what i'm already doing ?)
	// RETURN:
	//   true = yes i do want to take a new action.
	//

	if ( !m_pNPC || !pChar || ( pChar == this ) || !CanSeeLOS(pChar,LOS_NB_WINDOWS) )//Flag - we can attack through a window
		return false;

	if ( IsTrigUsed(TRIGGER_NPCLOOKATCHAR) )
	{
		switch ( OnTrigger(CTRIG_NPCLookAtChar, pChar) )
		{
			case  TRIGRET_RET_TRUE:		return true;
			case  TRIGRET_RET_FALSE:	return false;
			default:					break;
		}
	}

	if ( NPC_IsOwnedBy( pChar, false ))
	{
		// follow my owner again. (Default action)
		m_Act_Targ = pChar->GetUID();
		m_pNPC->m_Act_Motivation = 50;
		Skill_Start(Skill_GetActive() == NPCACT_FOLLOW_TARG ? NPCACT_FOLLOW_TARG : NPCACT_GUARD_TARG);
		return true;
	}

	else
	{
		// initiate a conversation ?
		if ( ! IsStatFlag( STATF_War ) &&
			( Skill_GetActive() == SKILL_NONE || Skill_GetActive() == NPCACT_WANDER ) && // I'm idle
			pChar->m_pPlayer &&
			! Memory_FindObjTypes( pChar, MEMORY_SPEAK ))
		{
			if ( IsTrigUsed(TRIGGER_NPCSEENEWPLAYER) )
			{
				if ( OnTrigger( CTRIG_NPCSeeNewPlayer, pChar ) != TRIGRET_RET_TRUE )
				{
					// record that we attempted to speak to them.
					CItemMemory * pMemory = Memory_AddObjTypes( pChar, MEMORY_SPEAK );
					if ( pMemory )
						pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
					// m_Act_Hear_Unknown = 0;
				}
			}
		}
	}

	switch ( m_pNPC->m_Brain )	// my type of brain
	{
		case NPCBRAIN_GUARD:
			// Guards should look around for criminals or nasty creatures.
			if ( NPC_LookAtCharGuard( pChar ))
				return true;
			break;

		case NPCBRAIN_MONSTER:
		case NPCBRAIN_DRAGON:
			if ( NPC_LookAtCharMonster( pChar ))
				return( true );
			break;

		case NPCBRAIN_BERSERK:
			// Blades or EV.
			// ??? Attack everyone you touch !
			if ( iDist <= CalcFightRange( m_uidWeapon.ItemFind() ) )
			{
				Fight_Hit( pChar );
			}
			if ( Fight_IsActive()) // Is this a better target than my last ?
			{
				CChar * pCharTarg = m_Act_Targ.CharFind();
				if ( pCharTarg != NULL )
				{
					if ( iDist >= GetTopDist3D( pCharTarg ))
						break;
				}
			}
			if ( Fight_Attack( pChar ) )
				return true;
			break;

		case NPCBRAIN_HEALER:
			// Healers should look around for ghosts.
			if ( NPC_LookAtCharHealer( pChar ))
				return( true );
			if ( NPC_LookAtCharHuman( pChar ))
				return( true );
			break;

		case NPCBRAIN_BANKER:
		case NPCBRAIN_VENDOR:
		case NPCBRAIN_STABLE:
		case NPCBRAIN_ANIMAL:
		case NPCBRAIN_HUMAN:
			if ( NPC_LookAtCharHuman(pChar) )
				return true;
			break;

		default:
			break;
	}

	return( false );
}

bool CChar::NPC_LookAround( bool fForceCheckItems )
{
	ADDTOCALLSTACK("CChar::NPC_LookAround");
	// Take a look around for other people/chars.
	// We may be doing something already. So check current action motivation level.
	// RETURN:
	//   true = found something better to do.

	if ( NPC_GetAiFlags()&NPC_AI_INTFOOD )
	{
		bool fFood = NPC_Act_Food();
		//DEBUG_ERR(("2 fFood %d\n",fFood));
		if ( fFood ) //are we hungry?
			return true;
	}

	CSector *pSector = GetTopSector();
	if ( !m_pNPC || !pSector )
		return false;

	if ( m_pNPC->m_Brain == NPCBRAIN_BERSERK || ! Calc_GetRandVal(6))
	{
		// Make some random noise
		SoundChar( Calc_GetRandVal(2) ? CRESND_RAND1 : CRESND_RAND2 );
	}

	int iRange = GetSight();
	int iRangeBlur = UO_MAP_VIEW_SIGHT;

	CCharBase * pCharDef = Char_GetDef();

	// If I can't move don't look to far.
	if ( !pCharDef->Can(CAN_C_WALK|CAN_C_FLY|CAN_C_SWIM) || IsStatFlag(STATF_Stone|STATF_Freeze) )
	{
		if ( !NPC_FightMayCast() )	// And i have no distance attack.
			iRange = iRangeBlur = 2;
	}
	else
	{
		// I'm mobile. do basic check if i would move here first.
		if ( !NPC_CheckWalkHere(GetTopPoint(), m_pArea, 0) )
		{
			// I should move. Someone lit a fire under me.
			m_Act_p = GetTopPoint();
			m_Act_p.Move(static_cast<DIR_TYPE>(Calc_GetRandVal( DIR_QTY )));
			NPC_WalkToPoint( true );
			return( true );
		}

		if ( Stat_GetAdjusted(STAT_INT) < 50 )
			iRangeBlur /= 2;
	}

	// Lower the number of chars we look at.
	if ( pSector->GetCharComplexity() > (g_Cfg.m_iMaxCharComplexity / 2) ) // if sector is to complex, lower our range to keep some performance.
		iRange /= 4;

	// Any interesting chars here ?
	CWorldSearch AreaChars(GetTopPoint(), iRange);
	for (;;)
	{
		CChar	*pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )	// just myself.
			continue;

		int iDist = GetTopDist3D(pChar);
		if ( iDist > iRangeBlur && !pChar->IsStatFlag(STATF_Fly) )
		{
			if ( Calc_GetRandVal(iDist) )
				continue;	// can't see them.
		}
		if ( NPC_LookAtChar(pChar, iDist) )
			return true;
	}

	// Check the ground for good stuff.
	if ( !fForceCheckItems &&
		Stat_GetAdjusted(STAT_INT) > 10 &&
		! IsSkillBase( Skill_GetActive()) &&
		! Calc_GetRandVal( 3 ))
	{
		fForceCheckItems = true;
	}

	if ( fForceCheckItems )
	{
		CWorldSearch AreaItems( GetTopPoint(), iRange );
		for (;;)
		{
			CItem	*pItem = AreaItems.GetItem();
			if ( !pItem )
				break;

			int iDist = GetTopDist3D(pItem);
			if ( iDist > iRangeBlur )
			{
				if ( Calc_GetRandVal(iDist) )
					continue;	// can't see them.
			}
			if ( NPC_LookAtItem(pItem, iDist) )
				return true;
		}
	}

	// Move stuff that is in our way ? (chests etc.)
	return false;
}

void CChar::NPC_Act_Wander()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Wander");
	// NPCACT_WANDER
	// just wander aimlessly. (but within bounds)
	// Stop wandering and re-eval frequently

	if ( Can(CAN_C_NONMOVER) )
		return;
	if ( ! Calc_GetRandVal( 7 + ( Stat_GetVal(STAT_DEX) / 30 )))
	{
		// Stop wandering ?
		Skill_Start( SKILL_NONE );
		return;
	}

	/*if ( Calc_GetRandVal( 2 ) )
	{
		if ( NPC_LookAround() )
			return;
	}*/

	// Staggering Walk around.
	m_Act_p = GetTopPoint();
	m_Act_p.Move( GetDirTurn( m_dirFace, 1 - Calc_GetRandVal(3)));

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
	// Protect our target or owner. (m_Act_Targ)
	if ( m_pNPC == NULL )
		return;

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar != NULL && pChar != this && CanSeeLOS(pChar, LOS_NB_WINDOWS) )
	{
		if ( pChar->Fight_IsActive() )	// protect the target if they're in a fight
		{
			if ( Fight_Attack( pChar->m_Fight_Targ.CharFind() ))
				return;
		}
	}

	// Target is out of range or doesn't need protecting, so just follow for now
	//NPC_LookAtChar(pChar, 1);
	NPC_Act_Follow();
}

bool CChar::NPC_Act_Follow( bool fFlee, int maxDistance, bool forceDistance )
{
	ADDTOCALLSTACK("CChar::NPC_Act_Follow");
	// Follow our target or owner. (m_Act_Targ) we may be fighting.
	// false = can't follow any more. give up.
	if ( Can(CAN_C_NONMOVER) )
		return false;

	EXC_TRY("NPC_Act_Follow")
	CChar * pChar = Fight_IsActive() ? m_Fight_Targ.CharFind() : m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		// free to do as i wish !
		Skill_Start( SKILL_NONE );
		return( false );
	}

	EXC_SET("Trigger");
	if ( IsTrigUsed(TRIGGER_NPCACTFOLLOW) )
	{
		CScriptTriggerArgs Args( fFlee, maxDistance, forceDistance );
		switch ( OnTrigger( CTRIG_NPCActFollow, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return false;
			case TRIGRET_RET_FALSE:	return true;
			default:				break;
		}

		fFlee			= (Args.m_iN1 != 0);
		maxDistance		= static_cast<int>(Args.m_iN2);
		forceDistance	= (Args.m_iN3 != 0);
	}

	EXC_SET("CanSee");
	// Have to be able to see target to follow.
	if ( CanSee( pChar ))
	{
		m_Act_p = pChar->GetTopPoint();
	}
	else
	{
		// Monster may get confused because he can't see you.
		// There is a chance they could forget about you if hidden for a while.
		if ( fFlee || !Calc_GetRandVal( 1 + (( 100 - Stat_GetAdjusted(STAT_INT)) / 20 )))
			return( false );
	}

	EXC_SET("Distance checks");
	int dist = GetTopPoint().GetDist( m_Act_p );
	if ( dist > UO_MAP_VIEW_RADAR )		// too far away ?
		return( false );

	if ( forceDistance )
	{
		if ( dist < maxDistance )
			fFlee = true;	// start moving away
	}
	else
	{
		if ( fFlee )
		{
			if ( dist >= maxDistance )
				return( false );
		}
		else if ( dist <= maxDistance )
			return( true );
	}

	EXC_SET("Fleeing");
	if ( fFlee )
	{
		CPointMap ptOld = m_Act_p;
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirTurn( m_Act_p.GetDir( ptOld ), 4 + 1 - Calc_GetRandVal(3)));
		NPC_WalkToPoint( dist > 3 );
		m_Act_p = ptOld;	// last known point of the enemy.
		return( true );
	}

	EXC_SET("WalkToPoint 1");
	NPC_WalkToPoint( IsStatFlag( STATF_War ) ? true : ( dist > 3 ));
	return( true );

	EXC_CATCH;
	return( false );
}

bool CChar::NPC_FightArchery( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_FightArchery");
	if ( !g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_RANGED) )
		return( false );

	int iMinDist = 0;
	int iMaxDist = 0;

	// determine how far we can shoot with this bow
	CItem *pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon != NULL )
	{
		iMinDist = pWeapon->RangeH();
		iMaxDist = pWeapon->RangeL();
	}

	// if range is not set on the weapon, default to ini settings
	if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
		iMaxDist = g_Cfg.m_iArcheryMaxDist;
	if ( !iMinDist )
		iMinDist = g_Cfg.m_iArcheryMinDist;

	int iDist = GetTopDist3D( pChar );
	if ( iDist > iMaxDist )	// way too far away . close in.
		return( false );

	if ( iDist > iMinDist )
		return( true );		// always use archery if distant enough

	if ( !Calc_GetRandVal( 2 ) )	// move away
	{
		// Move away
		NPC_Act_Follow( false, iMaxDist, true );
		return( true );
	}

	// Fine here.
	return( true );
}

// Retrieves all the spells this character has to spells[x] list
int CCharNPC::Spells_GetCount()
{
	ADDTOCALLSTACK("CCharNPC::Spells_GetAll");
	if (m_spells.empty())
		return -1;
	return m_spells.size();

	// This code was meant to check if found spells does really exist
	int total = 0;
	for (unsigned int count = 0; count < m_spells.size() ; count++)
	{
		Spells refSpell = m_spells.at(count);
		if (!refSpell.id)
			continue;
		total++;
	}
	return total;
}

// Retrieve the spell stored at index = n
SPELL_TYPE CCharNPC::Spells_GetAt(unsigned char id)
{
	ADDTOCALLSTACK("CCharNPC::Spells_GetAt");
	if (m_spells.empty())
		return SPELL_NONE;
	if (id >= m_spells.size())
		return SPELL_NONE;
	Spells refSpell = m_spells.at(id);
	if (refSpell.id)
		return refSpell.id;
	return SPELL_NONE;
}

// Delete the spell at the given index
bool CCharNPC::Spells_DelAt(unsigned char id)
{
	ADDTOCALLSTACK("CCharNPC::Spells_DelAt");
	if (m_spells.empty())
		return false;
	Spells refSpell = m_spells.at(id);
	if (refSpell.id)
	{
		std::vector<Spells>::iterator it = m_spells.begin() + id;
		m_spells.erase(it);
		return true;
	}
	return false;
}

// Add this spell to this NPC's list
bool CCharNPC::Spells_Add(SPELL_TYPE spell)
{
	ADDTOCALLSTACK("CCharNPC::Spells_Add");
	if (Spells_FindSpell(spell) >= 0)
		return false;
	CSpellDef * pSpell = g_Cfg.GetSpellDef(spell);
	if (!pSpell)
		return false;
	Spells refSpell;
	refSpell.id = spell;
	m_spells.push_back(refSpell);
	return true;
}

// Retrieves the indexed id for the given spell, if found.
int CCharNPC::Spells_FindSpell(SPELL_TYPE spellID)
{
	ADDTOCALLSTACK("CCharNPC::Spells_FindSpell");
	if (m_spells.empty())
		return -1;

	unsigned int count = 0;
	while (count < m_spells.size())
	{
		Spells spell = m_spells.at(count);
		if (spell.id == spellID)
			return count;
		count++;
	}
	return -1;
}

bool CChar::NPC_GetAllSpellbookSpells()	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	//	search for suitable book in hands first
	for ( CItem *pBook = GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
	{
		if (pBook->IsTypeSpellbook())
		{
			if (!NPC_AddSpellsFromBook(pBook))
				continue;
		}
	}

	//	then search in the top level of the pack
	CItemContainer *pPack = GetPack();
	if (pPack)
	{
		for ( CItem *pBook = pPack->GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
		{
			if (pBook->IsTypeSpellbook())
			{
				if (!NPC_AddSpellsFromBook(pBook))
					continue;
			}
		}
	}
	return true;
}

bool CChar::NPC_AddSpellsFromBook(CItem * pBook)
{
	if (!m_pNPC)
		return false;
	SKILL_TYPE skill = pBook->GetSpellBookSkill();
	int max = Spell_GetMax(skill);
	for (int i = 0; i <= max; i++)
	{
		SPELL_TYPE spell = static_cast<SPELL_TYPE>(i);
		if (pBook->IsSpellInBook(spell))
			m_pNPC->Spells_Add(spell);
	}
	return true;
}

// cast a spell if i can ?
// or i can throw or use archery ?
// RETURN:
//  false = revert to melee type fighting.
bool CChar::NPC_FightMagery(CChar * pChar)
{
	ADDTOCALLSTACK("CChar::NPC_FightMagery");
	if (!NPC_FightMayCast(false))	// not checking skill here since it will do a search later and it's an expensive function.
		return(false);

	int count = m_pNPC->Spells_GetCount();
	CItem * pWand = LayerFind(LAYER_HAND1);		//Try to get a working wand.
	CObjBase * pTarg = pChar;
	if (pWand)
	{
		if (pWand->GetType() != IT_WAND || pWand->m_itWeapon.m_spellcharges <= 0)// If the item is really a wand and have it charges it's a valid wand, if not ... we get rid of it.
			pWand = NULL;
	}
	if (count < 1 && !pWand)
		return false;

	int iDist = GetTopDist3D(pChar);
	if (iDist >((UO_MAP_VIEW_SIGHT * 3) / 4))	// way too far away . close in.
		return(false);

	if (iDist <= 1 &&
		Skill_GetBase(SKILL_TACTICS) > 200 &&
		!Calc_GetRandVal(2))
	{
		// Within striking distance.
		// Stand and fight for a bit.
		return(false);
	}
	int skill = SKILL_NONE;
	int iStatInt = Stat_GetBase(STAT_INT);
	int mana = Stat_GetVal(STAT_INT);
	int iChance = ((mana >= (iStatInt / 2)) ? mana : (iStatInt - mana));

	CObjBase * pSrc = this;
	if (Calc_GetRandVal(iChance) < iStatInt / 4)
	{
		// we failed this test, but we could be casting next time
		// back off from the target a bit
		if (mana > (iStatInt / 3) && Calc_GetRandVal(iStatInt))
		{
			if (iDist < 4 || iDist > 8)	// Here is fine?
			{
				NPC_Act_Follow(false, Calc_GetRandVal(3) + 2, true);
			}
			return(true);
		}
		return(false);
	}
	unsigned char i = 0;
	if (pWand)
		i = static_cast<unsigned char>(Calc_GetRandVal2(0, count));	//chance between all spells + wand
	else
		i = static_cast<unsigned char>(Calc_GetRandVal2(0, count-1));

	if (i > count)	// if i > count then we use wand to cast.
	{
		SPELL_TYPE spell = static_cast<SPELL_TYPE>(pWand->m_itWeapon.m_spell);
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		if (!pSpellDef)	// wand check failed ... we go on melee, next cast try might select another type of spell :)
			return false;
		pSrc = pWand;
		if (NPC_FightCast(pTarg, pWand, spell))
			goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
	}

	for (; i < count; i++)
	{
		SPELL_TYPE spell = m_pNPC->Spells_GetAt(i);
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		if (!pSpellDef)	//If it reached here it should exist, checking anyway.
			continue;

		int iSkillReq = 0;
		if (!pSpellDef->GetPrimarySkill(&skill, &iSkillReq))
			skill = SKILL_MAGERY;

		if (Skill_GetBase(static_cast<SKILL_TYPE>(skill)) < iSkillReq)
			continue;
		if (NPC_FightCast(pTarg, this, spell, static_cast<SKILL_TYPE>(skill)))
			goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
	}
	return false;	// No castable spell found, go back on melee.

	BeginCast:	//Start casting
	// KRJ - give us some distance
	// if the opponent is using melee
	// the bigger the disadvantage we have in hitpoints, the further we will go
	if (mana > iStatInt / 3 && Calc_GetRandVal(iStatInt << 1))
	{
		if (iDist < 4 || iDist > 8)	// Here is fine?
		{
			NPC_Act_Follow(false, 5, true);
		}
	}
	else NPC_Act_Follow();

	Reveal();

	m_Act_Targ = pTarg->GetUID();
	m_Act_TargPrv = pSrc->GetUID();	// I'm using a wand ... or casting this directly?.
	m_Act_p = pTarg->GetTopPoint();

	// Calculate the difficulty
	return Skill_Start(static_cast<SKILL_TYPE>(skill)); 
}

// I'm able to use magery
// test if I can cast this spell
// Specific behaviours for each spell and spellflag
bool CChar::NPC_FightCast(CObjBase * &pTarg, CObjBase * pSrc, SPELL_TYPE &spell, SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::NPC_FightCast");
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if (skill == SKILL_NONE)
	{
		int iSkillTest = 0;
		if (!pSpellDef->GetPrimarySkill(&iSkillTest, NULL))
			iSkillTest = SKILL_MAGERY;
		skill = static_cast<SKILL_TYPE>(iSkillTest);
	}
	if (!Spell_CanCast(spell, true, pSrc, false))
		return false;
	if (pSpellDef->IsSpellType(SPELLFLAG_PLAYERONLY))
		return false;

	if (!pSpellDef->IsSpellType(SPELLFLAG_HARM))
	{
		if (pSpellDef->IsSpellType(SPELLFLAG_GOOD))
		{
			if (pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pTarg->IsChar())
			{
				//	help self or friends if needed. support 3 friends + self for castings
				bool	bSpellSuits = false;
				CChar	*pFriend[4];
				int		iFriendIndex = 0;
				CChar	*pTarget = pTarg->GetUID().CharFind();

				pFriend[0] = this;
				pFriend[1] = pFriend[2] = pFriend[3] = NULL;
				iFriendIndex = 1;

				if (NPC_GetAiFlags()&NPC_AI_COMBAT)
				{
					//	search for the neariest friend in combat
					CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIGHT);
					for (;;)
					{
						pTarget = AreaChars.GetChar();
						if (!pTarget)
							break;

						CItemMemory *pMemory = pTarget->Memory_FindObj(pTarg);
						if (pMemory && pMemory->IsMemoryTypes(MEMORY_FIGHT | MEMORY_HARMEDBY | MEMORY_IRRITATEDBY))
						{
							pFriend[iFriendIndex++] = pTarget;
							if (iFriendIndex >= 4) break;
						}
					}
				}

				//	i cannot cast this on self. ok, then friends only
				if (pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF))
				{
					pFriend[0] = pFriend[1];
					pFriend[1] = pFriend[2];
					pFriend[2] = pFriend[3];
					pFriend[3] = NULL;
				}
				for (iFriendIndex = 0; iFriendIndex < 4; iFriendIndex++)
				{
					pTarget = pFriend[iFriendIndex];
					if (!pTarget) 
						break;
					//	check if the target need that
					switch (spell)
					{
						// Healing has the top priority?
						case SPELL_Heal:
						case SPELL_Great_Heal:
							if (pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 3)
								bSpellSuits = true;
							break;
						case SPELL_Gift_of_Renewal:
							if (pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 2)
								bSpellSuits = true;
							break;
						// Then curing poison.
						case SPELL_Cure:
							if (pTarget->LayerFind(LAYER_FLAG_Poison)) bSpellSuits = true;
							break;

						// Buffs are coming now.
							
						case SPELL_Reactive_Armor:	// Deffensive ones first
							if (!pTarget->LayerFind(LAYER_SPELL_Reactive))
								bSpellSuits = true;
							break;
						case SPELL_Protection:
							if (!pTarget->LayerFind(LAYER_SPELL_Protection))
								bSpellSuits = true;
							break;
						case SPELL_Magic_Reflect:
							if (!pTarget->LayerFind(LAYER_SPELL_Magic_Reflect))
								bSpellSuits = true;
							break;

						case SPELL_Bless:		// time for the others ...
							if (!pTarget->LayerFind(LAYER_SPELL_STATS))
								bSpellSuits = true;
							break;
						default:
							break;
					}
					if (bSpellSuits)
						break;

					LAYER_TYPE layer = pSpellDef->m_idLayer;
					if (layer != LAYER_NONE)	// If the spell applies an effect.
					{
						if (!pTarget->LayerFind(layer))	// and target doesn't have this effect already...
						{
							bSpellSuits = true;	//then it may need it
							break;
						}
					}

				}
				if (bSpellSuits)
				{
					pTarg = pTarget;
					m_atMagery.m_Spell = spell;
					return true;
				}
				return false;
			}
			else if (pSpellDef->IsSpellType(SPELLFLAG_TARG_ITEM))
			{
				//	spell is good, but must be targeted at an item
				switch (spell)
				{
					case SPELL_Immolating_Weapon:
						pTarg = m_uidWeapon.ObjFind();
						if (pTarg)
							break;
					default:
						break;
				}
			}
			if (pSpellDef->IsSpellType(SPELLFLAG_HEAL)) //Good spells that cannot be targeted
			{
				bool bSpellSuits = true;
				switch (spell)
				{
					//No spells added ATM until they are created, good example spell to here = SPELL_Healing_Stone
					case SPELL_Healing_Stone:
					{
						CItem * pStone = GetBackpackItem(ITEMID_HEALING_STONE);
						if (!pStone)
							break;
						if ((pStone->m_itNormal.m_morep.m_z == 0) && (Stat_GetVal(STAT_STR) < static_cast<int>(pStone->m_itNormal.m_more2)) && (pStone->m_itNormal.m_more1 >= pStone->m_itNormal.m_more2))
						{
							Use_Obj(pStone, false);
							return true; // we are not casting any spell but suceeded at using the stone created by this one, we are done now.
						}
						return false;	// Already have a stone, no need of more
					}
					default:
						break;
				}

				if (!bSpellSuits) 
					return false;
				pTarg = this;
				m_atMagery.m_Spell = spell;
				return true;
			}
		}
		else if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
		{
			m_atMagery.m_Spell = spell;
			return true;	// if flag is present ... we leave the rest to the incoming code
		}
	}
	else
	{
		/*if (NPC_GetAiFlags()&NPC_AI_STRICTCAST)
			return false;*/
		//if ( /*!pVar &&*/ !pSpellDef->IsSpellType(SPELLFLAG_HARM))
		//	return false;

		// less chance for berserker spells
		/*if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON) && Calc_GetRandVal(2))
			return false;

		// less chance for field spells as well
		if (pSpellDef->IsSpellType(SPELLFLAG_FIELD) && Calc_GetRandVal(4))
			return false;*/
	}
	m_atMagery.m_Spell = spell;
	return true;
}

CChar * CChar::NPC_FightFindBestTarget()
{
	ADDTOCALLSTACK("CChar::NPC_FightFindBestTarget");
	// Find the best target to attack, and switch to this
	// new target even if I'm already attacking someone.
	if ( !m_pNPC )
		return NULL;

	if ( Attacker() )
	{
		if ( !m_lastAttackers.size() )
			return NULL;

		INT64 threat = 0;
		int iClosest = INT_MAX;
		CChar *pChar = NULL;
		CChar *pClosest = NULL;
		SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();

		for ( std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
		{
			LastAttackers &refAttacker = *it;
			pChar = static_cast<CChar*>(static_cast<CGrayUID>(refAttacker.charUID).CharFind());
			if ( !pChar )
				continue;
			if ( pChar->IsStatFlag(STATF_DEAD) )	// dead chars can't be selected as target
			{
				pChar = NULL;
				continue;
			}
			if ( refAttacker.ignore )
			{
				bool bIgnore = true;
				if ( IsTrigUsed(TRIGGER_HITIGNORE) )
				{
					CScriptTriggerArgs Args;
					Args.m_iN1 = bIgnore;
					OnTrigger(CTRIG_HitIgnore, pChar, &Args);
					bIgnore = Args.m_iN1 ? true : false;
				}
				if ( bIgnore )
				{
					pChar = NULL;
					continue;
				}
			}
			if ( !pClosest )
				pClosest = pChar;

			int iDist = GetDist(pChar);
			if ( iDist > UO_MAP_VIEW_SIGHT )
				continue;
			if ( g_Cfg.IsSkillFlag(skillWeapon, SKF_RANGED) && (iDist < g_Cfg.m_iArcheryMinDist || iDist > g_Cfg.m_iArcheryMaxDist) )
				continue;
			if ( !CanSeeLOS(pChar) )
				continue;

			if ( (NPC_GetAiFlags() & NPC_AI_THREAT) && (threat < refAttacker.threat) )	// this char has more threat than others, let's switch to this target
			{
				pClosest = pChar;
				iClosest = iDist;
				threat = refAttacker.threat;
			}
			else if ( iDist < iClosest )	// this char is more closer to me than my current target, let's switch to this target
			{
				pClosest = pChar;
				iClosest = iDist;
			}
		}
		return pClosest ? pClosest : pChar;
	}

	// New target not found, check if I can keep attacking my current target
	CChar *pTarget = m_Fight_Targ.CharFind();
	if ( pTarget )
		return pTarget;

	return NULL;
}

void CChar::NPC_Act_Fight()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Fight");
	// I am in an attack mode.
	if ( !m_pNPC || !Fight_IsActive() )
		return;

	// Review our targets periodically.
	if ( ! IsStatFlag(STATF_Pet) ||
		m_pNPC->m_Brain == NPCBRAIN_BERSERK )
	{
		int iObservant = ( 130 - Stat_GetAdjusted(STAT_INT)) / 20;
		if ( ! Calc_GetRandVal( 2 + maximum( 0, iObservant )))
		{
			if ( NPC_LookAround())
				return;
		}
	}

	CChar * pChar = m_Fight_Targ.CharFind();
	if (pChar == NULL || !pChar->IsTopLevel()) // target is not valid anymore ?
		return;

	if (Attacker_GetIgnore(pChar))
	{
		if (!NPC_FightFindBestTarget())
		{
			Skill_Start(SKILL_NONE);
			StatFlag_Clear(STATF_War);
			m_Fight_Targ.InitUID();
			return;
		}
	}
	int iDist = GetDist( pChar );

	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		m_atFight.m_War_Swing_State == WAR_SWING_READY &&
		! Calc_GetRandVal(3))
	{
		// If a guard is ever too far away (missed a chance to swing)
		// Teleport me closer.
		NPC_LookAtChar( pChar, iDist );
	}


	// If i'm horribly damaged and smart enough to know it.
	int iMotivation = NPC_GetAttackMotivation( pChar );

	bool		fSkipHardcoded	= false;
	if ( IsTrigUsed(TRIGGER_NPCACTFIGHT) )
	{
		CGrayUID m_oldAct = m_Act_Targ;
		CScriptTriggerArgs Args( iDist, iMotivation );
		switch ( OnTrigger( CTRIG_NPCActFight, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return;
			case TRIGRET_RET_FALSE:	fSkipHardcoded	= true;	break;
			case static_cast<TRIGRET_TYPE>(2):
			{
				SKILL_TYPE iSkillforced = static_cast<SKILL_TYPE>(Args.m_VarsLocal.GetKeyNum("skill", false));
				if (iSkillforced)
				{
					SPELL_TYPE iSpellforced = static_cast<SPELL_TYPE>(Args.m_VarsLocal.GetKeyNum("spell", false));
					if (g_Cfg.IsSkillFlag(iSkillforced, SKF_MAGIC))
						m_atMagery.m_Spell = iSpellforced;

					Skill_Start(iSkillforced);
					return;
				}
			}
			default:				break;
		}

		iDist		= static_cast<int>(Args.m_iN1);
		iMotivation = static_cast<int>(Args.m_iN2);
	}

	if ( ! IsStatFlag(STATF_Pet))
	{
		if ( iMotivation < 0 )
		{
			m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
			m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
			Skill_Start( NPCACT_FLEE );	// Run away!
			return;
		}
	}



	// Can only do that with full stamina !
	if ( !fSkipHardcoded && Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX))
	{
		// If I am a dragon maybe I will breath fire.
		// NPCACT_BREATH
		if ( m_pNPC->m_Brain == NPCBRAIN_DRAGON &&
			iDist >= 1 &&
			iDist <= 8 &&
			CanSeeLOS( pChar,LOS_NB_WINDOWS )) //Dragon can breath through a window
		{
			if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
				UpdateDir( pChar );
			Skill_Start( NPCACT_BREATH );
			return;
		}


		// any special ammunition defined?

		//check Range
		int iRangeMin = 2;
		int iRangeMax = 9;
		CVarDefCont * pRange = GetDefKey("THROWRANGE",true);
		if (pRange)
		{
			INT64 RVal[2];
			size_t iQty = Str_ParseCmds( const_cast<TCHAR*>(pRange->GetValStr()), RVal, COUNTOF(RVal));
			switch(iQty)
			{
				case 1:
					iRangeMax = static_cast<int>(RVal[0]);
					break;
				case 2:
					iRangeMin = static_cast<int>(RVal[0]);
					iRangeMax = static_cast<int>(RVal[1]);
					break;
			}
		}
		if (iDist >= iRangeMin && iDist <= iRangeMax && CanSeeLOS( pChar,LOS_NB_WINDOWS ))//NPCs can throw through a window
		{
			CVarDefCont * pRock = GetDefKey("THROWOBJ",true);
			if ( GetDispID() == CREID_OGRE || GetDispID() == CREID_ETTIN || GetDispID() == CREID_Cyclops || pRock )
			{
				ITEMID_TYPE id = ITEMID_NOTHING;
				if (pRock)
				{
					LPCTSTR t_Str = pRock->GetValStr();
					RESOURCE_ID_BASE rid = static_cast<RESOURCE_ID_BASE>(g_Cfg.ResourceGetID( RES_ITEMDEF, t_Str ));
					ITEMID_TYPE obj = static_cast<ITEMID_TYPE>(rid.GetResIndex());
					if ( ContentFind( RESOURCE_ID(RES_ITEMDEF,obj), 0, 2 ) )
						id = ITEMID_NODRAW;
				}
				else 
				{
					if ( ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_AROCK), 0, 2 ) )
						id = ITEMID_NODRAW;
				}


				if ( id != ITEMID_NOTHING )
				{
					if (!IsSetCombatFlags(COMBAT_NODIRCHANGE))
						UpdateDir( pChar );
					Skill_Start( NPCACT_THROWING );
					return;
				}
			}
		}
	}

	// Maybe i'll cast a spell if I can. if so maintain a distance.
	if ( NPC_FightMagery( pChar ))
		return;

	if ( NPC_FightArchery( pChar ))
		return;

	// Move in for melee type combat.
	NPC_Act_Follow( false, CalcFightRange( m_uidWeapon.ItemFind() ), false );
}

bool CChar::NPC_Act_Talk()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Talk");
	// NPCACT_TALK:
	// NPCACT_TALK_FOLLOW
	// RETURN:
	//  false = do something else. go Idle
	//  true = just keep waiting.

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )	// they are gone ?
		return( false );

	// too far away.
	int iDist = GetTopDist3D( pChar );
	if (( iDist >= UO_MAP_VIEW_SIGHT ) || ( m_ptHome.GetDist3D( pChar->GetTopPoint() ) > m_pNPC->m_Home_Dist_Wander ))	// give up.
		return( false );

	// can't see them
	if ( !CanSee( pChar ) )
		return( false );

	if ( Skill_GetActive() == NPCACT_TALK_FOLLOW && iDist > 3 )
	{
		// try to move closer.
		if ( ! NPC_Act_Follow( false, 4, false ))
			return( false );
	}

	if ( m_atTalk.m_WaitCount <= 1 )
	{
		if ( NPC_CanSpeak())
		{
			static LPCTSTR const sm_szText[] =
			{
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_1 ),
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_2 )
			};
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, sm_szText[ Calc_GetRandVal( COUNTOF( sm_szText )) ], pChar->GetName());
			Speak(pszMsg);
		}
		return( false );
	}

	m_atTalk.m_WaitCount--;
	return( true );	// just keep waiting.
}

void CChar::NPC_Act_GoHome()
{
	ADDTOCALLSTACK("CChar::NPC_Act_GoHome");
	// NPCACT_GO_HOME
	// If our home is not valid then
	if ( !m_pNPC )
		return;

	if ( !Calc_GetRandVal(2) )
	{
		if ( NPC_LookAround() )
			return;
	}

	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		// Had to change this guards were still roaming the forests
		// this goes hand in hand with the change that guards arent
		// called if the criminal makes it outside guarded territory.

		const CRegionBase * pArea = m_ptHome.GetRegion( REGION_TYPE_AREA );
		if ( pArea && pArea->IsGuarded())
		{
			if ( !m_pArea || !m_pArea->IsGuarded() )
			{
				if ( Spell_Teleport(m_ptHome, false, false) )
				{
					Skill_Start(SKILL_NONE);
					return;
				}
			}
		}
		else
		{
			g_Log.Event( LOGL_WARN, "Guard 0%lx '%s' has no guard post (%s)!\n", static_cast<DWORD>(GetUID()), GetName(), GetTopPoint().WriteUsed());

			// If we arent conjured and still got no valid home
			// then set our status to conjured and take our life.
			if ( ! IsStatFlag(STATF_Conjured))
			{
				StatFlag_Set( STATF_Conjured );
				Stat_SetVal(STAT_STR, -1000);
				return;
			}
		}

		// else we are conjured and probably a timer started already.
	}

	if ( !m_ptHome.IsValidPoint() || !GetTopPoint().IsValidPoint() || ( GetTopPoint().GetDist(m_ptHome) < m_pNPC->m_Home_Dist_Wander ))
	{
   		Skill_Start(SKILL_NONE);
		return;
	}

	if ( g_Cfg.m_iLostNPCTeleport )
	{
		int	iDistance	= m_ptHome.GetDist( GetTopPoint() );
		if ( (iDistance > g_Cfg.m_iLostNPCTeleport) && (iDistance > m_pNPC->m_Home_Dist_Wander) )
		{
			if ( IsTrigUsed(TRIGGER_NPCLOSTTELEPORT) )
			{
				CScriptTriggerArgs Args(iDistance);	// ARGN1 - distance
				if ( OnTrigger(CTRIG_NPCLostTeleport, this, &Args) != TRIGRET_RET_TRUE )
					Spell_Teleport(m_ptHome, true, false);
			}
			else
			{
				Spell_Teleport(m_ptHome, true, false);
			}
		}
	}

   	m_Act_p = m_ptHome;
   	if ( !NPC_WalkToPoint() ) // get there
   	{
   		Skill_Start(SKILL_NONE);
		return;
	}
}

void CChar::NPC_LootMemory( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::NPC_LootMemory");
	// Create a memory of this item.
	// I have already looked at it.
	CItem * pMemory = Memory_AddObjTypes( pItem, MEMORY_SPEAK );
	pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_IGNORE;

	// If the item is set to decay.
	if ( pItem->IsTimerSet() && pItem->GetTimerDiff() > 0 )
		pMemory->SetTimeout(pItem->GetTimerDiff());		// forget about this once the item is gone
}

void CChar::NPC_Act_Looting()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Looting");
	// We killed something, let's take a look on the corpse.
	// Or we find something interesting on ground
	//
	// m_Act_Targ = UID of the item/corpse that we trying to loot

	if ( !(NPC_GetAiFlags() & NPC_AI_LOOTING) )
		return;
	if ( !m_pNPC || m_pNPC->m_Brain != NPCBRAIN_MONSTER || !Can(CAN_C_USEHANDS) || IsStatFlag(STATF_Conjured|STATF_Pet) || (m_TagDefs.GetKeyNum("DEATHFLAGS", true) & DEATH_NOCORPSE) )
		return;
	if ( m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) )
		return;

	CItem * pItem = m_Act_Targ.ItemFind();
	if ( pItem == NULL )
		return;

	if ( GetDist(pItem) > 2 )	// move toward it
	{
		NPC_WalkToPoint();
		return;
	}

	CItemCorpse * pCorpse = dynamic_cast<CItemCorpse *>(pItem);
	if ( pCorpse && pCorpse->GetCount() > 0 )
		pItem = pCorpse->GetAt(Calc_GetRandVal(pCorpse->GetCount()));

	if ( !CanTouch(pItem) || !CanMove(pItem) || !CanCarry(pItem) )
	{
		NPC_LootMemory(pItem);
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
	// I should move faster this way.
	// ??? turn to strike if they are close.
	if ( ++ m_atFlee.m_iStepsCurrent >= m_atFlee.m_iStepsMax )
	{
		Skill_Start( SKILL_NONE );
		return;
	}
	if ( ! NPC_Act_Follow( true, m_atFlee.m_iStepsMax ))
	{
		Skill_Start( SKILL_NONE );
		return;
	}
}

void CChar::NPC_Act_Runto(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Runto");
	// NPCACT_RUNTO:
	// Still trying to get to this point.

	switch ( NPC_WalkToPoint(true))
	{
		case 0:
			// We reached our destination
			NPC_Act_Idle();	// look for something new to do.
			break;
		case 1:
			// Took a step....keep trying to get there.
			break;
		case 2:
			// Give it up...
			// Go directly there...
			if ( NPC_GetAiFlags()&NPC_AI_PERSISTENTPATH )
			{
				if (!GetTopPoint().IsValidPoint())
					iDist --;
				else
					iDist = iDist > m_Act_p.GetDist(GetTopPoint()) ? m_Act_p.GetDist(GetTopPoint()) : iDist-1;

				if (iDist)
					NPC_Act_Runto(iDist);
				else
					NPC_Act_Idle();
			}
			else
			{
				if ( m_Act_p.IsValidPoint() &&
					IsPlayableCharacter() &&
					!IsStatFlag( STATF_Freeze|STATF_Stone ))
					Spell_Teleport( m_Act_p, true, false);
				else
					NPC_Act_Idle();
			}
			break;
	}
}

void CChar::NPC_Act_Goto(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Goto");
	// NPCACT_GOTO:
	// Still trying to get to this point.

	switch ( NPC_WalkToPoint())
	{
		case 0:
			// We reached our destination
			NPC_Act_Idle();	// look for something new to do.
			break;
		case 1:
			// Took a step....keep trying to get there.
			break;
		case 2:
			// Give it up...
			// Go directly there...
			if ( NPC_GetAiFlags()&NPC_AI_PERSISTENTPATH )
			{
				if (!GetTopPoint().IsValidPoint())
					iDist --;
				else
					iDist = iDist > m_Act_p.GetDist(GetTopPoint()) ? m_Act_p.GetDist(GetTopPoint()) : iDist-1;

				if (iDist)
					NPC_Act_Runto(iDist);
				else
					NPC_Act_Idle();
			}
			else
			{
				if ( m_Act_p.IsValidPoint() &&
					IsPlayableCharacter() &&
					!IsStatFlag( STATF_Freeze|STATF_Stone ))
					Spell_Teleport( m_Act_p, true, false);
				else
					NPC_Act_Idle();	// look for something new to do.
			}
			break;
	}
}

bool CChar::NPC_Act_Food()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Food");
	int		iFood = Stat_GetVal(STAT_FOOD);
	int		iFoodLevel = Food_GetLevelPercent();

	if ( iFood >= 10 )
		return false;							//	search for food is starving or very hungry
	if ( iFoodLevel > 40 )
		return false;							// and it is at least 60% hungry

	m_pNPC->m_Act_Motivation = static_cast<unsigned char>((50 - (iFoodLevel / 2)));

	short	iEatAmount = 1;
	int		iSearchDistance = 2;
	CItem	*pClosestFood = NULL;
	int		iClosestFood = 100;
	int		iMyZ = GetTopPoint().m_z;
	bool	bSearchGrass = false;
	CItem	* pCropItem = NULL;

	CItemContainer	*pPack = GetPack();
	if ( pPack )
	{
		for ( CItem *pFood = pPack->GetContentHead(); pFood != NULL; pFood = pFood->GetNext() )
		{
			// I have some food personaly, so no need to search for something
			if ( pFood->IsType(IT_FOOD) )
			{
				if ( (iEatAmount = Food_CanEat(pFood)) > 0 )
				{
					Use_EatQty(pFood, iEatAmount);
					return true;
				}
			}
		}
	}

	// Search for food nearby
	iSearchDistance = (UO_MAP_VIEW_SIGHT * ( 100 - iFoodLevel ) ) / 100;
	CWorldSearch AreaItems(GetTopPoint(), minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
	for (;;)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;

		if ( pItem->IsType(IT_CROPS) || pItem->IsType(IT_FOLIAGE) )
		{
			// is it ripe?
			CItemBase * checkItemBase = pItem->Item_GetDef();
			if ( checkItemBase->m_ttNormal.m_tData3 )
			{
				// remember this, just in case we do not find any suitable food
				pCropItem = pItem;
				continue;
			}
		}

		if ( pItem->GetTopPoint().m_z > (iMyZ + 10) || pItem->GetTopPoint().m_z < (iMyZ - 1) )
			continue;
		if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) )
			continue;

		if ( (iEatAmount = Food_CanEat(pItem)) > 0 )
		{
			int iDist = GetDist(pItem);
			if ( pClosestFood )
			{
				if ( iDist < iClosestFood )
				{
					pClosestFood = pItem;
					iClosestFood = iDist;
				}
			}
			else
			{
				pClosestFood = pItem;
				iClosestFood = iDist;
			}
		}
	}

	if ( pClosestFood )
	{
		if ( iClosestFood <= 1 )
		{
			//	can take and eat just in place
			short iEaten = static_cast<short>(pClosestFood->ConsumeAmount(iEatAmount));
			EatAnim(pClosestFood->GetName(), iEaten);
			if ( !pClosestFood->GetAmount() )
			{
				pClosestFood->Plant_CropReset();	// set growth if this is a plant
			}
		}
		else
		{
			//	move towards this item
			switch ( m_Act_SkillCurrent )
			{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_GO_HOME:
				case NPCACT_Napping:
				case NPCACT_FLEE:
					{
						CPointMap pt = pClosestFood->GetTopPoint();
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							Skill_Start(NPCACT_GOTO);
							return true;
							//NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
						}
						break;
					}
				default:
					break;
			}
		}
	}
					// no food around, but maybe i am ok with grass? Or shall I try to pick crops?
	else
	{

		NPCBRAIN_TYPE	brain = GetNPCBrain();
		if ( brain == NPCBRAIN_ANIMAL )						// animals eat grass always
			bSearchGrass = true;
		//else if (( brain == NPCBRAIN_HUMAN ) && !iFood )	// human eat grass if starving nearly to death
		//	bSearchGrass = true;

		// found any crops or foliage at least (nearby, of course)?
		if ( pCropItem )
		{
			if ( GetDist(pCropItem) < 5 )
			{
				Use_Item(pCropItem);
				bSearchGrass = false;	// no need to eat grass if at next tick we can eat better stuff
			}
		}
	}
	if ( bSearchGrass )
	{
		CCharBase			*pCharDef = Char_GetDef();
		RESOURCE_ID_BASE	rid = RESOURCE_ID(RES_TYPEDEF, IT_GRASS);

		if ( pCharDef->m_FoodType.ContainsResourceID(rid) ) // do I accept grass as food?
		{
			CItem	*pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				short iEaten = static_cast<short>(pResBit->ConsumeAmount(10));
				EatAnim("grass", iEaten/10);

				//	the bit is not needed in a worldsave, timeout of 10 minutes
				pResBit->m_TagDefs.SetNum("NOSAVE", 1);
				pResBit->SetTimeout(60*10*TICK_PER_SEC);
				//DEBUG_ERR(("Starting skill food\n"));
				Skill_Start( NPCACT_FOOD );
				SetTimeout(5*TICK_PER_SEC);
				return true;
			}
			else									//	search for grass nearby
			{
				CPointMap pt = g_World.FindTypeNear_Top(GetTopPoint(), IT_GRASS, minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
				if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
				{
					if (( pt.m_x != GetTopPoint().m_x ) && ( pt.m_y != GetTopPoint().m_y ) && ( pt.m_map == GetTopPoint().m_map ))
					{
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							//DEBUG_ERR(("NPCACT_GOTO started; pt.x %d pt.y %d\n",pt.m_x,pt.m_y));
							Skill_Start(NPCACT_GOTO);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void CChar::NPC_Act_Idle()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Idle");
	// Free to do as we please. decide what we want to do next.
	// Idle NPC's should try to take some action.
	m_pNPC->m_Act_Motivation = 0;	// we have no motivation to do anything.

	if ( NPC_GetAiFlags()&NPC_AI_INTFOOD )
	{
		bool fFood = NPC_Act_Food();
		//DEBUG_ERR(("fFood %d\n",fFood));
		if ( fFood ) //are we hungry?
			return;
	}

	// Look around for things to do.
	if ( NPC_LookAround() )
		return;

	// ---------- If we found nothing else to do. do this. -----------

	// If guards are found outside guarded territories, do the following.
	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD && !m_pArea->IsGuarded() && m_ptHome.IsValidPoint())
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	// Specific creature random actions.
	if ( Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX) && !Calc_GetRandVal(3) )
	{
		if ( IsTrigUsed(TRIGGER_NPCSPECIALACTION) )
		{
			if ( OnTrigger( CTRIG_NPCSpecialAction, this ) == TRIGRET_RET_TRUE )
				return;
		}

		switch ( GetDispID())
		{
			case CREID_FIRE_ELEM:
				if ( !g_World.IsItemTypeNear(GetTopPoint(), IT_FIRE, 0, false) )
				{
					Action_StartSpecial(CREID_FIRE_ELEM);
					return;
				}
				break;

			default:
				// TAG.OVERRIDE.SPIDERWEB
				CVarDefCont * pValue = GetKey("OVERRIDE.SPIDERWEB",true);
				if ( pValue )
				{
					if ( GetDispID() != CREID_GIANT_SPIDER )
					{
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				} else {
					if ( GetDispID() == CREID_GIANT_SPIDER )
					{
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				}
		}
	}

	// Periodically head home.
	if ( m_ptHome.IsValidPoint() && ! Calc_GetRandVal( 15 ))
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	//	periodically use hiding skill
	if ( Skill_GetBase(SKILL_HIDING) > 30 &&
		! Calc_GetRandVal( 15 - Skill_GetBase(SKILL_HIDING)/100) &&
		!m_pArea->IsGuarded())
	{
		// Just hide here.
		if ( !IsStatFlag(STATF_Hidden) )
		{
			Skill_Start(SKILL_HIDING);
			return;
		}
	}

	if ( Calc_GetRandVal( 100 - Stat_GetAdjusted(STAT_DEX)) < 25 )
	{
		// dex determines how jumpy they are.
		// Decide to wander about ?
		Skill_Start( NPCACT_WANDER );
		return;
	}

	// just stand here for a bit.
	Skill_Start(SKILL_NONE);
	SetTimeout(TICK_PER_SEC * 1 + Calc_GetRandLLVal(TICK_PER_SEC*2));
}

bool CChar::NPC_OnItemGive( CChar *pCharSrc, CItem *pItem )
{
	ADDTOCALLSTACK("CChar::NPC_OnItemGive");
	// Someone (Player) is giving me an item.
	// return true = accept

	if ( !m_pNPC || !pCharSrc )
		return false;

	CScriptTriggerArgs Args(pItem);
	if ( IsTrigUsed(TRIGGER_RECEIVEITEM) )
	{
		if ( OnTrigger(CTRIG_ReceiveItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	// Giving item to own pet
	if ( NPC_IsOwnedBy(pCharSrc) )
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
				GetBank(LAYER_VENDOR_STOCK)->ContentAdd(pItem);
			}
			return true;
		}

		if ( Food_CanEat(pItem) )
		{
			if ( Use_Eat(pItem, pItem->GetAmount()) )
			{
				if ( NPC_CanSpeak() )
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_TY));

				if ( !pItem->IsDeleted() )		// if the NPC don't eat the full stack, bounce back the remaining amount on player backpack
					pCharSrc->ItemBounce(pItem);
				return true;
			}

			if ( NPC_CanSpeak() )
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_NO));
			return false;
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
		CItemContainer *pPack = GetPack();
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
				default:
					break;
			}
		}

		if ( m_pNPC->m_Brain == NPCBRAIN_BANKER )
		{
			CItemContainer *pBankBox = pCharSrc->GetPackSafe();
			if ( !pBankBox )
				return false;

			if ( NPC_CanSpeak() )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_BANKER_DEPOSIT), pItem->GetAmount());
				Speak(pszMsg);
			}

			pBankBox->ContentAdd(pItem);
			return true;
		}
	}

	if ( NPC_IsVendor() && !IsStatFlag(STATF_Pet) )
	{
		// Dropping item on vendor means quick sell
		if ( pCharSrc->IsClient() )
		{
			VendorItem item;
			item.m_serial = pItem->GetUID();
			item.m_amount = pItem->GetAmount();
			pCharSrc->GetClient()->Event_VendorSell(this, &item, 1);
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
	CItemContainer *pPack = GetPack();
	if ( !pPack )
		return false;
	pPack->ContentAdd(pItem);
	return true;
}

void CChar::NPC_OnTickAction()
{
	ADDTOCALLSTACK("CChar::NPC_OnTickAction");
	// Our action timer has expired. last skill or task might be complete ?
	// What action should we take now ?
	EXC_TRY("NPC_TickAction");

	if ( !m_pNPC )
		return;
	SKILL_TYPE iSkillActive = Skill_GetActive();
	if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_SCRIPTED ) )
	{
		// SCRIPTED SKILL OnTickAction
	}
	else if (g_Cfg.IsSkillFlag(iSkillActive, SKF_FIGHT))
	{
		EXC_SET("fighting");
		NPC_Act_Fight();
	}
	else if (g_Cfg.IsSkillFlag(iSkillActive, SKF_MAGIC))
	{
		EXC_SET("fighting-magic");
		NPC_Act_Fight();
	}
	else
	{
		switch ( iSkillActive )
		{
			case SKILL_NONE:
				// We should try to do something new.
				EXC_SET("idle: Skill_None");
				NPC_Act_Idle();
				break;

			case SKILL_STEALTH:
			case SKILL_HIDING:
				// We are currently hidden.
				EXC_SET("look around");
				if ( NPC_LookAround())
					break;
				// just remain hidden unless we find something new to do.
				if ( Calc_GetRandVal( Skill_GetBase(SKILL_HIDING)))
					break;
				EXC_SET("idle: Hidding");
				NPC_Act_Idle();
				break;

			case SKILL_ARCHERY:
			case SKILL_FENCING:
			case SKILL_MACEFIGHTING:
			case SKILL_SWORDSMANSHIP:
			case SKILL_WRESTLING:
			case SKILL_THROWING:
				// If we are fighting . Periodically review our targets.
				EXC_SET("fight");
				NPC_Act_Fight();
				break;

			case NPCACT_GUARD_TARG:
				// fight with the target, or follow it
				EXC_SET("guard");
				NPC_Act_Guard();
				break;

			case NPCACT_FOLLOW_TARG:
				// continue to follow our target.
				EXC_SET("look at char");
				NPC_LookAtChar( m_Act_Targ.CharFind(), 1 );
				EXC_SET("follow char");
				NPC_Act_Follow();
				break;
			case NPCACT_STAY:
				// Just stay here til told to do otherwise.
				break;
			case NPCACT_GOTO:
				EXC_SET("goto");
				NPC_Act_Goto();
				break;
			case NPCACT_WANDER:
				EXC_SET("wander");
				NPC_Act_Wander();
				break;
			case NPCACT_FLEE:
				EXC_SET("flee");
				NPC_Act_Flee();
				break;
			case NPCACT_TALK:
			case NPCACT_TALK_FOLLOW:
				// Got bored just talking to you.
				EXC_SET("talk");
				if ( ! NPC_Act_Talk())
				{
					EXC_SET("idle: Talk");
					NPC_Act_Idle();	// look for something new to do.
				}
				break;
			case NPCACT_GO_HOME:
				EXC_SET("go home");
				NPC_Act_GoHome();
				break;
			case NPCACT_LOOKING:
				EXC_SET("looking");
				if ( NPC_LookAround( true ) )
					break;
				EXC_SET("idle: Looking");
				NPC_Act_Idle();
				break;
			case NPCACT_FOOD:
				EXC_SET("Food Skill");
				if ( NPC_GetAiFlags() & NPC_AI_INTFOOD )
				{
					if ( ! NPC_Act_Food() )
						Skill_Start(SKILL_NONE);
				}
				break;
			case NPCACT_RUNTO:
				EXC_SET("Run To");
				NPC_Act_Runto();
				break;

			default:
				if ( !IsSkillBase(iSkillActive) )	// unassigned skill ? that's weird
				{
					Skill_Start(SKILL_NONE);
				}
				break;
		}
	}

	EXC_SET("timer expired");
	if ( IsTimerExpired() && IsStatFlag(STATF_War) && !(IsSetCombatFlags(COMBAT_PREHIT) && m_atFight.m_War_Swing_State == WAR_SWING_SWINGING))	// Was not reset? PREHIT forces timer to be 0, so it get's defaulted here breaking NPC's speed when PREHIT is enabled. Must not check in this case.
	{
		INT64 timeout	= maximum((150-Stat_GetAdjusted(STAT_DEX))/2, 0);
		timeout = Calc_GetRandLLVal2(timeout/2, timeout);
		// default next brain/move tick
		SetTimeout( TICK_PER_SEC + timeout * TICK_PER_SEC / 10 );
	}

	//	vendors restock periodically
	if ( NPC_IsVendor() )
		NPC_Vendor_Restock();

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' [0%lx]\n", GetName(), (DWORD)GetUID());
	EXC_DEBUG_END;
}

void CChar::NPC_Pathfinding()
{
	ADDTOCALLSTACK("CChar::NPC_Pathfinding");
	CPointMap local = GetTopPoint();

	EXC_TRY("Pathfinding");
	EXC_SET("pre-checking");

	if ( !m_pNPC )
		return;

	// If NPC_AI_ALWAYSINT is set, just make it as smart as possible.
	int			iInt = ( NPC_GetAiFlags() & NPC_AI_ALWAYSINT ) ? 300 : Stat_GetAdjusted(STAT_INT);
	CPointMap	pTarg = m_Act_p;
	int			dist = local.GetDist(pTarg);

	//	do we really need to find the path?
	if ( iInt < 75 ) return;					// too dumb
	if ( m_pNPC->m_nextPt == pTarg ) return;			// we have path to that position already saved in m_NextX/Y
	if ( !pTarg.IsValidPoint() ) return;		// invalid point
	if (( pTarg.m_x == local.m_x ) && ( pTarg.m_y == local.m_y )) return; // same spot
	if ( pTarg.m_map != local.m_map ) return;	// cannot just move to another map
	if ( dist >= PATH_SIZE/2 ) return;			// skip too far locations which should be too slow
	if ( dist < 2 ) return;						// skip too low distance (1 step) - good in default
												// pathfinding is buggy near the edges of the map,
												// so do not use it there
	if (( local.m_x <= PATH_SIZE/2 ) || ( local.m_y <= PATH_SIZE/2 ) ||
		( local.m_x >= ( g_MapList.GetX(local.m_map) - PATH_SIZE/2) ) ||
		( local.m_y >= ( g_MapList.GetY(local.m_map) - PATH_SIZE/2) ))
		return;
												// need 300 int at least to pathfind each step, but always
												// search if this is a first step
	if (( Calc_GetRandVal(300) > iInt ) && ( m_pNPC->m_nextX[0] )) return;

	//	clear saved steps list
	EXC_SET("clearing last steps");
#ifndef _WIN32
	for (int i_tmpN=0;i_tmpN < MAX_NPC_PATH_STORAGE_SIZE;i_tmpN++)
	{
		m_pNPC->m_nextX[i_tmpN] = 0;
		m_pNPC->m_nextY[i_tmpN] = 0;
	}
#else
	memset(m_pNPC->m_nextX, 0, sizeof(m_pNPC->m_nextX));
	memset(m_pNPC->m_nextY, 0, sizeof(m_pNPC->m_nextY));
#endif

	//	proceed with the pathfinding
	EXC_SET("filling the map");
	CPathFinder	path(this, pTarg);

	EXC_SET("searching the path");
	if ( path.FindPath() == PATH_NONEXISTENT )
		return;

	//	save the found path
	EXC_SET("saving found path");

	CPointMap Next;
	// Don't read the first step, it's the same as the current position, so i = 1
	for ( size_t i = 1; (i != path.LastPathSize()) && (i < MAX_NPC_PATH_STORAGE_SIZE /* Don't overflow*/ ); ++i )
	{
		Next = path.ReadStep(i);
		m_pNPC->m_nextX[i - 1] = Next.m_x;
		m_pNPC->m_nextY[i - 1] = Next.m_y;
	}
	m_pNPC->m_nextPt = pTarg;
	path.ClearLastPath(); // !! Use explicitly when using one CPathFinder object for more NPCs

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' point '%d,%d,%d,%d' [0%lx]\n", GetName(), local.m_x, local.m_y, local.m_z, local.m_map, (DWORD)GetUID());
	EXC_DEBUG_END;
}

void CChar::NPC_Food()
{
	ADDTOCALLSTACK("CChar::NPC_Food");
	EXC_TRY("FoodAI");

	int		iFood = Stat_GetVal(STAT_FOOD);
	int		iFoodLevel = Food_GetLevelPercent();
	short		iEatAmount = 1;
	int		iSearchDistance = 2;
	CItem	*pClosestFood = NULL;
	int		iClosestFood = 100;
	int		iMyZ = GetTopPoint().m_z;
	bool	bSearchGrass = false;

	if ( iFood >= 10 ) return;							//	search for food is starving or very hungry
	if ( iFoodLevel > 40 ) return;						// and it is at least 60% hungry

	CItemContainer	*pPack = GetPack();
	if ( pPack )
	{
		EXC_SET("searching in pack");
		for ( CItem *pFood = pPack->GetContentHead(); pFood != NULL; pFood = pFood->GetNext() )
		{
			// i have some food personaly, so no need to search for something
			if ( pFood->IsType(IT_FOOD) )
			{
				if ( (iEatAmount = Food_CanEat(pFood)) > 0 )
				{
					EXC_SET("eating from pack");
					Use_EatQty(pFood, iEatAmount);
					return;
				}
			}
		}
	}

	// Search for food nearby
	EXC_SET("searching nearby");
	iSearchDistance = (UO_MAP_VIEW_SIGHT * ( 100 - iFoodLevel ) ) / 100;
	CWorldSearch	AreaItems(GetTopPoint(), minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
	for (;;)
	{
		CItem	*pItem = AreaItems.GetItem();
		if ( !pItem ) break;
		if ( !CanSee(pItem) || pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) ) continue;
		if ( (pItem->GetTopPoint().m_z < iMyZ) || (pItem->GetTopPoint().m_z > (iMyZ + (m_height / 2))) )
			continue;

		if ( (iEatAmount = Food_CanEat(pItem)) > 0 )
		{
			int iDist = GetDist(pItem);
			if ( pClosestFood )
			{
				if ( iDist < iClosestFood )
				{
					pClosestFood = pItem;
					iClosestFood = iDist;
				}
			}
			else
			{
				pClosestFood = pItem;
				iClosestFood = iDist;
			}
		}
	}

	if ( pClosestFood )
	{
		if ( iClosestFood <= 1 )
		{
			//	can take and eat just in place
			EXC_SET("eating nearby");
			short iEaten = static_cast<short>(pClosestFood->ConsumeAmount(iEatAmount));
			EatAnim(pClosestFood->GetName(), iEaten);
			if ( !pClosestFood->GetAmount() )
			{
				pClosestFood->Plant_CropReset();	// set growth if this is a plant
			}
		}
		else
		{
			//	move towards this item
			switch ( m_Act_SkillCurrent )
			{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_GO_HOME:
				case NPCACT_Napping:
				case NPCACT_FLEE:
					{
						EXC_SET("walking to desired");
						CPointMap pt = pClosestFood->GetTopPoint();
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							Skill_Start(NPCACT_GOTO);
							//NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
						}
						break;
					}
				default:
					break;
			}
		}
	}
					// no food around, but maybe i am ok with grass?
	else
	{
		NPCBRAIN_TYPE	brain = GetNPCBrain();
		if ( brain == NPCBRAIN_ANIMAL )						// animals eat grass always
			bSearchGrass = true;
		else if (( brain == NPCBRAIN_HUMAN ) && !iFood )	// human eat grass if starving nearly dead
			bSearchGrass = true;
	}

	if ( bSearchGrass )
	{
		CCharBase			*pCharDef = Char_GetDef();
		RESOURCE_ID_BASE	rid = RESOURCE_ID(RES_TYPEDEF, IT_GRASS);

		EXC_SET("searching grass");
		if ( pCharDef->m_FoodType.ContainsResourceID(rid) ) // do I accept grass as a food?
		{
			CItem	*pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				EXC_SET("eating grass");
				short iEaten = static_cast<short>(pResBit->ConsumeAmount(15));
				EatAnim("grass", iEaten/10);

				//	the bit is not needed in a worldsave, timeout of 10 minutes
				pResBit->m_TagDefs.SetNum("NOSAVE", 1);
				pResBit->SetTimeout(60*10*TICK_PER_SEC);
				return;
			}
			else									//	search for grass nearby
			{
				switch ( m_Act_SkillCurrent )
				{
					case NPCACT_STAY:
					case NPCACT_GOTO:
					case NPCACT_WANDER:
					case NPCACT_LOOKING:
					case NPCACT_GO_HOME:
					case NPCACT_Napping:
					case NPCACT_FLEE:
						{
							EXC_SET("searching grass nearby");
							CPointMap pt;
								pt = g_World.FindTypeNear_Top(GetTopPoint(), IT_GRASS, minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
							if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
							{
								// we found grass nearby, but has it already been consumed?
								pResBit = g_World.CheckNaturalResource(pt, IT_GRASS, false, this);
								if ( pResBit != NULL && pResBit->GetAmount() && CanMoveWalkTo(pt) )
								{
									EXC_SET("walking to grass");
									pResBit->m_TagDefs.SetNum("NOSAVE", 1);
									pResBit->SetTimeout(60*10*TICK_PER_SEC);
									m_Act_p = pt;
									Skill_Start(NPCACT_GOTO);
									return;
								}
							}
							break;
						}
					default:
						break;
				}
			}
		}
	}
	EXC_CATCH;
}

void CChar::NPC_ExtraAI()
{
	ADDTOCALLSTACK("CChar::NPC_ExtraAI");
	EXC_TRY("ExtraAI");

	if ( !m_pNPC )
		return;
	if ( GetNPCBrain() != NPCBRAIN_HUMAN )
		return;

	EXC_SET("init");
	if ( IsTrigUsed(TRIGGER_NPCACTION) )
	{
		if ( OnTrigger( CTRIG_NPCAction, this ) == TRIGRET_RET_TRUE )
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
			CItemContainer * pPack = GetPack();
			if (pPack)
			{
				pShield = pPack->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_SHIELD));
				if (pShield)
					ItemEquip(pShield);
			}
		}
		return;
	}

	// Equip lightsource at night time
	EXC_SET("light source");
	CPointMap pt = GetTopPoint();
	CSector *pSector = pt.GetSector();
	if ( pSector && pSector->IsDark() )
	{
		CItem *pLightSourceCheck = LayerFind(LAYER_HAND2);
		if ( !(pLightSourceCheck && (pLightSourceCheck->IsType(IT_LIGHT_OUT) || pLightSourceCheck->IsType(IT_LIGHT_LIT))) )
		{
			CItem *pLightSource = ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_LIGHT_OUT));
			if ( pLightSource )
			{
				ItemEquip(pLightSource);
				Use_Obj(pLightSource, false);
			}
		}
	}
	else
	{
		CItem *pLightSource = LayerFind(LAYER_HAND2);
		if ( pLightSource && (pLightSource->IsType(IT_LIGHT_OUT) || pLightSource->IsType(IT_LIGHT_LIT)) )
			ItemBounce(pLightSource, false);
	}

	EXC_CATCH;
}
