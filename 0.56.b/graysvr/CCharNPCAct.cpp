//
// CCharNPCAct.CPP
// Copyright Menace Software (www.menasoft.com).
//
// Actions specific to an NPC.
//

#include "graysvr.h"	// predef header.
#include "../graysvr/CPathFinder.h"

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
	NV_SELL,
	NV_SHRINK,
	NV_TRAIN,
	NV_WALK,
	NV_QTY,
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
	"SELL",
	"SHRINK",
	"TRAIN",
	"WALK",
	NULL,
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
		// Open up the buy dialog.
		if ( !pCharSrc || !pCharSrc->IsClient())
			return false;
		if ( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
			return true;
		}
		if ( !pCharSrc->GetClient()->addShopMenuBuy(this) )
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NO_GOODS));
		else
			pCharSrc->GetClient()->m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
		break;
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
		//NPC_WalkToPoint(true);
		m_Act_p.Read(s.GetArgRaw());
		Skill_Start( NPCACT_GOTO );
		break;
	case NV_HIRE:
		return NPC_OnHireHear( pCharSrc);
	case NV_PETRETRIEVE:
		return( NPC_StablePetRetrieve( pCharSrc ));
	case NV_PETSTABLE:
		return( NPC_StablePetSelect( pCharSrc ));
	case NV_RESTOCK:	// individual restock command.
		return NPC_Vendor_Restock(true);
	case NV_RUN:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( true );
		break;
	case NV_SELL:
		// Open up the sell dialog.
		if ( !pCharSrc || !pCharSrc->IsClient() )
			return false;
		if ( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_OFFDUTY));
			return true;
		}
		if ( ! pCharSrc->GetClient()->addShopMenuSell( this ))
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NOTHING_BUY));
		else
			pCharSrc->GetClient()->m_TagDefs.SetNum("BUYSELLTIME", g_World.GetCurrentTime().GetTimeRaw());
		break;
	case NV_SHRINK:
		// we must own it.
		if ( ! NPC_IsOwnedBy( pCharSrc ))
			return( false );
		return( NPC_Shrink() != NULL );
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

	bool bRestockNow = bForce;

	if ( !bForce && m_pNPC->m_timeRestock.IsTimeValid() )
	{
		// Restock occurs every 10 minutes of inactivity (unless
		// region tag specifies different time)
		CRegionWorld *region = GetRegion();
		int restockIn = 10 * 60 * TICK_PER_SEC;
		if( region != NULL )
		{
			CVarDefCont *vardef = region->m_TagDefs.GetKey("RestockVendors");
			if( vardef != NULL )
				restockIn = vardef->GetValNum();
		}

		bRestockNow = ( CServTime::GetCurrentTime().GetTimeDiff(m_pNPC->m_timeRestock) > restockIn );
	}

	// At restock the containers are actually emptied
	if ( bRestockNow )
	{
		m_pNPC->m_timeRestock.Init();

		for ( int i = 0; i < COUNTOF(sm_VendorLayers); ++i )
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
			CCharBase *pCharDef = Char_GetDef();
			ReadScriptTrig(pCharDef, CTRIG_NPCRestock, true);

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
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_FULL ) );
		return( false );
	}

	CItem* pItem = pBank->GetContentHead();
	for ( ; pItem!=NULL ; pItem = pItem->GetNext())
	{
		if ( pItem->IsType( IT_FIGURINE ) && pItem->m_uidLink == pCharPlayer->GetUID())
			iCount++;
	}

	int iPetmax = 10;
	if ( m_TagDefs.GetKey("MAXPLAYERPETS") )
		iPetmax = m_TagDefs.GetKeyNum("MAXPLAYERPETS");

	if ( iCount > iPetmax )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TOOMANY ) );
		return( false );
	}

	pCharPlayer->m_pClient->m_Targ_PrvUID = GetUID();
	pCharPlayer->m_pClient->addTarget( CLIMODE_TARG_PET_STABLE, g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TARGSTABLE ) );

	return( true );
}

bool CChar::NPC_StablePetRetrieve( CChar * pCharPlayer )
{
	ADDTOCALLSTACK("CChar::NPC_StablePetRetrieve");
	// Get pets for this person from my inventory.
	// May want to put up a menu ???

	if ( m_pNPC == NULL )
		return( false );
	if ( m_pNPC->m_Brain != NPCBRAIN_STABLE )
		return( false );

	int iCount = 0;
	CItem* pItem = GetBank()->GetContentHead();
	while ( pItem!=NULL )
	{
		CItem * pItemNext = pItem->GetNext();
		if ( pItem->IsType( IT_FIGURINE ) && pItem->m_uidLink == pCharPlayer->GetUID())
		{
			if ( pCharPlayer->Use_Figurine( pItem, 2 ))
			{
				pItem->Delete();
			}
			iCount++;
		}
		pItem = pItemNext;
	}

	if ( ! iCount )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_NOPETS ) );
	}
	else
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_STABLEMASTER_TREATWELL ) );
	}

	return( true );
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

void CChar::NPC_OnHear( LPCTSTR pszCmd, CChar * pSrc )
{
	ADDTOCALLSTACK("CChar::NPC_OnHear");
	// This CChar has heard you say something.
	if ( !m_pNPC || !pSrc )
		return;

	// Pets always have a basic set of actions.
	if ( NPC_OnHearPetCmd(pszCmd, pSrc, false) || !NPC_CanSpeak() )
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
					char	*z = Str_GetTemp();
					sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_INTERRUPT), (LPCTSTR) pCharOld->GetName(), pSrc->GetName());
					Speak(z);
				}
			}
			break;
	}

	// I've heard them for the first time.
	CItemMemory * pMemory = Memory_FindObjTypes( pSrc, MEMORY_SPEAK );
	if ( pMemory == NULL )
	{
		// This or CTRIG_SeeNewPlayer will be our first contact with people.
		if ( OnTrigger( CTRIG_NPCHearGreeting, pSrc ) == TRIGRET_RET_TRUE )
			return;

		// record that we attempted to speak to them.
		pMemory = Memory_AddObjTypes(pSrc, MEMORY_SPEAK);
		if ( pMemory )
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
	}

	// Do the scripts want me to take some action based on this speech.
	SKILL_TYPE skill = m_Act_SkillCurrent;

	TALKMODE_TYPE	mode	= TALKMODE_SAY;
	int i;
	for ( i=0; i<m_pNPC->m_Speech.GetCount(); i++ )
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
	for ( i=0; i<pCharDef->m_Speech.GetCount(); i++ )
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
	if ( OnTrigger( CTRIG_NPCHearUnknown, pSrc ) == TRIGRET_RET_TRUE )
		return;

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
	int iTrainCost = NPC_GetTrainMax( pCharSrc, Skill ) - iSkillSrcVal;

	// Train npc skill cap
	if ((pCharSrc->GetSkillTotal() + iTrainCost) > pCharSrc->Skill_GetMax( (SKILL_TYPE)MAX_SKILL ))
		iTrainCost = 0;

	LPCTSTR pszMsg;
	if ( iSkillVal <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_2 );
	}
	else if ( iSkillSrcVal > iSkillVal )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_3 );
	}
	else if ( iTrainCost <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_4 );
	}
	else
	{
		return( iTrainCost );
	}

	char	*z = Str_GetTemp();
	sprintf(z, pszMsg, g_Cfg.GetSkillKey(Skill));
	Speak(z);
	return 0;
}

bool CChar::NPC_OnTrainPay(CChar *pCharSrc,CItemMemory *pMemory, CItem * pGold)
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainPay");
	SKILL_TYPE skill = (SKILL_TYPE)( pMemory->m_itEqMemory.m_Skill );
	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_FORGOT));
		return false;
	}

	int iTrainCost = NPC_OnTrainCheck(pCharSrc, skill);
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
	pCharSrc->Skill_SetBase( skill, pCharSrc->Skill_GetBase(skill) + iTrainCost );
	return( true );
}

bool CChar::NPC_OnTrainHear( CChar * pCharSrc, LPCTSTR pszCmd )
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainHear");
	// We are asking for training ?

	if ( ! m_pNPC )
		return( false );

	// Check the NPC is capable of teaching
	if ( (m_pNPC->m_Brain < NPCBRAIN_HUMAN) || (m_pNPC->m_Brain > NPCBRAIN_THIEF) )
		return( false );

	// Check the NPC isn't busy fighting
	if ( Memory_FindObjTypes( pCharSrc, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_ENEMY ) );
		return true;
	}

	// Did they mention a skill name i recognize ?
	TemporaryString pszMsg;

	int i=SKILL_NONE+1;
	for ( ; i<MAX_SKILL; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex( i ) )
			continue;

		LPCTSTR pSkillKey = g_Cfg.GetSkillKey( (SKILL_TYPE) i );
		if ( ! FindStrWord( pszCmd, pSkillKey ))
			continue;

		// Can we train in this ?
		int iTrainCost = NPC_OnTrainCheck( pCharSrc, (SKILL_TYPE) i );
		if ( iTrainCost <= 0 )
			return true;

		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE), iTrainCost, (LPCTSTR)pSkillKey);
		Speak(pszMsg);
		CItemMemory * pMemory = Memory_AddObjTypes( pCharSrc, MEMORY_SPEAK );
		if ( pMemory )
		{
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_TRAIN;
			pMemory->m_itEqMemory.m_Skill = i;
		}
		return true;
	}

	// What can he teach me about ?
	// Just tell them what we can teach them or set up a memory to train.
	strcpy( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_1 ) );

	LPCTSTR pPrvSkill = NULL;

	int iCount = 0;
	for ( i = (SKILL_NONE+1); i < MAX_SKILL; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex( i ) )
			continue;

		int iDiff = NPC_GetTrainMax( pCharSrc, (SKILL_TYPE)i ) - pCharSrc->Skill_GetBase( (SKILL_TYPE) i);
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

		pPrvSkill = g_Cfg.GetSkillKey( (SKILL_TYPE) i );
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

bool CChar::NPC_Act_Begging( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_Act_Begging");
	// SKILL_BEGGING
	bool fSpeak;

	if ( pChar )
	{
		// Is this a proper target for begging ?
		if ( pChar == this ||
			pChar->m_pNPC ||	// Don't beg from NPC's or the PC you just begged from
			pChar->GetUID() == m_Act_TargPrv )	// Already targetting this person.
			return( false );

		if ( pChar->GetUID() == m_Act_Targ && Skill_GetActive() == SKILL_BEGGING )
			return( true );

		Skill_Start( SKILL_BEGGING );
		m_Act_Targ = pChar->GetUID();
		fSpeak = true;
	}
	else
	{
		// We are already begging.
		pChar = m_Act_Targ.CharFind();
		if (( pChar == NULL ) ||
		    ((! CanSee( pChar )) && ( Calc_GetRandVal(75) > Stat_GetAdjusted(STAT_INT))))  // Dumb beggars think I'm gone
		{
			m_Act_Targ.InitUID();
			Skill_Start( SKILL_NONE );
			return( false );
		}

		// Time to beg ?
		fSpeak = ! Calc_GetRandVal(6);
	}

	SetTimeout( 2*TICK_PER_SEC );

	static UINT const sm_szSpeakBeggar[] =
	{
		DEFMSG_NPC_BEGGAR_BEG_1,
		DEFMSG_NPC_BEGGAR_BEG_2,
		DEFMSG_NPC_BEGGAR_BEG_3,
		DEFMSG_NPC_BEGGAR_BEG_4,
		DEFMSG_NPC_BEGGAR_BEG_5,
		DEFMSG_NPC_BEGGAR_BEG_6,
	};

	UpdateDir( pChar );	// face PC
	if ( fSpeak )
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szSpeakBeggar[ Calc_GetRandVal( COUNTOF( sm_szSpeakBeggar )) ]) );
	}

	if ( ! Calc_GetRandVal( ( 10100 - pChar->Stat_GetAdjusted(STAT_FAME)) / 50 ))
	{
		UpdateAnimate( ANIM_BOW );
		return( true );
	}

	return( NPC_Act_Follow( false, 2 ));
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
		DEFMSG_NPC_GENERIC_SNOOPED_4,
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
	if ( g_Cfg.m_iNpcAi & NPC_AI_PATH )
	{
		NPC_Pathfinding();

		//	walk the saved path
		CPointMap local;
		local.m_x = m_pNPC->m_nextX[0];
		local.m_y = m_pNPC->m_nextY[0];
			// no steps available yet, or pathfinding not usable in this situation
			// so, use default movements
		if (!(( local.m_x < 1 ) || ( local.m_y < 1 )))
		{
			Dir = pMe.GetDir(local);
			local = pMe;
			pMe.Move(Dir);

			EXC_TRYSUB("Array Shift");
			//	also shift the steps array
			for ( int j = 0; j < MAX_NPC_PATH_STORAGE_SIZE-1; ++j )
			{
				m_pNPC->m_nextX[j] = m_pNPC->m_nextX[j+1];
				m_pNPC->m_nextY[j] = m_pNPC->m_nextY[j+1];
			}
			m_pNPC->m_nextX[MAX_NPC_PATH_STORAGE_SIZE-1] = 0;
			m_pNPC->m_nextY[MAX_NPC_PATH_STORAGE_SIZE-1] = 0;
			EXC_CATCHSUB("NPCAI");

			EXC_SET("CanMoveWalkTo after array shift");
			//	do the recorded move
			if ( CanMoveWalkTo(pMe, true, false, Dir) ) //	make standart default move if failed the recorded one
				bUsePathfinding = true;
			else
				pMe = local; //no pathfinding
		}
	}
	EXC_SET("Non-Advanced pathfinding");
	if ( ! bUsePathfinding )
	{
//		DEBUG_ERR(("ADVAI:%d: NPC '%s' uses default walk\n", irrr, GetName()));
		pMe.Move( Dir );
		if ( ! CanMoveWalkTo(pMe, true, false, Dir ) )
		{
			CPointMap	ptFirstTry = pMe;

			// try to step around it ?
			int iDiff = 0;
			int iRand = Calc_GetRandVal( 100 );
			if ( iRand < 30 )	// do nothing.
				return( 2 );
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
				if ( !pCharDef->Can(CAN_C_USEHANDS) ) ;		// i cannot use hands, so cannot move objects
				else if (( g_Cfg.m_iNpcAi&NPC_AI_MOVEOBSTACLES ) && ( iInt > iRand ))
				{
					int			i;
					CPointMap	point;
					for ( i = 0; i < 2; i++ )
					{
						if ( !i ) point = pMe;
						else point = ptFirstTry;

						//	Scan point for items that could be moved by me and move them to my position
						CWorldSearch	AreaItems(point);
						while ( true )
						{
							CItem	*pItem = AreaItems.GetItem();
							if ( !pItem ) break;
							else if ( abs(pItem->GetTopZ() - pMe.m_z) > 3 ) continue;		// item is too high
							else if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_OWNED|ATTR_INVIS|ATTR_STATIC|ATTR_MAGIC) ) bClearedWay = false;
							else if ( !pItem->Item_GetDef()->Can(CAN_I_BLOCK) ) continue;	// this item not blocking me
							else if ( !CanCarry(pItem) ) bClearedWay = false;
							else
							{
								//	If the item is a valuable one, investigate the way to loot it
								//	or not. Do it only with first direction we try to move items and
								//	if we are not too busy with something else
								SKILL_TYPE	st = Skill_GetActive();
								if ( !Calc_GetRandVal(4) )	// 25% chance for item review
								{
									if (( st == NPCACT_WANDER ) || ( st == NPCACT_LOOKING ) || ( st == NPCACT_GO_HOME ))
									{
										CItemVendable *pVend = dynamic_cast <CItemVendable*>(pItem);
										if ( pVend )	// the item is valuable
										{
											LONG price = pVend->GetBasePrice();
														//	the more intelligent i am, the more valuable
														//	staff I will look to
											if ( Calc_GetRandVal(price) < iInt )
											{
												m_Act_Targ = pItem->GetUID();
												NPC_Act_Looting();
												bClearedWay = false;
												break;
											}
										}
									}
								}
								bClearedWay = true;

								//	move this item to the position I am currently in
								pItem->SetTopPoint(GetTopPoint());
								pItem->Update();
							}

							if ( !bClearedWay ) break;
						}

						if ( bClearedWay ) break;
						//	If not cleared the way still, but I am still clever enough
						//	I should try to move in the first step I was trying to move to
						else if ( iInt < iRand*3 ) break;
					}

					//	we have just cleared our way
					if ( bClearedWay )
						if ( point == ptFirstTry )
							Dir = pMe.GetDir(m_Act_p);
				}
				if ( !bClearedWay ) return 2;
			}
		}
	}

	EXC_SET("Finishing Move Action a");
	//Finish Move Action

	// ??? Make sure we are not facing a wall.
	m_dirFace = Dir;	// Face this direction.
	if ( fRun && ( ! pCharDef->Can(CAN_C_RUN|CAN_C_FLY) || Stat_GetVal(STAT_DEX) <= 1 ))
		fRun = false;

	EXC_SET("StatFlag");
	StatFlag_Mod(STATF_Fly, fRun);

	EXC_SET("Old Top Point");
	CPointMap pold = GetTopPoint();

	EXC_SET("Revel");
	CheckRevealOnMove();

	EXC_SET("MoveToChar");
	MoveToChar(pMe);

	EXC_SET("Move Update");
	UpdateMove(pold);

	EXC_SET("Check Location");
	CheckLocation(false);	// Look for teleports etc.

	EXC_SET("Speed counting");
	// How fast can they move.
	int iTickNext;
	if ( fRun )
	{
		if ( IsStatFlag( STATF_Pet ))	// pets run a little faster.
		{
			if ( iDex < 75 )
				iDex = 75;
		}
		iTickNext = TICK_PER_SEC/4 + Calc_GetRandVal( (100-iDex)/5 ) * TICK_PER_SEC / 10;
	}
	else
		iTickNext = TICK_PER_SEC + Calc_GetRandVal( (100-iDex)/3 ) * TICK_PER_SEC / 10;

// TAG.OVERRIDE.MOVERATE
	CVarDefCont * pValue = GetKey("OVERRIDE.MOVERATE",true);
	if ( pValue ) 
	{
		int tTick = pValue->GetValNum();
		if ( tTick < 1 ) tTick=1;
		//g_Log.EventDebug("tag found: %d\n",tTick);
		iTickNext = ( iTickNext * tTick ) / 100;
	}
	else
	{
// END TAG.OVERRIDE.MOVERATE

		//g_Log.EventDebug("prop found: %d\n",pCharDef->m_iMoveRate);
		iTickNext = (iTickNext * pCharDef->m_iMoveRate)/100;

// TAG.OVERRIDE.MOVERATE
	}
// END TAG.OVERRIDE.MOVERATE

	if ( iTickNext < 1 )
		iTickNext = 1;

	SetTimeout(iTickNext);
	EXC_CATCH;
	return 1;
}

bool CChar::NPC_LookAtCharGuard( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharGuard");
	// Does the guard hate the target ?

	//	do not waste time on invul+dead, non-criminal and jailed chars
	if ( !pChar || pChar->IsStatFlag(STATF_INVUL|STATF_DEAD) || !pChar->Noto_IsCriminal() || pChar->IsPriv(PRIV_JAILED) )
		return false;

	static UINT const sm_szSpeakGuardJeer[] =
	{
		DEFMSG_NPC_GUARD_THREAT_1,
		DEFMSG_NPC_GUARD_THREAT_2,
		DEFMSG_NPC_GUARD_THREAT_3,
		DEFMSG_NPC_GUARD_THREAT_4,
		DEFMSG_NPC_GUARD_THREAT_5,
	};

	if ( ! pChar->m_pArea->IsGuarded())
	{
		// At least jeer at the criminal.
		if ( Calc_GetRandVal(10))
			return( false );

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(sm_szSpeakGuardJeer[ Calc_GetRandVal( COUNTOF( sm_szSpeakGuardJeer )) ]), (LPCTSTR) pChar->GetName());
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
		DEFMSG_NPC_GUARD_STRIKE_5,
	};

	if ( GetTopDist3D(pChar) > 1 )
	{
		if ( g_Cfg.m_fGuardsInstantKill || pChar->Skill_GetBase(SKILL_MAGERY) )
			Spell_Teleport(pChar->GetTopPoint(), false, false);

		// If we got intant kill guards enabled, allow the guards to swing immidiately
		if ( g_Cfg.m_fGuardsInstantKill )
			Fight_Hit(pChar);
	}
	if ( !IsStatFlag(STATF_War) || m_Act_Targ != pChar->GetUID() )
	{
		Speak(g_Cfg.GetDefaultMsg(sm_szSpeakGuardStrike[Calc_GetRandVal(COUNTOF(sm_szSpeakGuardStrike))]));
		Fight_Attack(pChar);
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

	Fight_Attack( pChar );
	m_pNPC->m_Act_Motivation = iActMotivation;
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

	if ( ! pChar->Noto_IsCriminal())	// not interesting.
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
	static LPCTSTR const sm_szHealerRefuseEvils[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_3 ),
	};
	static LPCTSTR const sm_szHealerRefuseCriminals[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_3 ),
	};
	static LPCTSTR const sm_szHealerRefuseGoods[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_3 ),
	};
	static LPCTSTR const sm_szHealer[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_5 ),
	};

	if ( ! pChar->IsStatFlag( STATF_DEAD ))
		return false;

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

bool CChar::NPC_LookAtItem( CItem * pItem, int iDist )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtItem");

	// I might want to go pickup this item ?
	if ( ! CanSee( pItem ))
		return( false );

	int iWantThisItem = NPC_WantThisItem(pItem);

	if ( IsSetEF( EF_New_Triggers ) )
	{
		if ( !pItem->IsAttr( ATTR_MOVE_NEVER ) )
		{

			CScriptTriggerArgs	Args( iDist, iWantThisItem, pItem );
			switch( OnTrigger( CTRIG_NPCLookAtItem, this, &Args ) )
			{
#ifdef _NAZDEBUG
			case  TRIGRET_RET_TRUE:	
				g_Log.EventError("CChar::NPC_LookAtItem: CTRIG_NPCLookAtItem on '%s' returned TRUE\n", GetName() );
				return false;
			case  TRIGRET_RET_FALSE:	
				g_Log.EventError("CChar::NPC_LookAtItem: CTRIG_NPCLookAtItem on '%s' returned FALSE\n", GetName() );
				return true;
#else
			case  TRIGRET_RET_TRUE:		return true;
			case  TRIGRET_RET_FALSE:	return false;
#endif
			}
#ifdef _NAZDEBUG
			g_Log.EventError("CChar::NPC_LookAtItem: CTRIG_NPCLookAtItem on '%s' returned DEFAULT\n", GetName() );
#endif
			iWantThisItem = Args.m_iN2;
		}
	}

	//	loot the item if wish to loot it, and not already looted
	if ( iWantThisItem && ( Memory_FindObj( pItem ) == NULL ))
	{
		if ( Calc_GetRandVal(100) < iWantThisItem )
		{
			m_Act_Targ = pItem->GetUID();
			CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
			m_Act_TargPrv = pObjTop->GetUID();
			m_Act_p = pObjTop->GetTopPoint();
			m_atLooting.m_iDistEstimate = GetTopDist(pObjTop) * 2;
			m_atLooting.m_iDistCurrent = 0;
			Skill_Start(NPCACT_LOOTING);
			return true;
		}
	}

	CCharBase * pCharDef = Char_GetDef();

	// check for crops we can rip.
	if (  pItem->IsType( IT_CROPS ) ||  pItem->IsType( IT_FOLIAGE ) )
	{
		CItemBase * checkItemBase = pItem->Item_GetDef();
		if ( checkItemBase->m_ttNormal.m_tData3 )
		{
			if (Food_GetLevelPercent() < 100)
			{
 				if (GetDist( pItem ) <= 2) 
				{
					if (CanTouch( pItem ) )
					{
						if (!Calc_GetRandVal(2))
						{
							Use_Item( pItem );
							return true;
						}
					}
				}
				else
				{
					// Walk towards it
					CPointMap pt = pItem->GetTopPoint();
					if ( CanMoveWalkTo( pt ))
					{
						m_Act_p = pt;
						NPC_WalkToPoint();
						return true;
					}
				}
			}
		}
	}


	// check for doors we can open.
	if ( Stat_GetAdjusted(STAT_INT) > 20 &&
		pCharDef->Can(CAN_C_USEHANDS) &&
		pItem->IsType( IT_DOOR ) &&	// not locked.
		GetDist( pItem ) <= 1 &&
		CanTouch( pItem ) &&
		!Calc_GetRandVal(2))
	{
		// Is it opened or closed?
		if ( pItem->IsDoorOpen())
			return( false );

		// The door is closed.
		UpdateDir( pItem );
		if ( ! Use_Item( pItem ))	// try to open it.
			return( false );

		// Walk through it
		CPointMap pt = GetTopPoint();
		pt.MoveN( GetDir( pItem ), 2 );
		if ( CanMoveWalkTo( pt ))
		{
			m_Act_p = pt;
			NPC_WalkToPoint();
			return true;
		}
	}

	return( false );
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

	if ( IsSetEF( EF_New_Triggers ) )
	{
		switch ( OnTrigger(CTRIG_NPCLookAtChar, pChar) )
		{
		case  TRIGRET_RET_TRUE:		return true;
		case  TRIGRET_RET_FALSE:	return false;
		}
	}

	if ( NPC_IsOwnedBy( pChar, false ))
	{
		// pets should protect there owners unless told otherwise.
		if ( pChar->Fight_IsActive())
		{
			CChar * pCharTarg = pChar->m_Act_Targ.CharFind();
			if ( Fight_Attack(pCharTarg))
				return true;
		}

		// follow my owner again. (Default action)
		m_Act_Targ = pChar->GetUID();
		m_atFollowTarg.m_DistMin = 1;
		m_atFollowTarg.m_DistMax = 6;
		m_pNPC->m_Act_Motivation = 50;
		Skill_Start( NPCACT_FOLLOW_TARG );
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
			if ( OnTrigger( CTRIG_NPCSeeNewPlayer, pChar ) != TRIGRET_RET_TRUE )
			{
				// record that we attempted to speak to them.
				CItemMemory * pMemory = Memory_AddObjTypes( pChar, MEMORY_SPEAK );
				pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
				// m_Act_Hear_Unknown = 0;
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

	case NPCBRAIN_BEGGAR:
		if ( NPC_Act_Begging( pChar ))
			return true;
		if ( NPC_LookAtCharHuman( pChar ))
			return( true );
		break;

	case NPCBRAIN_MONSTER:
	case NPCBRAIN_UNDEAD:
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
		Fight_Attack( pChar );
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
	case NPCBRAIN_THIEF:
		if ( NPC_LookAtCharHuman(pChar) )
			return true;
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

	if ( g_Cfg.m_iNpcAi&NPC_AI_INTFOOD )
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

	int iRange = UO_MAP_VIEW_SIZE;
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
			m_Act_p.Move( (DIR_TYPE) Calc_GetRandVal( DIR_QTY ));
			NPC_WalkToPoint( true );
			return( true );
		}

		if ( Stat_GetAdjusted(STAT_INT) < 50 )
			iRangeBlur /= 2;
	}

	// Lower the number of chars we look at if complex.
	if ( pSector->GetCharComplexity() > g_Cfg.m_iMaxCharComplexity / 2 )
		iRange /= 4;

	// Any interesting chars here ?
	CWorldSearch Area(GetTopPoint(), iRange);
	while ( true )
	{
		CChar	*pChar = Area.GetChar();
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
		CWorldSearch Area( GetTopPoint(), iRange );
		while ( true )
		{
			CItem	*pItem = Area.GetItem();
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

	if ( ! Calc_GetRandVal( 7 + ( Stat_GetVal(STAT_DEX) / 30 )))
	{
		// Stop wandering ?
		Skill_Start( SKILL_NONE );
		return;
	}

	if ( Calc_GetRandVal( 2 ) )
	{
		if ( NPC_LookAround() )
			return;
	}

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

bool CChar::NPC_Act_Follow( bool fFlee, int maxDistance, bool forceDistance )
{
	ADDTOCALLSTACK("CChar::NPC_Act_Follow");
	// Follow our target or owner. (m_Act_Targ) we may be fighting.
	// false = can't follow any more. give up.
	EXC_TRY("NPC_Act_Follow")
	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		// free to do as i wish !
		Skill_Start( SKILL_NONE );
		return( false );
	}

	EXC_SET("Trigger");
	if ( IsSetEF( EF_New_Triggers ) )
	{
		CScriptTriggerArgs Args( fFlee, maxDistance, forceDistance );
		switch ( OnTrigger( CTRIG_NPCActFollow, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return FALSE;
			case TRIGRET_RET_FALSE:	return TRUE;
		}
		fFlee			= (Args.m_iN1 != 0);
		maxDistance		= Args.m_iN2;
		forceDistance	= Args.m_iN3;
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
		if ( ! Calc_GetRandVal( 1 + (( 100 - Stat_GetAdjusted(STAT_INT)) / 20 )))
			return( false );
		if ( fFlee )
			return( false );
	}

	EXC_SET("Distance checks");
	int dist = GetTopPoint().GetDist( m_Act_p );
	if ( dist > UO_MAP_VIEW_RADAR*2 )			// too far away ?
		return( false );

	if ( forceDistance )
	{
		if ( dist < maxDistance )
		{
			// start moving away
			fFlee = true;
		}
	}
	else
	{
		if ( fFlee )
		{
			if ( dist >= maxDistance )
				return false;
		}
		else if ( dist <= maxDistance )
			return true;
	}

	EXC_SET("Fleeing");
	if ( fFlee )
	{
		CPointMap ptOld = m_Act_p;
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirTurn( m_Act_p.GetDir( ptOld ), 4 + 1 - Calc_GetRandVal(3)));
		NPC_WalkToPoint( dist < Calc_GetRandVal(10));
		m_Act_p = ptOld;	// last known point of the enemy.
		return( true );
	}

	EXC_SET("WalkToPoint 1");
	NPC_WalkToPoint( IsStatFlag( STATF_War ) ? true : ( dist > 3 ));
	return( true );
	EXC_CATCH;
	return false;
}

bool CChar::NPC_FightArchery( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_FightArchery");
	if ( Skill_GetActive() != SKILL_ARCHERY )
		return( false );

	int iDist = GetTopDist3D( pChar );
	if ( iDist > g_Cfg.m_iArcheryMaxDist )	// way too far away . close in.
		return( false );

	if ( iDist > g_Cfg.m_iArcheryMinDist )
		return( true );		// always use archery if distant enough

	if ( !Calc_GetRandVal( 2 ) )	// move away
	{
		// Move away
		NPC_Act_Follow( false, 5, true );
		return( true );
	}

	// Fine here.
	return( true );
}

bool CChar::NPC_FightMagery( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_FightMagery");
	// cast a spell if i can ?
	// or i can throw or use archery ?
	// RETURN:
	//  false = revert to melee type fighting.

	if ( ! NPC_FightMayCast())
		return( false );
	CChar	*pMageryTarget = pChar;

	int iDist = GetTopDist3D( pChar );
	if ( iDist > ((UO_MAP_VIEW_SIGHT*3)/4))	// way too far away . close in.
		return( false );

	if ( iDist <= 1 &&
		Skill_GetBase(SKILL_TACTICS) > 200 &&
		! Calc_GetRandVal(2))
	{
		// Within striking distance.
		// Stand and fight for a bit.
		return( false );
	}

	// A creature with a greater amount of mana will have a greater
	// chance of casting
	int iStatInt = Stat_GetAdjusted(STAT_INT);
	int iSkillVal = Skill_GetBase(SKILL_MAGERY);
	int mana	= Stat_GetVal(STAT_INT);
	int iChance = iSkillVal +
		(( mana >= ( iStatInt / 2 )) ? mana : ( iStatInt - mana ));
	if ( Calc_GetRandVal( iChance ) < 400 )
	{
		// we failed this test, but we could be casting next time
		// back off from the target a bit
		if ( mana > ( iStatInt / 3 ) && Calc_GetRandVal( iStatInt ))
		{
			if ( iDist < 4 || iDist > 8  )	// Here is fine?
			{
				NPC_Act_Follow( false, Calc_GetRandVal( 3 ) + 2, true );
			}
			return( true );
		}
		return( false );
	}

	// select proper spell.
	// defensive spells ???
	int imaxspell = minimum(( iSkillVal / 12 ) * 8, SPELL_BASE_QTY ) +1;

	// does the creature have a spellbook.
	CItem * pSpellbook = GetSpellbook();
	int i;

	CVarDefCont	*	pVar	= GetKey( "CASTSPELLS", true );

	if ( pVar )
		imaxspell	= i = GETINTRESOURCE( pVar->GetValNum() );
	else
		i = Calc_GetRandVal( imaxspell );

	for ( ; 1; i++ )
	{
		if ( i > imaxspell )	// didn't find a spell.
			return( false );

		SPELL_TYPE spell = (SPELL_TYPE) i;
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
		if ( pSpellDef == NULL )
			continue;

		if ( pSpellDef->IsSpellType(SPELLFLAG_DISABLED|SPELLFLAG_PLAYERONLY) ) continue;

		if ( pSpellbook )
		{
			if ( ! pSpellbook->IsSpellInBook(spell))
				continue;

			if ( ! pSpellDef->IsSpellType( SPELLFLAG_HARM ))
			{
				if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pSpellDef->IsSpellType(SPELLFLAG_GOOD) )
				{
					//	help self or friends if needed. support 3 friends + self for castings
					bool	bSpellSuits = false;
					CChar	*pFriend[4];
					int		iFriendIndex = 0;
					CChar	*pTarget = NULL;

					//	since i scan the surface near me for this code, i need to be sure that it is neccessary
					if (( spell != SPELL_Heal ) && ( spell != SPELL_Great_Heal ) && ( spell != SPELL_Reactive_Armor ) &&
						( spell != SPELL_Cure ) && ( spell != SPELL_Protection ) && ( spell != SPELL_Bless ) &&
						( spell != SPELL_Magic_Reflect )) continue;

					pFriend[0] = this;
					pFriend[1] = pFriend[2] = pFriend[3] = NULL;
					iFriendIndex = 1;

					if ( g_Cfg.m_iNpcAi&NPC_AI_COMBAT )
					{
						//	search for the neariest friend in combat
						CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIGHT);
						while ( true )
						{
							pTarget = AreaChars.GetChar();
							if ( !pTarget )
								break;

							CItem *pMemory = pTarget->Memory_FindObj(pChar);
							if ( pMemory && pMemory->IsMemoryTypes(MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY) )
							{
								pFriend[iFriendIndex++] = pTarget;
								if ( iFriendIndex >= 4 ) break;
							}
						}
					}

					//	i cannot cast this on self. ok, then friends only
					if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF) )
					{
						pFriend[0] = pFriend[1];
						pFriend[1] = pFriend[2];
						pFriend[2] = pFriend[3];
						pFriend[3] = NULL;
					}
					for ( iFriendIndex = 0; iFriendIndex < 4; iFriendIndex++)
					{
						pTarget = pFriend[iFriendIndex];
						if ( !pTarget ) break;
						//	check if the target need that
						switch ( spell )
						{
						case SPELL_Heal:
						case SPELL_Great_Heal:
							if ( pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR)/3 ) bSpellSuits = true;
							break;
						case SPELL_Reactive_Armor:
							if ( pTarget->LayerFind(LAYER_SPELL_Reactive) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Cure:
							if ( pTarget->LayerFind(LAYER_FLAG_Poison) != NULL ) bSpellSuits = true;
							break;
						case SPELL_Protection:
							if ( pTarget->LayerFind(LAYER_SPELL_Protection) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Bless:
							if ( pTarget->LayerFind(LAYER_SPELL_STATS) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Magic_Reflect:
							if ( pTarget->LayerFind(LAYER_SPELL_Magic_Reflect) == NULL ) bSpellSuits = true;
							break;
						}

						if ( bSpellSuits ) break;
					}
					if ( bSpellSuits && Spell_CanCast(spell, true, this, false) )
					{
						pMageryTarget = pTarget;
						m_atMagery.m_Spell = spell;
						break;
					}
					continue;
				}
				else if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
				{
					//	spell is good, but does not harm. the target should obey me. hoping sphere can do this ;)
					switch ( spell )
					{
					case SPELL_Air_Elem:
					case SPELL_Daemon:
					case SPELL_Earth_Elem:
					case SPELL_Fire_Elem:
					case SPELL_Water_Elem:
					case SPELL_Summon_Undead:
						break;
					default:
						continue;
					}
				}

			}
		}
		else
		{
			if ( !pVar && !pSpellDef->IsSpellType( SPELLFLAG_HARM ))
				continue;

			// less chance for berserker spells
			if ( pSpellDef->IsSpellType( SPELLFLAG_SUMMON ) && Calc_GetRandVal( 2 ))
				continue;

			// less chance for field spells as well
			if ( pSpellDef->IsSpellType( SPELLFLAG_FIELD ) && Calc_GetRandVal( 4 ))
				continue;
		}

		if ( ! Spell_CanCast( spell, true, this, false ))
			continue;

		m_atMagery.m_Spell = spell;
		break;	// I like this spell.
	}

	// KRJ - give us some distance
	// if the opponent is using melee
	// the bigger the disadvantage we have in hitpoints, the further we will go
	if ( mana > iStatInt / 3 && Calc_GetRandVal( iStatInt << 1 ))
	{
		if ( iDist < 4 || iDist > 8  )	// Here is fine?
		{
			NPC_Act_Follow( false, 5, true );
		}
	}
	else NPC_Act_Follow();

	Reveal();

	m_Act_Targ = pMageryTarget->GetUID();
	m_Act_TargPrv = GetUID();	// I'm casting this directly.
	m_Act_p = pMageryTarget->GetTopPoint();

	// Calculate the difficulty
	return( Skill_Start( SKILL_MAGERY ));
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

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL || ! pChar->IsTopLevel()) // target is not valid anymore ?
		return;
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
	if ( IsSetEF( EF_New_Triggers ) )
	{
		CScriptTriggerArgs Args( iDist, iMotivation );
		switch ( OnTrigger( CTRIG_NPCActFight, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return;
			case TRIGRET_RET_FALSE:	fSkipHardcoded	= true;	break;
		}
		iDist		= Args.m_iN1;
		iMotivation	= Args.m_iN2;
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
			UpdateDir( pChar );
			Skill_Start( NPCACT_BREATH );
			return;
		}

		// If I am a giant. i can throw stones.
		// NPCACT_THROWING

		if (( GetDispID() == CREID_OGRE ||
			GetDispID() == CREID_ETTIN ||
			GetDispID() == CREID_Cyclops ) &&
			iDist >= 2 &&
			iDist <= 9 &&
			CanSeeLOS( pChar,LOS_NB_WINDOWS ) &&
			ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_AROCK), 0, 2 )) //NPCs can throw stones through a window
		{
			UpdateDir( pChar );
			Skill_Start( NPCACT_THROWING );
			return;
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
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_2 ),
			};
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, sm_szText[ Calc_GetRandVal( COUNTOF( sm_szText )) ], (LPCTSTR) pChar->GetName() );
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
			g_Log.Event( LOGL_WARN, "Guard 0%lx '%s' has no guard post (%s)!\n", (DWORD) GetUID(), (LPCTSTR) GetName(), (LPCTSTR) GetTopPoint().WriteUsed());


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
			if ( !IsSetEF(EF_Minimize_Triggers) )
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
	if ( pItem->IsTimerSet() && ! pItem->IsTimerExpired())
	{
		pMemory->SetTimeout( pItem->GetTimerDiff());	// We'll forget about this once the item is gone
	}
}

bool CChar::NPC_LootContainer( CItemContainer * pContainer )
{
	ADDTOCALLSTACK("CChar::NPC_LootContainer");
	// Go through the pack and see if there is anything I want there...
	CItem * pNext;
	CItem * pLoot = pContainer->GetContentHead();
	for ( ; pLoot != NULL; pLoot = pNext )
	{
		pNext = pLoot->GetNext();
		// Have I checked this one already?
		if ( Memory_FindObj( pLoot ))
			continue;

		if ( pLoot->IsContainer())
		{
			// Loot it as well
			if ( ! NPC_LootContainer( dynamic_cast <CItemContainer *> (pLoot)))
			{
				// Not finished with it
				return false;
			}
		}

		if ( ! NPC_WantThisItem( pLoot ))
			continue;

		// How much can I carry
		if ( CanCarry( pLoot ))
		{
			UpdateAnimate( ANIM_BOW );
			ItemEquip( pLoot );
		}
		else
		{
			// Can't carry the whole stack...how much can I carry?
			NPC_LootMemory( pLoot );
		}

		// I can only pick up one thing at a time, so come back here on my next tick
		SetTimeout( 1 );	// Don't go through things so fast.
		return false;
	}

	// I've gone through everything here...remember that we've looted this container
	NPC_LootMemory( pContainer );
	return true;
}

inline void CChar::NPC_Act_Looting_CantGetItem( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::NPC_Act_Looting_CantGetItem");
	NPC_LootMemory( pItem );
	Skill_Start( SKILL_NONE );
}

void CChar::NPC_Act_Looting()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Looting");
	// NPCACT_LOOTING
	// We have seen something good that we want. checking it out now.
	// We just killed something, so we should see if it has anything interesting on it.
	// Find the corpse first

	if ( !m_pNPC )
		return;

	CItem * pItem = m_Act_Targ.ItemFind();
	if ( pItem == NULL )
	{
		Skill_Start( SKILL_NONE );
		return;
	}

	CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	if ( m_Act_TargPrv != pObjTop->GetUID() ||
		m_Act_p != pObjTop->GetTopPoint())
	{
		// It moved ?
		// give up on this.
		NPC_Act_Looting_CantGetItem( pItem );
		return;
	}

	if ( GetDist( pItem ) > 1 )	// move toward it.
	{
		if ( ++(m_atLooting.m_iDistCurrent) > m_atLooting.m_iDistEstimate )
		{
			NPC_Act_Looting_CantGetItem( pItem );
			return;
		}
		NPC_WalkToPoint();
		return;
	}

	if ( pItem->IsTypeLocked())
	{
		NPC_Act_Looting_CantGetItem( pItem );
		return;
	}

	if ( pItem->IsType(IT_CORPSE))
	{
		// Did I kill this one?
		if ( pItem->m_itCorpse.m_uidKiller != GetUID())
		{
			// Wasn't me...less chance of actually looting it.
			if ( Calc_GetRandVal( 4 ))
			{
				NPC_Act_Looting_CantGetItem( pItem );
				return;
			}
		}
	}

	// Can i reach the object that i want ?
	if ( ! CanTouch( pItem ))
	{
		NPC_Act_Looting_CantGetItem( pItem );
		return;
	}

	// I can reach it
	CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> ( pItem );
	if ( pCorpse )
	{
		if ( ! NPC_LootContainer( pCorpse ))
			return;

		// Eat raw meat ? or just evil ?
		if ( m_pNPC->m_Brain == NPCBRAIN_MONSTER ||
			m_pNPC->m_Brain == NPCBRAIN_ANIMAL ||
			m_pNPC->m_Brain == NPCBRAIN_DRAGON )
		{
			// Only do this if it has a resource type we want...
			if ( pCorpse->m_itCorpse.m_timeDeath.IsTimeValid() )
			{
				Use_CarveCorpse( pCorpse );
				Skill_Start( NPCACT_LOOKING );
				return;
			}
		}
	}
	else
	{
		if ( ! CanCarry( pItem ))
		{
			NPC_Act_Looting_CantGetItem( pItem );
			return;
		}

		// can i eat it on the ground ?

		UpdateAnimate( ANIM_BOW );
		ItemBounce( pItem );
	}

	// Done looting
	// We might be looting this becuase we are hungry...
	// What was I doing before this?

	Skill_Start( SKILL_NONE );
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

void CChar::NPC_Act_Goto()
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
		if ( m_Act_p.IsValidPoint() &&
			IsHuman() &&
			!IsStatFlag( STATF_Freeze|STATF_Stone ))
			Spell_Teleport( m_Act_p, true, false);
		else
			NPC_Act_Idle();	// look for something new to do.
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

	m_pNPC->m_Act_Motivation = ( 50 - ( iFoodLevel/2 ));

	int		iEatAmount = 1;
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
				if ( iEatAmount = Food_CanEat(pFood) )
				{
					Use_EatQty(pFood, iEatAmount);
					return true;
				}
			}
		}
	}

	if ( iFoodLevel <= 1 )
		iSearchDistance = UO_MAP_VIEW_SIGHT;					// 12
	else if ( iFoodLevel <= 5 )
		iSearchDistance = (UO_MAP_VIEW_SIGHT/3) * 2;	// 8
	else if ( iFoodLevel <= 10 )
		iSearchDistance = UO_MAP_VIEW_SIGHT/2;			// 6
	else if ( iFoodLevel <= 15 )
		iSearchDistance = UO_MAP_VIEW_SIGHT/4;			// 3

	//	Search for food nearby
	CWorldSearch AreaItems(GetTopPoint(), minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
	while (true)
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

		if ( ( pItem->GetTopPoint().m_z > (iMyZ + 10) ) || ( pItem->GetTopPoint().m_z < (iMyZ - 1) ) )
		{
#ifdef _NAZDEBUG
			g_Log.EventError("CChar::NPC_Act_Food: '%s' found %s but its Z (%d) is too different from his (%d)\n", GetName(), pItem->GetName(), pItem->GetTopPoint().m_z, iMyZ );
#endif
			continue;
		}
		if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC) )
			continue;

		if ( iEatAmount = Food_CanEat(pItem) )
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
			int iEaten = pClosestFood->ConsumeAmount(iEatAmount);
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
			}
		}
	}
					// no food around, but maybe i am ok with grass? Or shall I try to pick crops?
	else
	{

		NPCBRAIN_TYPE	brain = GetNPCBrain(true);
		if ( brain == NPCBRAIN_ANIMAL )						// animals eat grass always
			bSearchGrass = true;
		//else if (( brain == NPCBRAIN_HUMAN ) && !iFood )	// human eat grass if starving nearly to death
		//	bSearchGrass = true;

		// found any crops or foliage at least (nearby, of course)?
		if ( pCropItem )
		{
			g_Log.EventError("CChar::NPC_Act_Food: '%s' found %s\n", GetName(), pCropItem->GetName() );
			if (GetDist( pCropItem) < 5)
			{
				g_Log.EventError("CChar::NPC_LookAtItem: '%s' is near %s\n", GetName(), pCropItem->GetName() );
				Use_Item( pCropItem );
				bSearchGrass = false;	// no need to eat grass if at next tick we can eat better stuff
			}
		}
	}
	if ( bSearchGrass )
	{
		CCharBase			*pCharDef = Char_GetDef();
		RESOURCE_ID_BASE	rid = RESOURCE_ID(RES_TYPEDEF, IT_GRASS);

		if ( pCharDef->m_FoodType.FindResourceID(rid) >= 0 )	//	do I accept grass as food?
		{
			CItem	*pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				int iEaten = pResBit->ConsumeAmount(10);
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

	if ( g_Cfg.m_iNpcAi&NPC_AI_INTFOOD )
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
		switch ( GetDispID())
		{
			case CREID_FIRE_ELEM:
				if ( !g_World.IsItemTypeNear(GetTopPoint(), IT_FIRE) )
				{
					Action_StartSpecial(CREID_FIRE_ELEM);
					return;
				}
				break;
#ifndef _NAZTEST
			case CREID_GIANT_SPIDER:
				if ( !g_World.IsItemTypeNear(GetTopPoint(), IT_WEB) )
				{
					Action_StartSpecial(CREID_GIANT_SPIDER);
					return;
				}
				break;
#else
			default:
				// TAG.OVERRIDE.SPIDERWEB
				CVarDefCont * pValue = GetKey("OVERRIDE.SPIDERWEB",true);
				if ( pValue )
				{
					DEBUG_ERR(("NPCAI: NPC '%s' has TAG.OVERRIDE.SPIDERWEB set\n", GetName()));
					if ( GetDispID() != CREID_GIANT_SPIDER )
					{
						DEBUG_ERR(("NPCAI: NPC '%s' has TAG.OVERRIDE.SPIDERWEB set and is NOT a Giant Spider\n", GetName()));
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				} else {
					if ( GetDispID() == CREID_GIANT_SPIDER )
					{
						DEBUG_ERR(("NPCAI: NPC '%s' has TAG.OVERRIDE.SPIDERWEB NOT set and IS a Giant Spider\n", GetName()));
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				}
#endif
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
	SetTimeout(TICK_PER_SEC * 1 + Calc_GetRandVal(TICK_PER_SEC*2));
}

bool CChar::NPC_OnItemGive( CChar * pCharSrc, CItem * pItem )
{
	ADDTOCALLSTACK("CChar::NPC_OnItemGive");
	// Someone (Player) is giving me an item. Return true = accept

	if ( !pCharSrc || !m_pNPC )
		return false;

	CScriptTriggerArgs Args(pItem);
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( OnTrigger(CTRIG_ReceiveItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	//	giving item to own pet
	if ( NPC_IsOwnedBy(pCharSrc) )
	{
		if ( pCharSrc->IsPriv(PRIV_GM) )
			return ItemEquip(pItem);

		//	stuff and gold to vendor
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

		//	humanoids
		if ( NPC_CanSpeak() )
		{
			if ( Food_CanEat(pItem) )
			{
				if (( Food_GetLevelPercent() < 100 ) && Use_Eat(pItem) )
				{
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_TY));
					return true;
				}
				else
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_NO));

				return false;
			}

			if ( pItem->IsType(IT_GOLD))
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_THANKS));
			else if ( !CanCarry(pItem) )
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_WEAK));
				return false;
			}

			if ( Use_Item(pItem) )
				return true;

			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_DROP));
			GetPackSafe()->ContentAdd(pItem);
			return true;
		}

		switch ( pItem->GetType() )
		{
		case IT_POTION:
		case IT_DRINK:
		case IT_PITCHER:
		case IT_WATER_WASH:
		case IT_BOOZE:
			if ( Use_Item(pItem) )
				return true;
		}
	}

	// have i paid the npc due to some order?
	if ( pItem->IsType(IT_GOLD) )
	{
		CItemMemory * pMemory = Memory_FindObj(pCharSrc);
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
	}

	//	do i wish to take this item?

	CItemContainer * pPack = NULL;
	if ( !NPC_WantThisItem(pItem) )
	{
		if ( OnTrigger(CTRIG_NPCRefuseItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
		{
			g_Log.EventError("CChar::NPC_OnItemGive: CTRIG_NPCRefuseItem on '%s' returned '1'\n", GetName() );
			pCharSrc->GetClient()->addObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_DONTWANT), this);
			return false;
		} else {
			g_Log.EventError("CChar::NPC_OnItemGive: CTRIG_NPCRefuseItem on '%s' returned ! '1'\n", GetName() );
			g_Log.EventError("CChar::NPC_OnItemGive: Trying to drop %s in  '%s's pack\n", pItem->GetName(), GetName() );
			if ( pPack == NULL )
				pPack = GetPackSafe();
			pPack->ContentAdd( pItem );
			return true;
		}
	}

	//	gold goes to the bankbox or as a financial support to the NPC
	if ( pItem->IsType(IT_GOLD) )
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_BANKER )
		{
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_BANKER_DEPOSIT), pItem->GetAmount());
			Speak(pszMsg);
			pCharSrc->GetBank()->ContentAdd(pItem);
			return true;
		}

		if ( NPC_CanSpeak() )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_THANKS));
		}
		if ( pPack == NULL )
			pPack = GetPackSafe();
		pPack->ContentAdd( pItem );
		return true;
	}

	//	dropping item on vendor means quick sell
	if ( NPC_IsVendor() )
	{
		if ( pCharSrc->IsClient() && !IsStatFlag(STATF_Pet) )
		{
			CEvent	sell;
			sell.VendorSell.m_count = 1;
			sell.VendorSell.m_item[0].m_UID = pItem->GetUID();
			sell.VendorSell.m_item[0].m_amount = pItem->GetAmount();
			pCharSrc->GetClient()->Event_VendorSell(GetUID(), &sell);
		}
		return false;
	}

	// The NPC might want it ?
	g_Log.EventError("CChar::NPC_OnItemGive: '%s' wants %s\n", GetName(), pItem->GetName() );
	switch ( m_pNPC->m_Brain )
	{
	case NPCBRAIN_DRAGON:
	case NPCBRAIN_ANIMAL:
	case NPCBRAIN_MONSTER:
		// Might want food ?
		if ( Food_CanEat(pItem))
		{
			// ??? May know it is poisoned ?
			if ( pItem->m_itFood.m_poison_skill )
			{
				if ( Calc_GetRandVal2(1, pItem->m_itFood.m_poison_skill) < (m_Skill[SKILL_TASTEID] / 10) )
				{
					if ( NPC_CanSpeak() )
					{
						Speak(g_Cfg.GetDefaultMsg(DEFMSG_MURDERER));
					}
					// PC attacks NPC
					return false;
				}
			}
			// ??? May not be hungry
			if ( Use_Eat( pItem, pItem->GetAmount() ))
				return( true );
		}
		break;

	case NPCBRAIN_BEGGAR:
	case NPCBRAIN_THIEF:
		if ( Food_CanEat(pItem) &&
			Use_Eat( pItem ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_FOOD_TY ) );
			return( true );
		}
		if ( ! CanCarry( pItem ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_WEAK ) );
			return( false );
		}
		if ( pItem->IsType(IT_GOLD) || Food_CanEat(pItem))
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_FOOD_TAL ) );
		else
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_SELL ) );

		ItemEquip( pItem );
		pItem->Update();

		if (m_Act_Targ == pCharSrc->GetUID())
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_IFONLY ) );
			m_Act_TargPrv = m_Act_Targ;
			m_Act_Targ.InitUID();
			Skill_Start( SKILL_NONE );
		}
		return( true );
	}

	if ( OnTrigger( CTRIG_NPCAcceptItem, pCharSrc, &Args ) == TRIGRET_RET_TRUE )
	{
		g_Log.EventError("CChar::NPC_OnItemGive: CTRIG_NPCAcceptItem on '%s' returns '1', means: REFUSE\n", GetName() );
		pCharSrc->ItemBounce( pItem );
		pItem->Update();
		return false;
	}

	if ( pPack == NULL )
		pPack = GetPackSafe();
	pPack->ContentAdd( pItem );
	pItem->Update();
	g_Log.EventError("'%s': accepted %s (default action)\n", GetName(), pItem->GetName());
	return( true );
}

bool CChar::NPC_OnTickFood( int nFoodLevel )
{
	ADDTOCALLSTACK("CChar::NPC_OnTickFood");
	// Check Food usage.
	// Are we hungry enough to take some new action ?
	// RETURN: true = we have taken an action.
	if ( !m_pNPC )
		return false;

	bool bMsg = false;
	bool bPet = IsStatFlag(STATF_Pet);
	int maxfood = Stat_GetMax(STAT_FOOD);

	if ( g_Cfg.m_iHitsHungerLoss && maxfood )
	{
		//	hungry at 20% level and lower will decrease hits
		if ( nFoodLevel <= maxfood/5 )
		{
			if ( !m_pArea || m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_ARENA) ) ;
			else if ( m_pNPC && m_pArea->IsFlag(REGION_FLAG_UNDERGROUND|REGION_FLAG_NODECAY) ) ;
			else
			{
				int loss = ( nFoodLevel ? Calc_GetRandVal2(1, g_Cfg.m_iHitsHungerLoss-1) : g_Cfg.m_iHitsHungerLoss);

				UpdateStatVal( STAT_STR, -((maximum(Stat_GetMax(STAT_STR), 10) * loss)/100) );
			}

			char *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_NPC), Food_GetLevelMessage(bPet, false));
			Emote(pszMsg, GetClient());

			bMsg = true;
		}
	}

   	if ( bPet )
   	{
		if ( !bMsg )
		{
			TCHAR *pszMsg = Str_GetTemp();
   			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_FOOD_LVL_NPC), Food_GetLevelMessage(true, false));
   			Emote(pszMsg, GetClient());
		}

		//	pets deserts on being deadly hungry
		if ( nFoodLevel <= 0 )
		{
   			SoundChar(CRESND_RAND2);
			NPC_PetDesert();
			return true;
		}
   	}

	if ( IsStatFlag(STATF_Stone|STATF_Freeze|STATF_DEAD|STATF_Sleeping) )
		return false;
	SoundChar(CRESND_RAND2);
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

	SKILL_TYPE iSkillActive	= Skill_GetActive();
	if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_SCRIPTED ) )
	{
		// SCRIPTED SKILL OnTickAction
	}
	else if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_FIGHT ) )
	{
		EXC_SET("fighting");
		NPC_Act_Fight();
	}
	else
	{
		switch ( iSkillActive )
		{
			case SKILL_NONE:
				// We should try to do something new.
				EXC_SET("idle");
				NPC_Act_Idle();
				break;

			case SKILL_BEGGING:
				EXC_SET("begging");
				NPC_Act_Begging(NULL);
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
				EXC_SET("idle");
				NPC_Act_Idle();
				break;

			case SKILL_ARCHERY:
			case SKILL_FENCING:
			case SKILL_MACEFIGHTING:
			case SKILL_SWORDSMANSHIP:
			case SKILL_WRESTLING:
				// If we are fighting . Periodically review our targets.
				EXC_SET("fight");
				NPC_Act_Fight();
				break;

			case NPCACT_FOLLOW_TARG:
			case NPCACT_GUARD_TARG:
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
					EXC_SET("idle");
					NPC_Act_Idle();	// look for something new to do.
				}
				break;
			case NPCACT_GO_HOME:
				EXC_SET("go home");
				NPC_Act_GoHome();
				break;
			case NPCACT_LOOTING:
				EXC_SET("looting");
				NPC_Act_Looting();
				break;
			case NPCACT_LOOKING:
				EXC_SET("looking");
				if ( NPC_LookAround( true ) )
					break;
				EXC_SET("idle");
				NPC_Act_Idle();
				break;
			case NPCACT_FOOD:
				EXC_SET("Food Skill");
				if ( g_Cfg.m_iNpcAi&NPC_AI_INTFOOD )
				{
					if ( ! NPC_Act_Food() )
						Skill_Start(SKILL_NONE);
				}
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
	if ( IsTimerExpired() )	// Was not reset ?
	{
		int timeout	= maximum((150-Stat_GetAdjusted(STAT_DEX))/2, 0);
		timeout = Calc_GetRandVal2(timeout/2, timeout);
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
	int			iInt = ( g_Cfg.m_iNpcAi & NPC_AI_ALWAYSINT ) ? 300 : Stat_GetAdjusted(STAT_INT);
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
	for ( size_t i = 1; (i != path.LastPathSize()) && (i < 24 /* Don't overflow*/ ); ++i )
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
	int		iEatAmount = 1;
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
				if ( iEatAmount = Food_CanEat(pFood) )
				{
					EXC_SET("eating from pack");
					Use_EatQty(pFood, iEatAmount);
					return;
				}
			}
		}
	}

	if ( iFoodLevel <= 1 ) iSearchDistance = UO_MAP_VIEW_SIGHT;					// 12
	else if ( iFoodLevel <= 5 ) iSearchDistance = (UO_MAP_VIEW_SIGHT/3) * 2;	// 8
	else if ( iFoodLevel <= 10 ) iSearchDistance = UO_MAP_VIEW_SIGHT/2;			// 6
	else if ( iFoodLevel <= 15 ) iSearchDistance = UO_MAP_VIEW_SIGHT/4;			// 3

	//	Search for food nearby
	EXC_SET("searching nearby");
	CWorldSearch	AreaItems(GetTopPoint(), minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
	while (true)
	{
		CItem	*pItem = AreaItems.GetItem();
		if ( !pItem ) break;
		if ( !CanSee(pItem) || pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC) ) continue;
		if ( pItem->GetTopPoint().m_z != iMyZ )
			continue;

		if ( iEatAmount = Food_CanEat(pItem) )
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
			int iEaten = pClosestFood->ConsumeAmount(iEatAmount);
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
						NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
					}
					break;
				}
			}
		}
	}
					// no food around, but maybe i am ok with grass?
	else
	{
		NPCBRAIN_TYPE	brain = GetNPCBrain(true);
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
		if ( pCharDef->m_FoodType.FindResourceID(rid) >= 0 )	//	do I accept grass as a food?
		{
			CItem	*pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				EXC_SET("eating grass");
				int iEaten = pResBit->ConsumeAmount(15);
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
							if (IsSetEF( EF_NewPositionChecks )) //anti-dumbness
								pt = g_World.FindTypeNear_Top(GetTopPoint(), IT_GRASS, minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
							else
								pt = g_World.FindItemTypeNearby(GetTopPoint(), IT_GRASS, minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander), true);
							if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
							{
								if ( CanMoveWalkTo(pt) )
								{
									EXC_SET("walking to grass");
									m_Act_p = pt;
									Skill_Start(NPCACT_GOTO);
									//NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
									return;
								}
							}
							break;
						}
				}
			}
		}
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
}

#define NPCAI_MAX_ITERATIONS	5
#define NPCAI_INT_TOACTMORE		150

void CChar::NPC_AI()
{
	ADDTOCALLSTACK("CChar::NPC_AI");
	EXC_TRY("ExtraAI");

	if ( !m_pNPC )
		return;

	EXC_SET("init");
	CPointMap	pt		= GetTopPoint();
	CSector		*pSector = pt.GetSector();
	int			iInt	= Stat_GetAdjusted(STAT_INT);
	int			iDex	= Stat_GetAdjusted(STAT_DEX);
	bool		bActed;
	int			iter	= 1;
	CREID_TYPE	npcType = NPC_GetAllyGroupType(GetDispID());

	if ( !pSector )
		return;

	//	some very very basic actions which does not need any INT/DEX for self

	//	domestic animals pooping the ground
  CItemMemory * pMemory = Memory_FindTypes( MEMORY_FIGHT );
  if (( m_pNPC->m_Brain == NPCBRAIN_ANIMAL ) && !IsStatFlag ( STATF_Freeze | STATF_Stone | STATF_Insubstantial | STATF_Conjured) && !pMemory)
	{
		if ( Calc_GetRandVal(130) ) ;
		else if (( npcType == CREID_HORSE1 ) || ( npcType == CREID_Bull_Brown ) || ( npcType == CREID_Pig ) || ( npcType == CREID_Llama ))
		{
			EXC_SET("dung pooping");
			Sound( Calc_GetRandVal(2) ? 0xe3 : 0x23f );
			Emote(g_Cfg.GetDefaultMsg(DEFMSG_NPC_ANIMAL_POOP));
			CItem	*pDung = CItem::CreateBase( Calc_GetRandVal(2) ? ITEMID_Dung1 : ITEMID_Dung2 );
			if ( pDung )
				pDung->MoveToDecay(pt, Calc_GetRandVal2(10,30)*TICK_PER_SEC);
			return;
		}
	}

	//	are we active enough to do some actions?
	if ( iDex < Calc_GetRandVal(NPCAI_INT_TOACTMORE) )
		return;

	while ( iter < NPCAI_MAX_ITERATIONS )
	{
		bActed = false;
		iter++;

		switch ( m_pNPC->m_Brain )
		{
			case NPCBRAIN_VENDOR:
			{
				EXC_SET("vendor");

				//	shops are closing at night
				if ( pSector->IsDark() && g_Cfg.m_iNpcAi&NPC_AI_VEND_TIME )
				{
					bool canGoOffDuty = true;
					CWorldSearch Area(GetTopPoint(), GetVisualRange());
					while ( CChar *pChar = Area.GetChar() )
					{
						if ( !pChar->IsClient() )
							continue;

						if ( !CanSee( pChar ) )
							continue;

						canGoOffDuty = false;
						break;
					}
					if ( canGoOffDuty )
					{
						m_pNPC->m_Brain = NPCBRAIN_VENDOR_OFFDUTY;
						bActed = true;
					}
				}
				break;
			} // vendor
			case NPCBRAIN_VENDOR_OFFDUTY:
			{
				EXC_SET("vendor off-duty");

				//	shops are re-opened in daylight
				if ( !pSector->IsDark() )
				{
					//	TODO: when it will be possible to record a number of commands, vendor should
					//	go unlock the door if nearby

					m_pNPC->m_Brain = NPCBRAIN_VENDOR;
					bActed = true;
				}
				break;
			} // vendor off duty
			case NPCBRAIN_THIEF:
			{
				EXC_SET("thief");

				//	thief searches a target to steal something in the packs and ground around
				//	if it is dark, steal and run to home position
				//	TODO:
				break;
			} // thief
			case NPCBRAIN_STABLE:				//	Stable Man
			{
				EXC_SET("stable man");

				CWorldSearch Area(GetTopPoint(), GetVisualRange());
				CChar	*pChar;
				while (( pChar = Area.GetChar() ) && !bActed )
				{
					if ( pChar == this )
						continue;

					if ( !CanSee( pChar ) )
						continue;

					int iDist = GetDist(pChar);

					//	gives some food for player to feed his animals
					if ( pChar->IsClient() )
					{
						if (( iDist < 3 ) && !Calc_GetRandVal(50) )
						{
							RESOURCE_ID food = g_Cfg.ResourceGetIDType(RES_ITEMDEF, "RANDOM_VEGGIE");
							CItem	*pItem = CItem::CreateScript((ITEMID_TYPE)food.GetResIndex());
							if ( pItem )
							{
								UpdateDir(pChar);
								pItem->SetAmount(Calc_GetRandVal2(3, 10));
								pChar->ItemBounce(pItem);
								char	*z = Str_GetTemp();
								sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_STABLEMASTER_FOOD), pItem->GetName(), pChar->GetName());
								Speak(z);
								bActed = true;
							}
						}
					}

					if (( !pChar->m_pNPC ) ||
						( pChar->m_pNPC->m_Brain != NPCBRAIN_ANIMAL ) ||
						( pChar->IsStatFlag(STATF_Stone|STATF_Hidden|STATF_Conjured|STATF_Ridden) ))
						continue;

					//	feeds animals nearby
					int iMaxFood = pChar->Stat_GetMax(STAT_FOOD);
					int iFood = pChar->Stat_GetVal(STAT_FOOD);
					if ( iMaxFood && ( iFood < iMaxFood/4 ))
					{
						if ( iDist > 1 )
						{
							UpdateDir(pChar);
							UpdateAnimate(ANIM_FIDGET_YAWN, false);
							pChar->UpdateAnimate(ANIM_ANI_EAT, false);
							pChar->Stat_SetVal(STAT_FOOD, iFood + Calc_GetRandVal2(1, iMaxFood - iFood));
							char *z = Str_GetTemp();
							sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_STABLEMASTER_FEED), pChar->GetName());
							Emote(z);
						}
						else
						{
							CPointMap	pt = pChar->GetTopPoint();
							if ( CanMoveWalkTo(pt) )
							{
								m_Act_p = pt;
								Skill_Start(NPCACT_GOTO);
								NPC_WalkToPoint(iDist > 3);
							}
						}
						bActed = true;
					}
				}
				break;
			} // stabler
			case NPCBRAIN_BEGGAR:
			{
				EXC_SET("beggar");

				//	TODO: should go sleeping if darktime
				break;
			} // beggar
			case NPCBRAIN_HEALER:
			{
				EXC_SET("healer");

				//	healers searches for chars nearby to heal them. if can heal and have
				//	bandages, go to him and apply bandages on him. these are converted to
				//	bloody bandages
				//	TODO:

				//	healers searches for items nearby to gather bandages/bloody bandages,
				//	water to clean the bandages
				//	TODO:
				break;
			} // healer
			case NPCBRAIN_UNDEAD:
			{
				EXC_SET("undead");

				//	TODO: if active combat in progress there should a chance to summon some char
				break;
			} // undead
		} // switch brain

		if ( bActed ) ;
		else if ( GetNPCBrain(true) == NPCBRAIN_HUMAN )
		{
			//	equip/unequip lightsource if not equipped
			EXC_SET("light source");
			if ( bActed ) ;
			else if ( IsStatFlag(STATF_War) ) ;
			else if ( pSector->IsDark() && !IsStatFlag(STATF_NightSight) )
			{

				CItem *pLightSourceCheck = LayerFind(LAYER_HAND2);
				if ( !( pLightSourceCheck && ( pLightSourceCheck->IsType(IT_LIGHT_OUT) || pLightSourceCheck->IsType(IT_LIGHT_LIT) )))
				{
					CItem *pLightSource = ContentFind(RESOURCE_ID(RES_TYPEDEF,IT_LIGHT_OUT));
					if ( pLightSource )
					{
						if ( ItemEquip(pLightSource) && Use_Obj(pLightSource, false, true) )
							bActed = true;
					}
				}
			}
			else
			{
				CItem *pLightSource = LayerFind(LAYER_HAND2);
				if ( pLightSource && ( pLightSource->IsType(IT_LIGHT_OUT) || pLightSource->IsType(IT_LIGHT_LIT) ))
				{
					if ( ItemBounce(pLightSource) )
						bActed = true;
				}
			}

			//	equip weapons if possible
			EXC_SET("weapon/shield");
			if ( bActed ) ;
			else if ( IsStatFlag(STATF_War) )
			{
				CItem *pWeapon = LayerFind(LAYER_HAND1);
				if ( !pWeapon || !pWeapon->IsTypeWeapon() )
				{
					pWeapon = LayerFind(LAYER_HAND2);
					if ( !pWeapon || !pWeapon->IsTypeWeapon() )
					{
						CItem *pItem = GetPackSafe()->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_SHIELD));
						if ( pItem )
							ItemEquip(pItem);
						ItemEquipWeapon(false);
					}
				}
			}

			//	all humans having little of food level should take some time to visit a
			//	dinning place, returning back after some time. the dinning place detection
			//	should take place in mapcache routines somehow, here to act only.
			EXC_SET("human generic");
			//	TODO:
		}

		//	clever creatures are able to make several actions during one tick
		if (( iInt < NPCAI_INT_TOACTMORE ) || ( Calc_GetRandVal(iInt) < NPCAI_INT_TOACTMORE ))
			break;
	}

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
}
