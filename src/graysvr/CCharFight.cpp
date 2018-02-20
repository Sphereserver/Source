// Fight/Criminal actions/Noto.
#include "graysvr.h"	// predef header.
#include "CClient.h"
#include "../network/send.h"

// Get my guild stone for my guild. even if i'm just a STONEPRIV_CANDIDATE ?
// ARGS:
//  MemType == MEMORY_GUILD or MEMORY_TOWN
CItemStone * CChar::Guild_Find( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_Find");
	if ( ! m_pPlayer )
		return( NULL );
	CItemMemory * pMyGMem = Memory_FindTypes(static_cast<WORD>(MemType));
	if ( ! pMyGMem )
		return( NULL );
	CItemStone * pMyStone = static_cast<CItemStone *>(pMyGMem->m_uidLink.ItemFind());
	if ( pMyStone == NULL )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes(static_cast<WORD>(MemType)); 	// Make them forget they were ever in this guild....again!
		return( NULL );
	}
	return( pMyStone );
}

// Get my member record for my guild.
CStoneMember * CChar::Guild_FindMember( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_FindMember");
	CItemStone * pMyStone = Guild_Find(MemType);
	if ( pMyStone == NULL )
		return( NULL );
	CStoneMember * pMember = pMyStone->GetMember( this );
	if ( pMember == NULL )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes(static_cast<WORD>(MemType)); 	// Make them forget they were ever in this guild....again!
		return( NULL );
	}
	return( pMember );
}

// Resign me from my guild
void CChar::Guild_Resign( MEMORY_TYPE MemType )
{
	ADDTOCALLSTACK("CChar::Guild_Resign");
	CStoneMember * pMember = Guild_FindMember(MemType);
	if ( pMember )
		delete pMember;
}

// Get my guild abbrev if i have chosen to turn it on.
LPCTSTR CChar::Guild_Abbrev( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_Abbrev");
	CStoneMember * pMember = Guild_FindMember(MemType);
	if ( pMember == NULL )
		return( NULL );
	if ( ! pMember->IsAbbrevOn())
		return( NULL );
	CItemStone * pMyStone = pMember->GetParentStone();
	if ( pMyStone == NULL ||
		! pMyStone->GetAbbrev()[0] )
		return( NULL );
	return( pMyStone->GetAbbrev());
}

// Get my [guild abbrev] if i have chosen to turn it on.
LPCTSTR CChar::Guild_AbbrevBracket( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_AbbrevBracket");
	LPCTSTR pszAbbrev = Guild_Abbrev(MemType);
	if ( pszAbbrev == NULL )
		return( NULL );
	TCHAR * pszTemp = Str_GetTemp();
	sprintf( pszTemp, " [%s]", pszAbbrev );
	return( pszTemp );
}

//*****************************************************************

// I'm a murderer?
bool CChar::Noto_IsMurderer() const
{
	ADDTOCALLSTACK("CChar::Noto_IsMurderer");
	return (m_pPlayer && (m_pPlayer->m_wMurders > g_Cfg.m_iMurderMinCount));
}

// I'm evil?
bool CChar::Noto_IsEvil() const
{
	ADDTOCALLSTACK("CChar::Noto_IsEvil");
	int iKarma = Stat_GetAdjusted(STAT_KARMA);

	//	guarded areas could be both RED and BLUE ones.
	if ( m_pArea && m_pArea->IsGuarded() && m_pArea->m_TagDefs.GetKeyNum("RED") )
	{
		//	red zone is opposite to blue - murders are considered normal here
		//	while people with 0 kills and good karma are considered bad here
		if ( Noto_IsMurderer() )
			return false;

		if ( m_pPlayer )
			return (iKarma > g_Cfg.m_iPlayerKarmaEvil);

		return (iKarma > 0);
	}

	if ( Noto_IsMurderer() )
		return true;

	if ( m_pPlayer )
		return (iKarma < g_Cfg.m_iPlayerKarmaEvil);

	switch ( GetNPCBrain(false) )
	{
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_DRAGON:
			return (iKarma < 0);
		case NPCBRAIN_BERSERK:
			return true;
		case NPCBRAIN_ANIMAL:
			return (iKarma <= -800);
		default:
			return (iKarma < -3000);
	}
}

bool CChar::Noto_IsNeutral() const
{
	ADDTOCALLSTACK("CChar::Noto_IsNeutral");
	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	if ( m_pPlayer )
		return (iKarma < g_Cfg.m_iPlayerKarmaNeutral);

	switch ( GetNPCBrain(false) )
	{
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_DRAGON:
		case NPCBRAIN_BERSERK:
			return (iKarma <= 0);
		case NPCBRAIN_ANIMAL:
			return (iKarma <= 100);
		default:
			return (iKarma < 0);
	}
}

NOTO_TYPE CChar::Noto_GetFlag(const CChar *pCharViewer, bool bAllowInvul, bool bGetColor) const
{
	ADDTOCALLSTACK("CChar::Noto_GetFlag");
	if ( !pCharViewer )
		return NOTO_INVALID;

	CChar *pTarget = const_cast<CChar *>(pCharViewer);
	CChar *pSrc = const_cast<CChar *>(this);
	if ( g_Cfg.m_iPetsInheritNotoriety && m_pNPC && NPC_PetGetOwner() )	// If I'm a pet and have owner I redirect noto to him.
		pSrc = NPC_PetGetOwner();

	if ( pSrc->m_notoSaves.size() )
	{
		int id = pSrc->NotoSave_GetID(pTarget);
		if ( id != -1 )
		{
			NotoSaves refNoto = m_notoSaves.at(id);
			return bGetColor ? refNoto.color : refNoto.value;
		}
	}

	NOTO_TYPE iNoto = NOTO_INVALID;
	NOTO_TYPE iColor = NOTO_INVALID;

	if ( IsTrigUsed(TRIGGER_NOTOSEND) )
	{
		CScriptTriggerArgs args;
		pSrc->OnTrigger(CTRIG_NotoSend, pTarget, &args);
		iNoto = static_cast<NOTO_TYPE>(args.m_iN1);
		iColor = static_cast<NOTO_TYPE>(args.m_iN2);
		if ( iNoto < NOTO_INVALID )
			iNoto = NOTO_INVALID;
	}

	if ( iNoto == NOTO_INVALID )
		iNoto = Noto_CalcFlag(pTarget, bAllowInvul);
	if ( iColor == NOTO_INVALID )
		iColor = iNoto;
	pSrc->NotoSave_Add(pTarget, iNoto, iColor);

	return bGetColor ? iColor : iNoto;
}

NOTO_TYPE CChar::Noto_CalcFlag(const CChar *pCharViewer, bool bAllowInvul) const
{
	ADDTOCALLSTACK("CChar::Noto_CalcFlag");
	if ( !pCharViewer )
		return NOTO_INVALID;

	NOTO_TYPE NotoFlag = static_cast<NOTO_TYPE>(m_TagDefs.GetKeyNum("OVERRIDE.NOTO"));
	if ( NotoFlag != NOTO_INVALID )
		return NotoFlag;

	if ( IsStatFlag(STATF_Incognito) )
		return NOTO_NEUTRAL;

	if ( bAllowInvul && IsStatFlag(STATF_INVUL) )
		return NOTO_INVUL;

	if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_ARENA) )	// everyone is neutral here
		return NOTO_NEUTRAL;

	if ( pCharViewer != this )
	{
		// Do we have a master to inherit notoriety from?
		if ( g_Cfg.m_iPetsInheritNotoriety != 0 )
		{
			CChar *pMaster = NPC_PetGetOwner();
			if ( pMaster && pMaster != pCharViewer ) // master doesn't want to see their own status
			{
				// Protect against infinite loop
				static int sm_iReentrant = 0;
				if ( sm_iReentrant < 32 )
				{
					// Get master's notoriety
					++sm_iReentrant;
					NOTO_TYPE NotoMaster = pMaster->Noto_GetFlag(pCharViewer, bAllowInvul);
					--sm_iReentrant;

					// Check if this notoriety is inheritable based on bitmask settings
					int iNotoFlags = 1 << (NotoMaster - 1);
					if ( (g_Cfg.m_iPetsInheritNotoriety & iNotoFlags) == iNotoFlags )
						return NotoMaster;
				}
				else
				{
					// Too many owners, return the notoriety for however far we got down the chain
					DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring notoriety towards %s uid=0%lx\n", pMaster->GetName(), pMaster->GetUID().GetPrivateUID()));
				}
			}
		}

		if ( m_pParty && (m_pParty == pCharViewer->m_pParty) )
		{
			if ( m_pParty->GetLootFlag(this) )
				return NOTO_GUILD_SAME;
		}
	}

	if ( Noto_IsEvil() )
		return NOTO_EVIL;

	if ( pCharViewer != this )
	{
		if ( m_pPlayer )
		{
			// Check the guild stuff
			CItemStone *pMyGuild = Guild_Find(MEMORY_GUILD);
			if ( pMyGuild )
			{
				CItemStone *pViewerGuild = pCharViewer->Guild_Find(MEMORY_GUILD);
				if ( pViewerGuild )
				{
					if ( (pViewerGuild == pMyGuild) || pMyGuild->IsAlliedWith(pViewerGuild) )
						return NOTO_GUILD_SAME;
					if ( pMyGuild->IsAtWarWith(pViewerGuild) )
						return NOTO_GUILD_WAR;
				}
			}

			// Check the town stuff
			CItemStone *pMyTown = Guild_Find(MEMORY_TOWN);
			if ( pMyTown )
			{
				CItemStone *pViewerTown = pCharViewer->Guild_Find(MEMORY_TOWN);
				if ( pViewerTown )
				{
					if ( pMyTown->IsAtWarWith(pViewerTown) )
						return NOTO_GUILD_WAR;
				}
			}
		}

		// If they saw me commit a crime or I am their aggressor then criminal to just them
		CItemMemory *pMemory = pCharViewer->Memory_FindObjTypes(this, MEMORY_SAWCRIME|MEMORY_AGGREIVED|MEMORY_HARMEDBY);
		if ( pMemory )
			return NOTO_CRIMINAL;
	}

	if ( IsStatFlag(STATF_Criminal) )	// criminal to everyone.
		return NOTO_CRIMINAL;

	if ( Noto_IsNeutral() )
		return NOTO_NEUTRAL;

	return NOTO_GOOD;
}

HUE_TYPE CChar::Noto_GetHue(const CChar *pCharViewer) const
{
	ADDTOCALLSTACK("CChar::Noto_GetHue");
	CVarDefCont *sVal = GetKey("NAME.HUE", true);
	if ( sVal )
		return static_cast<HUE_TYPE>(sVal->GetValNum());

	NOTO_TYPE Noto = Noto_GetFlag(pCharViewer, true, true);
	switch ( Noto )
	{
		case NOTO_GOOD:			return g_Cfg.m_iColorNotoGood;		// Blue
		case NOTO_GUILD_SAME:	return g_Cfg.m_iColorNotoGuildSame;	// Green (same guild)
		case NOTO_NEUTRAL:		return g_Cfg.m_iColorNotoNeutral;	// Grey (someone that can be attacked)
		case NOTO_CRIMINAL:		return g_Cfg.m_iColorNotoCriminal;	// Grey (criminal)
		case NOTO_GUILD_WAR:	return g_Cfg.m_iColorNotoGuildWar;	// Orange (enemy guild)
		case NOTO_EVIL:			return g_Cfg.m_iColorNotoEvil;		// Red
		case NOTO_INVUL:		return IsPriv(PRIV_GM) ? g_Cfg.m_iColorNotoInvulGameMaster : g_Cfg.m_iColorNotoInvul;	// Purple / Yellow
		default:				return (Noto > NOTO_INVUL) ? static_cast<HUE_TYPE>(Noto) : g_Cfg.m_iColorNotoDefault;	// Grey
	}
}


LPCTSTR CChar::Noto_GetFameTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetFameTitle");
	if ( IsStatFlag(STATF_Incognito|STATF_Polymorph) )
		return "";

	if ( !IsPriv(PRIV_PRIV_NOSHOW) )	// PRIVSHOW is on
	{
		if ( IsPriv(PRIV_GM) )			// GM mode is on
		{
			switch ( GetPrivLevel() )
			{
				case PLEVEL_Owner:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_OWNER );	//"Owner ";
				case PLEVEL_Admin:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ADMIN );	//"Admin ";
				case PLEVEL_Dev:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_DEV );	//"Dev ";
				case PLEVEL_GM:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_GM );	//"GM ";
			}
		}
		switch ( GetPrivLevel() )
		{
			case PLEVEL_Seer:
				return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_SEER );	//"Seer ";
			case PLEVEL_Counsel:
				return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_COUNSEL );	//"Counselor ";
		}
	}

	if ( (m_pPlayer || !g_Cfg.m_NPCNoFameTitle) && (Stat_GetAdjusted(STAT_FAME) >= 10000) )
		return g_Cfg.GetDefaultMsg(Char_GetDef()->IsFemale() ? DEFMSG_TITLE_LADY : DEFMSG_TITLE_LORD);	//"Lady " : "Lord "

	return "";
}

int CChar::Noto_GetLevel() const
{
	ADDTOCALLSTACK("CChar::Noto_GetLevel");

	size_t i = 0;
	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	for ( ; i < g_Cfg.m_NotoKarmaLevels.GetCount() && iKarma < g_Cfg.m_NotoKarmaLevels.GetAt(i); i++ )
		;

	size_t j = 0;
	int iFame = Stat_GetAdjusted(STAT_FAME);
	for ( ; j < g_Cfg.m_NotoFameLevels.GetCount() && iFame > g_Cfg.m_NotoFameLevels.GetAt(j); j++ )
		;

	return( ( i * (g_Cfg.m_NotoFameLevels.GetCount() + 1) ) + j );
}

LPCTSTR CChar::Noto_GetTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetTitle");

	LPCTSTR pTitle = Noto_IsMurderer() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_MURDERER ) : ( IsStatFlag(STATF_Criminal) ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_CRIMINAL ) :  g_Cfg.GetNotoTitle(Noto_GetLevel(), Char_GetDef()->IsFemale()) );
	LPCTSTR pFameTitle = GetKeyStr("NAME.PREFIX");
	if ( !*pFameTitle )
		pFameTitle = Noto_GetFameTitle();

	TCHAR * pTemp = Str_GetTemp();
	sprintf( pTemp, "%s%s%s%s%s%s",
		(pTitle[0]) ? ( Char_GetDef()->IsFemale() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ARTICLE_FEMALE ) : g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ARTICLE_MALE ) )  : "",
		pTitle,
		(pTitle[0]) ? " " : "",
		pFameTitle,
		GetName(),
		GetKeyStr("NAME.SUFFIX"));

	return pTemp;
}

void CChar::Noto_Murder()
{
	ADDTOCALLSTACK("CChar::Noto_Murder");
	if ( Noto_IsMurderer() )
		SysMessageDefault(DEFMSG_MSG_MURDERER);

	if ( m_pPlayer && m_pPlayer->m_wMurders )
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Murders, 0, g_Cfg.m_iMurderDecayTime, NULL);
}

bool CChar::Noto_Criminal( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::Noto_Criminal");
	if ( m_pNPC || IsPriv(PRIV_GM) )
		return false;

	int decay = g_Cfg.m_iCriminalTimer;

	if ( IsTrigUsed(TRIGGER_CRIMINAL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = decay;
		Args.m_pO1 = pChar;
		if ( OnTrigger(CTRIG_Criminal, this, &Args) == TRIGRET_RET_TRUE )
			return false;

		decay = static_cast<int>(Args.m_iN1);
	}

	if ( !IsStatFlag(STATF_Criminal) )
		SysMessageDefault(DEFMSG_MSG_GUARDS);

	Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Criminal, 0, decay);
	return true;
}

void CChar::Noto_ChangeDeltaMsg( int iDelta, LPCTSTR pszType )
{
	ADDTOCALLSTACK("CChar::Noto_ChangeDeltaMsg");
	if ( !iDelta )
		return;

#define	NOTO_DEGREES	8
#define	NOTO_FACTOR		(300/NOTO_DEGREES)

	static UINT const sm_DegreeTable[8] =
	{
		DEFMSG_MSG_NOTO_CHANGE_1,
		DEFMSG_MSG_NOTO_CHANGE_2,
		DEFMSG_MSG_NOTO_CHANGE_3,
		DEFMSG_MSG_NOTO_CHANGE_4,
		DEFMSG_MSG_NOTO_CHANGE_5,
		DEFMSG_MSG_NOTO_CHANGE_6,
		DEFMSG_MSG_NOTO_CHANGE_7,
		DEFMSG_MSG_NOTO_CHANGE_8		// 300 = huge
	};

	int iDegree = minimum(abs(iDelta) / NOTO_FACTOR, 7);

	TCHAR *pszMsg = Str_GetTemp();
	sprintf( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_0 ), 
		( iDelta < 0 ) ? g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_LOST ) : g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_GAIN ),
		 g_Cfg.GetDefaultMsg(sm_DegreeTable[iDegree]), pszType );
	
	SysMessage( pszMsg );
}

void CChar::Noto_ChangeNewMsg( int iPrvLevel )
{
	ADDTOCALLSTACK("CChar::Noto_ChangeNewMsg");
	if ( iPrvLevel != Noto_GetLevel() )		// reached a new title level ?
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_GETTITLE), Noto_GetTitle());
}

void CChar::Noto_Fame( int iFameChange )
{
	ADDTOCALLSTACK("CChar::Noto_Fame");

	if ( ! iFameChange )
		return;

	int iFame = maximum(Stat_GetAdjusted(STAT_FAME), 0);
	if ( iFameChange > 0 )
	{
		if ( iFame + iFameChange > g_Cfg.m_iMaxFame )
			iFameChange = g_Cfg.m_iMaxFame - iFame;
	}
	else
	{
		if ( iFame + iFameChange < 0 )
			iFameChange = -iFame;
	}

	if ( IsTrigUsed(TRIGGER_FAMECHANGE) )
	{
		CScriptTriggerArgs Args(iFameChange);	// ARGN1 - Fame change modifier
		TRIGRET_TYPE retType = OnTrigger(CTRIG_FameChange, this, &Args);

		if ( retType == TRIGRET_RET_TRUE )
			return;
		iFameChange = static_cast<int>(Args.m_iN1);
	}

	if ( ! iFameChange )
		return;

	iFame += iFameChange;
	Noto_ChangeDeltaMsg( iFame - Stat_GetAdjusted(STAT_FAME), g_Cfg.GetDefaultMsg( DEFMSG_NOTO_FAME ) );
	Stat_SetBase(STAT_FAME, iFame);
}

void CChar::Noto_Karma( int iKarmaChange, int iBottom, bool bMessage )
{
	ADDTOCALLSTACK("CChar::Noto_Karma");

	int	iKarma = Stat_GetAdjusted(STAT_KARMA);
	iKarmaChange = g_Cfg.Calc_KarmaScale( iKarma, iKarmaChange );

	if ( iKarmaChange > 0 )
	{
		if ( iKarma + iKarmaChange > g_Cfg.m_iMaxKarma )
			iKarmaChange = g_Cfg.m_iMaxKarma - iKarma;
	}
	else
	{
		if (iBottom == INT_MIN)
			iBottom = g_Cfg.m_iMinKarma;
		if ( iKarma + iKarmaChange < iBottom )
			iKarmaChange = iBottom - iKarma;
	}

	if ( IsTrigUsed(TRIGGER_KARMACHANGE) )
	{
		CScriptTriggerArgs Args(iKarmaChange);	// ARGN1 - Karma change modifier
		TRIGRET_TYPE retType = OnTrigger(CTRIG_KarmaChange, this, &Args);

		if ( retType == TRIGRET_RET_TRUE )
			return;
		iKarmaChange = static_cast<int>(Args.m_iN1);
	}

	if ( ! iKarmaChange )
		return;

	iKarma += iKarmaChange;
	Noto_ChangeDeltaMsg( iKarma - Stat_GetAdjusted(STAT_KARMA), g_Cfg.GetDefaultMsg( DEFMSG_NOTO_KARMA ) );
	Stat_SetBase(STAT_KARMA, iKarma);
	NotoSave_Update();
	if ( bMessage )
	{
		int iPrvLevel = Noto_GetLevel();
		Noto_ChangeNewMsg( iPrvLevel );
	}
}

extern unsigned int Calc_ExpGet_Exp(unsigned int);

void CChar::Noto_Kill(CChar * pKill, bool fPetKill, int iTotalKillers)
{
	ADDTOCALLSTACK("CChar::Noto_Kill");

	if ( !pKill )
		return;

	// What was there noto to me ?
	NOTO_TYPE NotoThem = pKill->Noto_GetFlag(this);

	// Fight is over now that i have won. (if i was fighting at all )
	// ie. Magery cast might not be a "fight"
	Attacker_Delete(pKill);
	if ( pKill == this )
		return;

	if ( m_pNPC )
	{
		if ( pKill->m_pNPC )
		{
			if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )	// don't create corpse if NPC got killed by a guard
				pKill->StatFlag_Set(STATF_Conjured);
			return;
		}
	}
	else if ( NotoThem < NOTO_GUILD_SAME )
	{
		ASSERT(m_pPlayer);
		// I'm a murderer !
		if ( !IsPriv(PRIV_GM) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = m_pPlayer->m_wMurders+1;
			args.m_iN2 = true;

			if ( IsTrigUsed(TRIGGER_MURDERMARK) )
			{
				OnTrigger(CTRIG_MurderMark, this, &args);
				if ( args.m_iN1 < 0 )
					args.m_iN1 = 0;
			}

			m_pPlayer->m_wMurders = static_cast<WORD>(args.m_iN1);
			NotoSave_Update();
			if ( args.m_iN2 )
				Noto_Criminal();

			Noto_Murder();
		}
	}

	// No fame/karma/exp gain on these conditions
	if ( fPetKill || NotoThem == NOTO_GUILD_SAME || pKill->IsStatFlag(STATF_Conjured) || (pKill->m_pNPC && pKill->m_pNPC->m_bonded) )
		return;

	int iPrvLevel = Noto_GetLevel();	// store title before fame/karma changes to check if it got changed
	Noto_Fame(g_Cfg.Calc_FameKill(pKill) / iTotalKillers);
	Noto_Karma(g_Cfg.Calc_KarmaKill(pKill, NotoThem) / iTotalKillers);

	if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_RAISE_COMBAT) )
	{
		int change = (pKill->m_exp / 10) / iTotalKillers;
		if ( change )
		{
			if ( m_pPlayer && pKill->m_pPlayer )
				change = (change * g_Cfg.m_iExperienceKoefPVP) / 100;
			else
				change = (change * g_Cfg.m_iExperienceKoefPVM) / 100;
		}

		if ( change )
		{
			//	bonuses of different experiences
			if ( (m_exp * 4) < pKill->m_exp )		// 200%		[exp < 1/4 of killed]
				change *= 2;
			else if ( (m_exp * 2) < pKill->m_exp )	// 150%		[exp < 1/2 of killed]
				change = (change * 3) / 2;
			else if ( m_exp <= pKill->m_exp )		// 100%		[exp <= killed]
				;
			else if ( m_exp < (pKill->m_exp * 2) )	//  50%		[exp < 2 * killed]
				change /= 2;
			else if ( m_exp < (pKill->m_exp * 3) )	//  25%		[exp < 3 * killed]
				change /= 4;
			else									//  10%		[exp >= 3 * killed]
				change /= 10;
		}

		ChangeExperience(change, pKill);
	}

	Noto_ChangeNewMsg(iPrvLevel);	// inform any title changes
}

void CChar::NotoSave_Add(CChar *pChar, NOTO_TYPE value, NOTO_TYPE color)
{
	ADDTOCALLSTACK("CChar::NotoSave_Add");
	if ( !pChar )
		return;
	if ( m_notoSaves.size() )
	{
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves &refNoto = *it;
			if ( refNoto.charUID == pChar->GetUID() )
			{
				refNoto.elapsed = 0;
				refNoto.value = value;
				refNoto.color = color;
				return;
			}
		}
	}
	NotoSaves refNew;
	refNew.charUID = pChar->GetUID();
	refNew.elapsed = 0;
	refNew.value = value;
	refNew.color = color;
	m_notoSaves.push_back(refNew);
}

void CChar::NotoSave_Delete(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NotoSave_Delete");
	if ( !pChar )
		return;
	if ( m_notoSaves.size() )
	{
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves &refNoto = *it;
			if ( refNoto.charUID == pChar->GetUID() )
			{
				m_notoSaves.erase(it);
				return;
			}
		}
	}
}

void CChar::NotoSave_Clear()
{
	ADDTOCALLSTACK("CChar::NotoSave_Clear");
	if ( m_notoSaves.size() )
		m_notoSaves.clear();
}

void CChar::NotoSave_Update()
{
	ADDTOCALLSTACK("CChar::NotoSave_Update");
	NotoSave_Clear();
	UpdateMode(NULL, true);
}

int CChar::NotoSave_GetID(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NotoSave_GetID");
	if ( !pChar )
		return -1;
	if ( m_notoSaves.size() )
	{
		int i = 0;
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves &refNoto = *it;
			if ( refNoto.charUID == pChar->GetUID() )
				return i;
			i++;
		}
	}
	return -1;
}

void CChar::NotoSave_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::NotoSave_CheckTimeout");
	if ( m_notoSaves.size() )
	{
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves &refNoto = *it;
			if ( ++refNoto.elapsed > g_Cfg.m_iNotoTimeout )
			{
				m_notoSaves.erase(it);
				return;
			}
		}
	}
}

//***************************************************************
// Memory this char has about something in the world.

// Reset the check timer based on the type of memory.
bool CChar::Memory_UpdateFlags( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_UpdateFlags");

	ASSERT(pMemory);
	ASSERT(pMemory->IsType(IT_EQ_MEMORY_OBJ));

	WORD wMemTypes = pMemory->GetMemoryTypes();

	if ( !wMemTypes )	// No memories here anymore so kill it.
		return false;

	INT64 iCheckTime;
	if ( wMemTypes & MEMORY_IPET )
		StatFlag_Set( STATF_Pet );
	if ( wMemTypes & MEMORY_FIGHT )	// update more often to check for retreat.
		iCheckTime = 30*TICK_PER_SEC;
	else if ( wMemTypes & ( MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN ))
		iCheckTime = -1;	// never go away.
	else if ( m_pNPC )	// MEMORY_SPEAK
		iCheckTime = 5*60*TICK_PER_SEC;
	else
		iCheckTime = 20*60*TICK_PER_SEC;
	pMemory->SetTimeout( iCheckTime );	// update it's decay time.	
	CChar * pCharLink = pMemory->m_uidLink.CharFind();
	if (pCharLink)
	{
		pCharLink->NotoSave_Update();	// Clear my notoriety from the target.
		NotoSave_Update();		// iAggressor is stored in the other char, so the call should be reverted.
	}
	return true;
}

// Just clear these flags but do not delete the memory.
// RETURN: true = still useful memory.
bool CChar::Memory_UpdateClearTypes( CItemMemory * pMemory, WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_UpdateClearTypes");
	ASSERT(pMemory);

	WORD wPrvMemTypes = pMemory->GetMemoryTypes();
	bool fMore = ( pMemory->SetMemoryTypes( wPrvMemTypes &~ MemTypes ) != 0);

	MemTypes &= wPrvMemTypes;	// Which actually got turned off ?

	if ( MemTypes & MEMORY_IPET )
	{
		// Am i still a pet of some sort ?
		if ( Memory_FindTypes( MEMORY_IPET ) == NULL )
			StatFlag_Clear( STATF_Pet );
	}

	return fMore && Memory_UpdateFlags( pMemory );
}

// Adding a new flag to the given pMemory
void CChar::Memory_AddTypes( CItemMemory * pMemory, WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_AddTypes");
	if ( pMemory )
	{
		pMemory->SetMemoryTypes( pMemory->GetMemoryTypes() | MemTypes );
		pMemory->m_itEqMemory.m_pt = GetTopPoint();	// Where did the fight start ?
		pMemory->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());
		Memory_UpdateFlags( pMemory );
	}
}

// Clear the memory object of this type.
bool CChar::Memory_ClearTypes( CItemMemory * pMemory, WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	if ( pMemory )
	{
		if ( Memory_UpdateClearTypes( pMemory, MemTypes ))
			return true;
		pMemory->Delete();
	}
	return false;
}

// Create a memory about this object.
// NOTE: Does not check if object already has a memory.!!!
//  Assume it does not !
CItemMemory * CChar::Memory_CreateObj( CGrayUID uid, WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_CreateObj");

	CItemMemory * pMemory = static_cast<CItemMemory *>(CItem::CreateBase( ITEMID_MEMORY ));
	if ( pMemory == NULL )
		return NULL;

	pMemory->SetType(IT_EQ_MEMORY_OBJ);
	pMemory->SetAttr(ATTR_NEWBIE);
	pMemory->m_uidLink = uid;

	Memory_AddTypes( pMemory, MemTypes );
	LayerAdd( pMemory, LAYER_SPECIAL );
	return pMemory;
}

// Remove all the memories of this type.
void CChar::Memory_ClearTypes( WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		CItemMemory * pMemory = static_cast<CItemMemory *>(pItem);
		if ( !pMemory )
			continue;
		Memory_ClearTypes(pMemory, MemTypes);
	}
}

// Do I have a memory / link for this object ?
CItemMemory * CChar::Memory_FindObj( CGrayUID uid ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindObj");
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) )
			continue;
		if ( pItem->m_uidLink != uid )
			continue;
		return static_cast<CItemMemory *>(pItem);
	}
	return NULL;
}

// Do we have a certain type of memory.
// Just find the first one.
CItemMemory * CChar::Memory_FindTypes( WORD MemTypes ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindTypes");
	if ( !MemTypes )
		return NULL;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		return static_cast<CItemMemory *>(pItem);
	}
	return NULL;
}

// Looping through all memories ( ForCharMemoryType ).
TRIGRET_TYPE CChar::OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, WORD wMemType )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForMemTypeLoop");
	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	if ( wMemType )
	{
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( !pItem->IsMemoryTypes(wMemType) )
				continue;
			TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
				return( iRet );
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext( StartContext );
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
			return( iRet );
	}
	else
		s.SeekContext( EndContext );
	return( TRIGRET_ENDIF );
}

// Adding a new value for this memory, updating notoriety
CItemMemory * CChar::Memory_AddObjTypes( CGrayUID uid, WORD MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_AddObjTypes");
	CItemMemory * pMemory = Memory_FindObj( uid );
	if ( pMemory == NULL )
	{
		return Memory_CreateObj( uid, MemTypes );
	}
	Memory_AddTypes( pMemory, MemTypes );
	NotoSave_Delete( uid.CharFind() );
	return( pMemory );
}

// NOTE: Do not return true unless u update the timer !
// RETURN: false = done with this memory.
bool CChar::Memory_OnTick( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_OnTick");
	ASSERT(pMemory);

	CObjBase * pObj = pMemory->m_uidLink.ObjFind();
	if ( pObj == NULL )
		return( false );

	if ( pMemory->IsMemoryTypes( MEMORY_FIGHT ))
	{
		// Is the fight still valid ?
		return Memory_Fight_OnTick( pMemory );
	}

	if ( pMemory->IsMemoryTypes( MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN ))
		return( true );	// never go away.

	return( false );	// kill it?.
}

//////////////////////////////////////////////////////////////////////////////

// I noticed a crime.
void CChar::OnNoticeCrime( CChar * pCriminal, const CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::OnNoticeCrime");
	if ( !pCriminal || (pCriminal == this) || (pCriminal == pCharMark) || pCriminal->IsPriv(PRIV_GM) || (pCriminal->GetNPCBrain(false) == NPCBRAIN_GUARD) )
		return;
	NOTO_TYPE iNoto = pCharMark->Noto_GetFlag(pCriminal);
	if ( iNoto == NOTO_CRIMINAL || iNoto == NOTO_EVIL )
		return;


	if ( m_pPlayer )
	{
		// I have the option of attacking the criminal. or calling the guards.
		bool bCriminal = true;
		if (IsTrigUsed(TRIGGER_SEECRIME))
		{
			CScriptTriggerArgs Args;
			Args.m_iN1 = bCriminal; 
			Args.m_pO1 = const_cast<CChar*>(pCharMark);
			OnTrigger(CTRIG_SeeCrime, pCriminal, &Args);
			bCriminal = Args.m_iN1 ? true : false;
		}
		if (bCriminal)
			Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
		return;
	}

	// NPC's can take other actions.

	ASSERT(m_pNPC);
	bool fMyMaster = NPC_IsOwnedBy( pCriminal );

	if ( this != pCharMark )	// it's not me.
	{
		if ( fMyMaster )	// I won't rat you out.
			return;
	}
	else
	{
		// I being the victim can retaliate.
		Memory_AddObjTypes( pCriminal, MEMORY_SAWCRIME );
		OnHarmedBy( pCriminal );
	}

	if ( ! NPC_CanSpeak())
		return;	// I can't talk anyhow.

	if (GetNPCBrain() != NPCBRAIN_HUMAN)
	{
		// Good monsters don't call for guards outside guarded areas.
		if (!m_pArea || !m_pArea->IsGuarded())
			return;
	}

	if (m_pNPC->m_Brain != NPCBRAIN_GUARD)
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_CRIM));

	// Find a guard.
	CallGuards( pCriminal );
}

// I am commiting a crime.
// Did others see me commit or try to commit the crime.
//  SkillToSee = NONE = everyone can notice this.
// RETURN:
//  true = somebody saw me.
bool CChar::CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, LPCTSTR pAction )
{
	ADDTOCALLSTACK("CChar::CheckCrimeSeen");

	bool fSeen = false;

	// Who notices ?

	if (m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD) // guards only fight for justice, they can't commit a crime!!?
		return false;

	CWorldSearch AreaChars( GetTopPoint(), UO_MAP_VIEW_SIZE );
	for (;;)
	{
		CChar * pChar = AreaChars.GetChar();
		if ( pChar == NULL )
			break;
		if ( this == pChar )
			continue;	// I saw myself before.
		if ( ! pChar->CanSeeLOS( this, LOS_NB_WINDOWS )) //what if I was standing behind a window when I saw a crime? :)
			continue;
		if ( pChar->GetPrivLevel() >= PLEVEL_Counsel ) // if a GM sees you it's not a crime
			continue;

		bool fYour = ( pCharMark == pChar );


		char *z = Str_GetTemp();
		if ( pItem && pAction )
		{
			if ( pCharMark )
				sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_2), GetName(), pAction, fYour ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_YOUR) : pCharMark->GetName(), fYour ? "" : g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_S), pItem->GetName());
			else
				sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_1), GetName(), pAction, pItem->GetName());
			pChar->ObjMessage(z, this);
		}

		fSeen = true;

		// They are not a criminal til someone calls the guards !!!
		if ( SkillToSee == SKILL_SNOOPING )
		{
			if (IsTrigUsed(TRIGGER_SEESNOOP))
			{
				CScriptTriggerArgs Args(pAction);
				Args.m_iN1 = SkillToSee ? SkillToSee : pCharMark->Skill_GetActive();
				Args.m_iN2 = pItem ? (DWORD)pItem->GetUID() : 0;
				Args.m_pO1 = pCharMark;
				TRIGRET_TYPE iRet = pChar->OnTrigger(CTRIG_SeeSnoop, this, &Args);

				if (iRet == TRIGRET_RET_TRUE)
					continue;
				else if (iRet == TRIGRET_RET_DEFAULT)
				{
					if (!g_Cfg.Calc_CrimeSeen(this, pChar, SkillToSee, fYour))
						continue;
				}
			}
			// Off chance of being a criminal. (hehe)
			if ( Calc_GetRandVal(100) < g_Cfg.m_iSnoopCriminal )
				pChar->OnNoticeCrime( this, pCharMark );
			if ( pChar->m_pNPC )
				pChar->NPC_OnNoticeSnoop( this, pCharMark );
		}
		else
			pChar->OnNoticeCrime( this, pCharMark );
	}
	return fSeen;
}

// Assume the container is not locked.
// return: true = snoop or can't open at all.
//  false = instant open.
bool CChar::Skill_Snoop_Check( const CItemContainer * pItem )
{
	ADDTOCALLSTACK("CChar::Skill_Snoop_Check");

	if ( pItem == NULL )
		return( true );

	ASSERT( pItem->IsItem());
	if ( pItem->IsContainer() )
	{
		CItemContainer *pItemCont = dynamic_cast<CItemContainer *>(pItem->GetParentObj());
		if ( !g_Cfg.m_iTradeWindowSnooping && pItemCont->IsItemInTrade() )
			return false;
	}

	if ( ! IsPriv(PRIV_GM))
	switch ( pItem->GetType())
	{
		case IT_SHIP_HOLD_LOCK:
		case IT_SHIP_HOLD:
			// Must be on board a ship to open the hatch.
			ASSERT(m_pArea);
			if ( m_pArea->GetResourceID() != pItem->m_uidLink )
			{
				SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_HATCH_FAIL));
				return( true );
			}
			break;
		case IT_EQ_BANK_BOX:
			// Some sort of cheater.
			return( false );
	}

	CChar * pCharMark;
	if ( ! IsTakeCrime( pItem, &pCharMark ) || pCharMark == NULL )
		return( false );

	if ( Skill_Wait(SKILL_SNOOPING))
		return( true );

	m_Act_Targ = pItem->GetUID();
	Skill_Start( SKILL_SNOOPING );
	return( true );
}

// SKILL_SNOOPING
// m_Act_Targ = object to snoop into.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Snooping( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Snooping");

	if ( stage == SKTRIG_STROKE )
		return( 0 );

	// Assume the container is not locked.
	CItemContainer * pCont = static_cast<CItemContainer *>(m_Act_Targ.ItemFind());
	if ( pCont == NULL )
		return( -SKTRIG_QTY );

	CChar * pCharMark;
	if ( ! IsTakeCrime( pCont, &pCharMark ) || pCharMark == NULL )
		return( 0 );	// Not a crime really.

	if ( GetTopDist3D( pCharMark ) > 1 )
	{
		SysMessageDefault( DEFMSG_SNOOPING_REACH );
		return( -SKTRIG_QTY );
	}

	if ( !CanTouch( pCont ))
	{
		SysMessageDefault( DEFMSG_SNOOPING_CANT );
		return( -SKTRIG_QTY );
	}

	if ( stage == SKTRIG_START )
	{
		PLEVEL_TYPE plevel = GetPrivLevel();
		if ( plevel < pCharMark->GetPrivLevel())
		{
			SysMessageDefault( DEFMSG_SNOOPING_CANT );
			return( -SKTRIG_QTY );
		}

		// return the difficulty.
		return( (Skill_GetAdjusted(SKILL_SNOOPING) < Calc_GetRandVal(1000))? 100 : 0 );
	}

	// did anyone see this ?
	CheckCrimeSeen( SKILL_SNOOPING, pCharMark, pCont, g_Cfg.GetDefaultMsg( DEFMSG_SNOOPING_ATTEMPTING ) );
	Noto_Karma( -4, INT_MIN, true );

	if ( stage == SKTRIG_FAIL )
	{
		SysMessageDefault( DEFMSG_SNOOPING_FAILED );
		if ( Skill_GetAdjusted(SKILL_HIDING) / 2 < Calc_GetRandVal(1000) )
			Reveal();
	}

	if ( stage == SKTRIG_SUCCESS )
	{
		if ( m_pClient )
			m_pClient->addContainerSetup( pCont );	// open the container
	}
	return( 0 );
}

// m_Act_Targ = object to steal.
// RETURN:
// -SKTRIG_QTY = no chance. and not a crime
// -SKTRIG_FAIL = no chance and caught.
// 0-100 = difficulty = percent chance of failure.
int CChar::Skill_Stealing( SKTRIG_TYPE stage )
{
	ADDTOCALLSTACK("CChar::Skill_Stealing");
	if ( stage == SKTRIG_STROKE )
		return( 0 );

	CItem * pItem = m_Act_Targ.ItemFind();
	CChar * pCharMark = NULL;
	if ( pItem == NULL )	// on a chars head ? = random steal.
	{
		pCharMark = m_Act_Targ.CharFind();
		if ( pCharMark == NULL )
		{
			SysMessageDefault( DEFMSG_STEALING_NOTHING );
			return( -SKTRIG_QTY );
		}
		CItemContainer *pPack = pCharMark->GetContainer(LAYER_PACK);
		if ( pPack == NULL )
		{
cantsteal:
			SysMessageDefault( DEFMSG_STEALING_EMPTY );
			return( -SKTRIG_QTY );
		}

		pItem = pPack->GetAt(Calc_GetRandVal(pPack->GetCount()));		// random item on backpack
		if ( !pItem )
			goto cantsteal;

		m_Act_Targ = pItem->GetUID();
	}

	// Special cases.
	CContainer *pContainer = dynamic_cast<CContainer *>(pItem->GetParentObj());
	if ( pContainer )
	{
		CItemCorpse *pCorpse = dynamic_cast<CItemCorpse *>(pContainer);
		if ( pCorpse )
		{
			SysMessageDefault( DEFMSG_STEALING_CORPSE );
			return( -SKTRIG_ABORT );
		}
	}
	CItem *pCItem = dynamic_cast<CItem *>(pItem->GetParentObj());
	if ( pCItem )
	{
		if ( pCItem->GetType() == IT_GAME_BOARD )
		{
			SysMessageDefault( DEFMSG_STEALING_GAMEBOARD );
			return( -SKTRIG_ABORT );
		}
		if ( pCItem->GetType() == IT_EQ_TRADE_WINDOW )
		{
			SysMessageDefault( DEFMSG_STEALING_TRADE );
			return( -SKTRIG_ABORT );
		}
	}
	if ( pItem->IsType(IT_TRAIN_PICKPOCKET))
	{
		SysMessageDefault( DEFMSG_STEALING_PICKPOCKET );
		return -SKTRIG_QTY;
	}
	if ( pItem->IsType( IT_GAME_PIECE ))
	{
		return -SKTRIG_QTY;
	}
	if ( ! CanTouch( pItem ))
	{
		SysMessageDefault( DEFMSG_STEALING_REACH );
		return( -SKTRIG_ABORT );
	}
	if ( ! CanMove( pItem ) || ! CanCarry( pItem ))
	{
		SysMessageDefault( DEFMSG_STEALING_HEAVY );
		return( -SKTRIG_ABORT );
	}
	if ( ! IsTakeCrime( pItem, & pCharMark ))
	{
		SysMessageDefault( DEFMSG_STEALING_NONEED );

		// Just pick it up ?
		return( -SKTRIG_QTY );
	}
	if ( m_pArea->IsFlag(REGION_FLAG_SAFE))
	{
		SysMessageDefault( DEFMSG_STEALING_STOP );
		return( -SKTRIG_QTY );
	}

	Reveal();	// If we take an item off the ground we are revealed.

	bool fGround = false;
	if ( pCharMark != NULL )
	{
		if ( GetTopDist3D( pCharMark ) > 2 )
		{
			SysMessageDefault( DEFMSG_STEALING_MARK );
			return -SKTRIG_QTY;
		}
		if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pCharMark->m_pPlayer && ! IsPriv(PRIV_GM))
		{
			SysMessageDefault( DEFMSG_STEALING_SAFE );
			return( -1 );
		}
		if ( GetPrivLevel() < pCharMark->GetPrivLevel())
		{
			return -SKTRIG_FAIL;
		}
		if ( stage == SKTRIG_START )
		{
			return g_Cfg.Calc_StealingItem( this, pItem, pCharMark );
		}
	}
	else
	{
		// stealing off the ground should always succeed.
		// it's just a matter of getting caught.
		if ( stage == SKTRIG_START )
			return( 1 );	// town stuff on the ground is too easy.

		fGround = true;
	}

	// Deliver the goods.

	if ( stage == SKTRIG_SUCCESS || fGround )
	{
		pItem->ClrAttr(ATTR_OWNED);	// Now it's mine
		CItemContainer *pPack = GetContainer(LAYER_PACK);
		if ( pPack && (pItem->GetParent() != pPack) )
		{
			pItem->RemoveFromView();
			// Put in my invent.
			pPack->ContentAdd( pItem );
		}
	}

	if ( m_Act_Difficulty == 0 )
		return( 0 );	// Too easy to be bad. hehe

	// You should only be able to go down to -1000 karma by stealing.
	if ( CheckCrimeSeen( SKILL_STEALING, pCharMark, pItem, (stage == SKTRIG_FAIL)? g_Cfg.GetDefaultMsg( DEFMSG_STEALING_YOUR ) : g_Cfg.GetDefaultMsg( DEFMSG_STEALING_SOMEONE ) ))
		Noto_Karma( -100, -1000, true );

	return( 0 );
}

void CChar::CallGuards()
{
	ADDTOCALLSTACK("CChar::CallGuards");
	if ( !m_pArea || !m_pArea->IsGuarded() || IsStatFlag(STATF_DEAD) )
		return;
	if ( g_World.GetCurrentTime().GetTimeRaw() - m_timeLastCallGuards < TICK_PER_SEC )	// flood check to avoid waste CPU performance with excessive calls of this function
		return;

	// We don't have any target yet, let's check everyone nearby
	CChar *pCriminal = NULL;
	CWorldSearch AreaCrime(GetTopPoint(), UO_MAP_VIEW_SIZE);
	while ( (pCriminal = AreaCrime.GetChar()) != NULL )
	{
		if ( pCriminal == this )
			continue;
		if ( !pCriminal->m_pArea->IsGuarded() )
			continue;
		if ( !CanDisturb(pCriminal) )	// don't allow guards to be called on someone we can't disturb
			continue;

		// Mark person as criminal if I saw him criming
		// Only players call guards this way. NPC's flag criminal instantly
		if ( m_pPlayer && Memory_FindObjTypes(pCriminal, MEMORY_SAWCRIME) )
			pCriminal->Noto_Criminal();

		if ( pCriminal->IsStatFlag(STATF_Criminal) || (g_Cfg.m_fGuardsOnMurderers && pCriminal->Noto_IsEvil()) )
			CallGuards(pCriminal);
	}
	m_timeLastCallGuards = g_World.GetCurrentTime().GetTimeRaw();
	return;
}

void CChar::CallGuards( CChar * pCriminal )
{
	ADDTOCALLSTACK("CChar::CallGuards1");
	if ( !pCriminal || (pCriminal == this) )
		return;
	if ( !m_pArea || !pCriminal->m_pArea->IsGuarded() || IsStatFlag(STATF_DEAD) )
		return;
	if ( pCriminal->IsStatFlag(STATF_DEAD|STATF_INVUL) || pCriminal->IsPriv(PRIV_GM) )
		return;

	CChar *pGuard = NULL;
	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		// I'm a guard, why summon someone else to do my work? :)
		pGuard = this;
	}
	else
	{
		// Search for a free guards nearby
		CWorldSearch AreaGuard(GetTopPoint(), UO_MAP_VIEW_SIZE);
		CChar *pGuardFound = NULL;
		while ( (pGuardFound = AreaGuard.GetChar()) != NULL )
		{
			if ( pGuardFound->m_pNPC && (pGuardFound->m_pNPC->m_Brain == NPCBRAIN_GUARD) && // Char found must be a guard
				( pGuardFound->m_Fight_Targ == pCriminal->GetUID() || !pGuardFound->IsStatFlag(STATF_War)) )	// and will be eligible to fight this target if it's not already on a fight or if its already attacking this target (to avoid spamming docens of guards at the same target).
			{
				pGuard = pGuardFound;
				break;
			}
		}
	}

	CVarDefCont *pVarDef = pCriminal->m_pArea->m_TagDefs.GetKey("OVERRIDE.GUARDS");
	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_CHARDEF, (pVarDef ? pVarDef->GetValStr() : "GUARDS"));
	if ( IsTrigUsed(TRIGGER_CALLGUARDS) )
	{
		CScriptTriggerArgs Args(pGuard);
		Args.m_iN1 = rid.GetResIndex();
		Args.m_iN2 = 0;
		Args.m_VarObjs.Insert(1, pCriminal, true);

		if ( OnTrigger(CTRIG_CallGuards, pCriminal, &Args) == TRIGRET_RET_TRUE )
			return;

		if ( static_cast<int>(Args.m_iN1) != rid.GetResIndex() )
			rid = RESOURCE_ID(RES_CHARDEF, static_cast<int>(Args.m_iN1));
		if ( Args.m_iN2 > 0 )	//ARGN2: If set to 1, a new guard will be spawned regardless of whether a nearby guard is available.
			pGuard = NULL;
	}
	if ( !pGuard )		//	spawn a new guard
	{
		if ( !rid.IsValidUID() )
			return;
		pGuard = CChar::CreateNPC(static_cast<CREID_TYPE>(rid.GetResIndex()));
		if ( !pGuard )
			return;

		if ( pCriminal->m_pArea->m_TagDefs.GetKeyNum("RED") )
			pGuard->m_TagDefs.SetNum("NAME.HUE", g_Cfg.m_iColorNotoEvil, true);
		pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, 1000, g_Cfg.m_iGuardLingerTime);
		pGuard->Spell_Teleport(pCriminal->GetTopPoint(), false, false);
	}
	pGuard->NPC_LookAtCharGuard(pCriminal);
}

// i notice a Crime or attack against me ..
// Actual harm has taken place.
// Attack back.
void CChar::OnHarmedBy( CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::OnHarmedBy");

	bool fFightActive = Fight_IsActive();
	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY);

	if (fFightActive && m_Fight_Targ.CharFind())
	{
		// In war mode already
		if ( m_pPlayer )
			return;
		if ( Calc_GetRandVal( 10 ))
			return;
		// NPC will Change targets.
	}

	if ( NPC_IsOwnedBy(pCharSrc, false) )
		NPC_PetDesert();

	// I will Auto-Defend myself.
	Fight_Attack(pCharSrc);
}

// We have been attacked in some way by this CChar.
// Might not actually be doing any real damage. (yet)
//
// They may have just commanded their pet to attack me.
// Cast a bad spell at me.
// Fired projectile at me.
// Attempted to provoke me ?
//
// RETURN: true = ok.
//  false = we are immune to this char ! (or they to us)
bool CChar::OnAttackedBy(CChar *pCharSrc, bool bCommandPet, bool bShouldReveal)
{
	ADDTOCALLSTACK("CChar::OnAttackedBy");

	if ( !pCharSrc || (pCharSrc == this) )
		return true;
	if ( IsStatFlag(STATF_DEAD) )
		return false;

	if ( bShouldReveal )
		pCharSrc->Reveal();

	if ( Fight_IsActive() && (m_Fight_Targ == pCharSrc->GetUID()) )
		return true;

	Memory_AddObjTypes( pCharSrc, MEMORY_HARMEDBY|MEMORY_IRRITATEDBY );
	Attacker_Add(pCharSrc);

	// Are they a criminal for it ? Is attacking me a crime ?
	if ( Noto_GetFlag(pCharSrc) == NOTO_GOOD )
	{
		if ( m_pClient )	// I decide if this is a crime.
			OnNoticeCrime( pCharSrc, this );
		else		
		{
			// If it is a pet then this a crime others can report.
			CChar * pCharMark = IsStatFlag(STATF_Pet) ? NPC_PetGetOwner() : this;
			pCharSrc->CheckCrimeSeen(Skill_GetActive(), pCharMark, NULL, NULL);
		}
	}

	if ( !bCommandPet )
		OnHarmedBy(pCharSrc);	// possibly retaliate. (auto defend)

	return true;
}

// Armor layers that can be damaged on combat
// PS: Hand layers (weapons/shields) are not included here
static const LAYER_TYPE sm_ArmorDamageLayers[] = { LAYER_SHOES, LAYER_PANTS, LAYER_SHIRT, LAYER_HELM, LAYER_GLOVES, LAYER_COLLAR, LAYER_HALF_APRON, LAYER_CHEST, LAYER_TUNIC, LAYER_ARMS, LAYER_CAPE, LAYER_ROBE, LAYER_SKIRT, LAYER_LEGS };

// Layers covering the armor zone
static const LAYER_TYPE sm_ArmorLayerHead[] = { LAYER_HELM };															// ARMOR_HEAD
static const LAYER_TYPE sm_ArmorLayerNeck[] = { LAYER_COLLAR };															// ARMOR_NECK
static const LAYER_TYPE sm_ArmorLayerBack[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_CAPE, LAYER_ROBE };		// ARMOR_BACK
static const LAYER_TYPE sm_ArmorLayerChest[] = { LAYER_SHIRT, LAYER_CHEST, LAYER_TUNIC, LAYER_ROBE };					// ARMOR_CHEST
static const LAYER_TYPE sm_ArmorLayerArms[] = { LAYER_ARMS, LAYER_CAPE, LAYER_ROBE };									// ARMOR_ARMS
static const LAYER_TYPE sm_ArmorLayerHands[] = { LAYER_GLOVES };														// ARMOR_HANDS
static const LAYER_TYPE sm_ArmorLayerLegs[] = { LAYER_PANTS, LAYER_SKIRT, LAYER_HALF_APRON, LAYER_ROBE, LAYER_LEGS };	// ARMOR_LEGS
static const LAYER_TYPE sm_ArmorLayerFeet[] = { LAYER_SHOES, LAYER_LEGS };												// ARMOR_FEET

struct CArmorLayerType
{
	WORD m_wCoverage;	// Percentage of humanoid body area
	const LAYER_TYPE * m_pLayers;
};

static const CArmorLayerType sm_ArmorLayers[ARMOR_QTY] =
{
	{ 15,	sm_ArmorLayerHead },	// ARMOR_HEAD
	{ 7,	sm_ArmorLayerNeck },	// ARMOR_NECK
	{ 0,	sm_ArmorLayerBack },	// ARMOR_BACK
	{ 35,	sm_ArmorLayerChest },	// ARMOR_CHEST
	{ 14,	sm_ArmorLayerArms },	// ARMOR_ARMS
	{ 7,	sm_ArmorLayerHands },	// ARMOR_HANDS
	{ 22,	sm_ArmorLayerLegs },	// ARMOR_LEGS
	{ 0,	sm_ArmorLayerFeet }		// ARMOR_FEET
};

// When armor is added or subtracted check this.
// This is the general AC number printed.
// Tho not really used to compute damage.
WORD CChar::CalcArmorDefense() const
{
	ADDTOCALLSTACK("CChar::CalcArmorDefense");

	WORD iArmorCount = 0;
	WORD iDefense = 0;
	WORD iDefenseTotal = 0;
	WORD ArmorRegionMax[ARMOR_QTY];
	for ( int i = 0; i < ARMOR_QTY; i++ )
		ArmorRegionMax[i] = 0;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		iDefense = static_cast<WORD>(pItem->Armor_GetDefense());
		if ( !iDefense && !pItem->IsType(IT_SPELL) )
			continue;

		// reverse of sm_ArmorLayers
		switch ( pItem->GetEquipLayer() )
		{
			case LAYER_HELM:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_HEAD] += iDefense;
				else
					ArmorRegionMax[ARMOR_HEAD] = maximum(ArmorRegionMax[ARMOR_HEAD], iDefense);
				break;
			case LAYER_COLLAR:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_NECK] += iDefense;
				else
					ArmorRegionMax[ARMOR_NECK] = maximum(ArmorRegionMax[ARMOR_NECK], iDefense);
				break;
			case LAYER_SHIRT:
			case LAYER_CHEST:
			case LAYER_TUNIC:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					ArmorRegionMax[ARMOR_CHEST] += iDefense;
					ArmorRegionMax[ARMOR_BACK] += iDefense;
				}
				else
				{
					ArmorRegionMax[ARMOR_CHEST] = maximum(ArmorRegionMax[ARMOR_CHEST], iDefense);
					ArmorRegionMax[ARMOR_BACK] = maximum(ArmorRegionMax[ARMOR_BACK], iDefense);
				}
				break;
			case LAYER_ARMS:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_ARMS] += iDefense;
				else
					ArmorRegionMax[ARMOR_ARMS] = maximum(ArmorRegionMax[ARMOR_ARMS], iDefense);
				break;
			case LAYER_PANTS:
			case LAYER_SKIRT:
			case LAYER_HALF_APRON:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_LEGS] += iDefense;
				else
					ArmorRegionMax[ARMOR_LEGS] = maximum(ArmorRegionMax[ARMOR_LEGS], iDefense);
				break;
			case LAYER_SHOES:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_FEET] += iDefense;
				else
					ArmorRegionMax[ARMOR_FEET] = maximum(ArmorRegionMax[ARMOR_FEET], iDefense);
				break;
			case LAYER_GLOVES:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					ArmorRegionMax[ARMOR_HANDS] += iDefense;
				else
					ArmorRegionMax[ARMOR_HANDS] = maximum(ArmorRegionMax[ARMOR_HANDS], iDefense);
				break;
			case LAYER_CAPE:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					ArmorRegionMax[ARMOR_BACK] += iDefense;
					ArmorRegionMax[ARMOR_ARMS] += iDefense;
				}
				else
				{
					ArmorRegionMax[ARMOR_BACK] = maximum(ArmorRegionMax[ARMOR_BACK], iDefense);
					ArmorRegionMax[ARMOR_ARMS] = maximum(ArmorRegionMax[ARMOR_ARMS], iDefense);
				}
				break;
			case LAYER_ROBE:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					ArmorRegionMax[ARMOR_CHEST] += iDefense;
					ArmorRegionMax[ARMOR_BACK] += iDefense;
					ArmorRegionMax[ARMOR_ARMS] += iDefense;
					ArmorRegionMax[ARMOR_LEGS] += iDefense;
				}
				else
				{
					ArmorRegionMax[ARMOR_CHEST] = maximum(ArmorRegionMax[ARMOR_CHEST], iDefense);
					ArmorRegionMax[ARMOR_BACK] = maximum(ArmorRegionMax[ARMOR_BACK], iDefense);
					ArmorRegionMax[ARMOR_ARMS] = maximum(ArmorRegionMax[ARMOR_ARMS], iDefense);
					ArmorRegionMax[ARMOR_LEGS] = maximum(ArmorRegionMax[ARMOR_LEGS], iDefense);
				}
				break;
			case LAYER_LEGS:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					ArmorRegionMax[ARMOR_LEGS] += iDefense;
					ArmorRegionMax[ARMOR_FEET] += iDefense;
				}
				else
				{
					ArmorRegionMax[ARMOR_LEGS] = maximum(ArmorRegionMax[ARMOR_LEGS], iDefense);
					ArmorRegionMax[ARMOR_FEET] = maximum(ArmorRegionMax[ARMOR_FEET], iDefense);
				}
				break;
			case LAYER_HAND2:
				if ( pItem->IsType(IT_SHIELD) )
				{
					if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
						ArmorRegionMax[ARMOR_HANDS] += iDefense;
					else
						ArmorRegionMax[ARMOR_HANDS] = maximum(ArmorRegionMax[ARMOR_HANDS], iDefense);
				}
				break;
			case LAYER_SPELL_Protection:
				iDefenseTotal += pItem->m_itSpell.m_spelllevel * 100;
				break;
			default:
				continue;
		}
		iArmorCount++;
	}

	if ( iArmorCount )
	{
		for ( int i = 0; i < ARMOR_QTY; i++ )
			iDefenseTotal += sm_ArmorLayers[i].m_wCoverage * ArmorRegionMax[i];
	}

	return (iDefenseTotal / 100) + static_cast<WORD>(m_ModAr);
}

// Someone hit us.
// iDmg already defined, here we just apply armor related calculations
//
// uType: damage flags
//  DAMAGE_GOD
//  DAMAGE_HIT_BLUNT
//  DAMAGE_MAGIC
//  ...
//
// iDmg[Physical/Fire/Cold/Poison/Energy]: % of each type to split the damage into
//  Eg: iDmgPhysical=70 + iDmgCold=30 will split iDmg value into 70% physical + 30% cold
//
// RETURN: damage done
//  -1		= already dead / invalid target.
//  0		= no damage.
//  INT_MAX	= killed.
int CChar::OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy )
{
	ADDTOCALLSTACK("CChar::OnTakeDamage");

	if ( pSrc == NULL )
		pSrc = this;

	if ( IsStatFlag(STATF_DEAD) )	// already dead
		return( -1 );
	if ( !(uType & DAMAGE_GOD) )
	{
		if ( IsStatFlag(STATF_INVUL|STATF_Stone) )
		{
effect_bounce:
			Effect( EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16 );
			return( 0 );
		}
		if ( (uType & DAMAGE_FIRE) && Can(CAN_C_FIRE_IMMUNE) )
			goto effect_bounce;
		if ( m_pArea )
		{
			if ( m_pArea->IsFlag(REGION_FLAG_SAFE) )
				goto effect_bounce;
			if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pSrc && ((IsStatFlag(STATF_Pet) && NPC_PetGetOwner() == pSrc) || (m_pPlayer && (pSrc->m_pPlayer || pSrc->IsStatFlag(STATF_Pet)))) )
				goto effect_bounce;
		}
	}

	// Make some notoriety checks
	// Don't reveal attacker if the damage has DAMAGE_NOREVEAL flag set (this is set by default for poison and spell damage)
	if ( !OnAttackedBy(pSrc, false, !(uType & DAMAGE_NOREVEAL)) )
		return( 0 );

	// Apply Necromancy cursed effects
	if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
	{
		CItem * pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
		if ( pEvilOmen )
		{
			iDmg += iDmg / 4;
			pEvilOmen->Delete();
		}

		CItem * pBloodOath = LayerFind(LAYER_SPELL_Blood_Oath);
		if ( pBloodOath && pBloodOath->m_uidLink == pSrc->GetUID() && !(uType & DAMAGE_FIXED) )	// if DAMAGE_FIXED is set we are already receiving a reflected damage, so we must stop here to avoid an infinite loop.
		{
			iDmg += iDmg / 10;
			pSrc->OnTakeDamage(iDmg * (100 - pBloodOath->m_itSpell.m_spelllevel) / 100, this, DAMAGE_MAGIC|DAMAGE_FIXED);
		}
	}

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// MAGICF_IGNOREAR bypasses defense completely
	if ( (uType & DAMAGE_MAGIC) && IsSetMagicFlags(MAGICF_IGNOREAR) )
		uType |= DAMAGE_FIXED;

	// Apply armor calculation
	if ( !(uType & (DAMAGE_GOD|DAMAGE_FIXED)) )
	{
		if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// AOS elemental combat
			if ( iDmgPhysical == 0 )		// if physical damage is not set, let's assume it as the remaining value
				iDmgPhysical = 100 - (iDmgFire + iDmgCold + iDmgPoison + iDmgEnergy);

			int iPhysicalDamage = iDmg * iDmgPhysical * (100 - static_cast<int>(GetDefNum("RESPHYSICAL")));
			int iFireDamage = iDmg * iDmgFire * (100 - static_cast<int>(GetDefNum("RESFIRE")));
			int iColdDamage = iDmg * iDmgCold * (100 - static_cast<int>(GetDefNum("RESCOLD")));
			int iPoisonDamage = iDmg * iDmgPoison * (100 - static_cast<int>(GetDefNum("RESPOISON")));
			int iEnergyDamage = iDmg * iDmgEnergy * (100 - static_cast<int>(GetDefNum("RESENERGY")));

			iDmg = (iPhysicalDamage + iFireDamage + iColdDamage + iPoisonDamage + iEnergyDamage) / 10000;
		}
		else
		{
			// pre-AOS armor rating (AR)
			int iArmorRating = pCharDef->m_defense + m_defense;

			int iArMax = iArmorRating * Calc_GetRandVal2(7,35) / 100;
			int iArMin = iArMax / 2;

			int iDef = Calc_GetRandVal2( iArMin, (iArMax - iArMin) + 1 );
			if ( uType & DAMAGE_MAGIC )		// magical damage halves effectiveness of defense
				iDef /= 2;

			iDmg -= iDef;
		}
	}

	CScriptTriggerArgs Args( iDmg, uType, static_cast<INT64>(0) );
	if ( !(uType & DAMAGE_POISON) )
	{
		Args.m_VarsLocal.SetNum("ItemDamageLayer", sm_ArmorDamageLayers[Calc_GetRandVal(COUNTOF(sm_ArmorDamageLayers))]);
		Args.m_VarsLocal.SetNum("ItemDamageChance", 40);
	}

	if ( IsTrigUsed(TRIGGER_GETHIT) )
	{
		if ( OnTrigger( CTRIG_GetHit, pSrc, &Args ) == TRIGRET_RET_TRUE )
			return( 0 );
		iDmg = static_cast<int>(Args.m_iN1);
		uType = static_cast<DAMAGE_TYPE>(Args.m_iN2);
	}

	int iItemDamageChance = static_cast<int>(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
	LAYER_TYPE iItemDamageLayer = static_cast<LAYER_TYPE>(Args.m_VarsLocal.GetKeyNum("ItemDamageLayer"));
	if ( iItemDamageLayer && (iItemDamageChance > Calc_GetRandVal(100)) && !pCharDef->Can(CAN_C_NONHUMANOID) )
	{
		CItem *pItemHit = LayerFind(iItemDamageLayer);
		if ( pItemHit )
			pItemHit->OnTakeDamage(iDmg, pSrc, uType);
	}

	// Remove stuck/paralyze effect
	if ( !(uType & DAMAGE_NOUNPARALYZE) )
	{
		CItem * pParalyze = LayerFind(LAYER_SPELL_Paralyze);
		if ( pParalyze )
			pParalyze->Delete();

		CItem * pStuck = LayerFind(LAYER_FLAG_Stuck);
		if ( pStuck )
			pStuck->Delete();

		if ( IsStatFlag(STATF_Freeze) )
		{
			StatFlag_Clear(STATF_Freeze);
			UpdateMode();
		}
	}

	// Disturb magic spells (only players can be disturbed)
	if ( m_pPlayer && (pSrc != this) && !(uType & DAMAGE_NODISTURB) && g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
	{
		// Check if my spell can be interrupted
		int iDisturbChance = 0;
		int iSpellSkill;
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
		if ( pSpellDef && pSpellDef->GetPrimarySkill(&iSpellSkill) )
			iDisturbChance = pSpellDef->m_Interrupt.GetLinear(Skill_GetBase(static_cast<SKILL_TYPE>(iSpellSkill)));

		if ( iDisturbChance && IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// Protection spell can cancel the disturb
			CItem *pProtectionSpell = LayerFind(LAYER_SPELL_Protection);
			if ( pProtectionSpell )
			{
				int iChance = pProtectionSpell->m_itSpell.m_spelllevel;
				if ( iChance > Calc_GetRandVal(1000) )
					iDisturbChance = 0;
			}
		}

		if ( iDisturbChance > Calc_GetRandVal(1000) )
			Skill_Fail();
	}

	if ( pSrc && (pSrc != this) )
	{
		// Update attacker list
		bool bAttackerExists = false;
		for (std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it)
		{
			LastAttackers & refAttacker = *it;
			if ( refAttacker.charUID == pSrc->GetUID().GetPrivateUID() )
			{
				refAttacker.elapsed = 0;
				refAttacker.amountDone += maximum( 0, iDmg );
				refAttacker.threat += maximum( 0, iDmg );
				bAttackerExists = true;
				break;
			}
		}

		if (bAttackerExists == false)
		{
			LastAttackers attacker;
			attacker.charUID = pSrc->GetUID().GetPrivateUID();
			attacker.elapsed = 0;
			attacker.amountDone = maximum( 0, iDmg );
			attacker.threat = maximum( 0, iDmg );
			m_lastAttackers.push_back(attacker);
		}

		// A physical blow of some sort.
		if (uType & (DAMAGE_HIT_BLUNT|DAMAGE_HIT_PIERCE|DAMAGE_HIT_SLASH))
		{
			// Check if Reactive Armor will reflect some damage back
			if ( IsStatFlag(STATF_Reactive) && !(uType & DAMAGE_GOD) )
			{
				if ( GetTopDist3D(pSrc) < 2 )
				{
					int iReactiveDamage = iDmg / 5;
					if ( iReactiveDamage < 1 )
						iReactiveDamage = 1;

					iDmg -= iReactiveDamage;
					pSrc->OnTakeDamage( iReactiveDamage, this, static_cast<DAMAGE_TYPE>(DAMAGE_FIXED), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy );
					pSrc->Sound( 0x1F1 );
					pSrc->Effect( EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 10, 16 );
				}
			}
		}
	}

	if (iDmg <= 0)
		return(0);

	// Apply damage
	SoundChar(CRESND_GETHIT);
	UpdateStatVal(STAT_STR, -iDmg);
	if ( pSrc->m_pClient )
		pSrc->m_pClient->addHitsUpdate(this);		// always send updates to src

	if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_DAMAGE )
	{
		if ( m_pClient )
			m_pClient->addShowDamage(GetUID(), iDmg);
		if ( pSrc->m_pClient && (pSrc != this) )
			pSrc->m_pClient->addShowDamage(GetUID(), iDmg);
		else
		{
			CChar * pSrcOwner = pSrc->NPC_PetGetOwner();
			if ( pSrcOwner != NULL )
			{
				if ( pSrcOwner->m_pClient )
					pSrcOwner->m_pClient->addShowDamage(GetUID(), iDmg);
			}
		}
	}

	if ( Stat_GetVal(STAT_STR) <= 0 )
	{
		// We will die from this. Make sure the killer is set correctly, otherwise the person we are currently attacking will get credit for killing us.
		m_Fight_Targ = pSrc->GetUID();
		return( iDmg );
	}

	if ( m_atFight.m_Swing_State != WAR_SWING_SWINGING )	// don't interrupt my swing animation
		UpdateAnimate(ANIM_GET_HIT);

	return( iDmg );
}

//*******************************************************************************
// Fight specific memories.

// Check on the status of the fight.
// return: false = delete the memory completely.
//  true = skip it.
bool CChar::Memory_Fight_OnTick( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_OnTick");

	ASSERT(pMemory);
	CChar *pTarg = pMemory->m_uidLink.CharFind();
	if ( !pTarg )
		return false;

	if ( GetDist(pTarg) > UO_MAP_VIEW_RADAR )
	{
		Attacker_Delete(pTarg, true, ATTACKER_CLEAR_DISTANCE);
		if ( m_pPlayer && pTarg->m_pPlayer )	// only show retreat msg on PvP
		{
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_1), pTarg->GetName());
			pTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_2), GetName());
		}
		return false;
	}

	pMemory->SetTimeout(20 * TICK_PER_SEC);
	return true;	// reschedule it
}

void CChar::Memory_Fight_Start( const CChar * pTarg )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Start");
	// I am attacking this creature.
	// i might be the aggressor or just retaliating.
	// This is just the "Intent" to fight. Maybe No damage done yet.

	ASSERT(pTarg);
	if (Fight_IsActive() && m_Fight_Targ == pTarg->GetUID())
	{
		// quick check that things are ok.
		return;
	}

	WORD MemTypes;
	CItemMemory * pMemory = Memory_FindObj( pTarg );
	if ( pMemory == NULL )
	{
		// I have no memory of them yet.
		// There was no fight. Am I the aggressor ?
		CItemMemory * pTargMemory = pTarg->Memory_FindObj( this );
		if ( pTargMemory != NULL )	// My target remembers me.
		{
			if ( pTargMemory->IsMemoryTypes( MEMORY_IAGGRESSOR ))
				MemTypes = MEMORY_HARMEDBY;
			else if ( pTargMemory->IsMemoryTypes( MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED ))
				MemTypes = MEMORY_IAGGRESSOR;
			else
				MemTypes = 0;
		}
		else
		{
			// Hmm, I must have started this i guess.
			MemTypes = MEMORY_IAGGRESSOR;
		}
		pMemory = Memory_CreateObj( pTarg, MEMORY_FIGHT|MemTypes );
	}
	else
	{
		if ( Attacker_GetID(pTarg->GetUID()) )	// I'm already in fight against pTarg, no need of more code
			return;
		if ( pMemory->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED))
			MemTypes = 0;	// I am defending myself rightly.
		else
			MemTypes = MEMORY_IAGGRESSOR;

		Memory_AddTypes( pMemory, MEMORY_FIGHT|MemTypes );// Update the fight status.
	}
	if ( m_pClient && m_Fight_Targ == pTarg->GetUID() && !IsSetCombatFlags(COMBAT_NODIRCHANGE))
	{
		// This may be a useless command. How do i say the fight is over ?
		// This causes the funny turn to the target during combat !
		new PacketSwing(m_pClient, pTarg);
	}
	else
	{
		if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_BERSERK ) // it will attack everything.
			return;
	}
}

//********************************************************

// What sort of weapon am i using?
SKILL_TYPE CChar::Fight_GetWeaponSkill() const
{
	ADDTOCALLSTACK("CChar::Fight_GetWeaponSkill");
	CItem *pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon )
		return pWeapon->Weapon_GetSkill();
	return SKILL_WRESTLING;
}

// Am i in an active fight mode ?
bool CChar::Fight_IsActive() const
{
	ADDTOCALLSTACK("CChar::Fight_IsActive");
	if ( !IsStatFlag(STATF_War) )
		return false;

	SKILL_TYPE iSkillActive = Skill_GetActive();
	switch ( iSkillActive )
	{
		case SKILL_ARCHERY:
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
		case SKILL_THROWING:
			return true;
	}

	if ( iSkillActive == Fight_GetWeaponSkill() )
		return true;

	return g_Cfg.IsSkillFlag(iSkillActive, SKF_FIGHT);
}

bool CChar::Fight_IsAttackable() const
{
	ADDTOCALLSTACK("CChar::Fight_IsAttackable");
	return !IsStatFlag(STATF_DEAD|STATF_Stone|STATF_Invisible|STATF_Insubstantial|STATF_Hidden|STATF_INVUL);
}

// Calculating base DMG (also used for STATUS value)
int CChar::Fight_CalcDamage( const CItem * pWeapon, bool bNoRandom, bool bGetMax ) const
{
	ADDTOCALLSTACK("CChar::Fight_CalcDamage");

	if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_GUARD && g_Cfg.m_fGuardsInstantKill )
		return( 20000 );	// swing made.

	int iDmgMin = 0;
	int iDmgMax = 0;
	STAT_TYPE iStatBonus = static_cast<STAT_TYPE>(GetDefNum("COMBATBONUSSTAT"));
	int iStatBonusPercent = static_cast<int>(GetDefNum("COMBATBONUSPERCENT"));
	if ( pWeapon != NULL )
	{
		iDmgMin = pWeapon->Weapon_GetAttack(false);
		iDmgMax = pWeapon->Weapon_GetAttack(true);
	}
	else
	{
		iDmgMin = m_attackBase;
		iDmgMax = iDmgMin + m_attackRange;

		// Horrific Beast (necro spell) changes char base damage to 5-15
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if (pPoly && pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast)
			{
				iDmgMin += pPoly->m_itSpell.m_PolyStr;
				iDmgMax += pPoly->m_itSpell.m_PolyDex;
			}
		}
	}

	if ( m_pPlayer )	// only players can have damage bonus
	{
		int iDmgBonus = minimum(static_cast<int>(GetDefNum("INCREASEDAM", true)), 100);		// Damage Increase is capped at 100%

		// Racial Bonus (Berserk), gargoyles gains +15% Damage Increase per each 20 HP lost
		if ((g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle())
			iDmgBonus += minimum(15 * ((Stat_GetMax(STAT_STR) - Stat_GetVal(STAT_STR)) / 20), 60);		// value is capped at 60%

		// Horrific Beast (necro spell) add +25% Damage Increase
		if (g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)
		{
			CItem * pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if (pPoly && pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast)
				iDmgBonus += 25;
		}

		switch ( g_Cfg.m_iCombatDamageEra )
		{
			default:
			case 0:
			{
				// Sphere custom formula
				if ( !iStatBonus )
					iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
				if ( !iStatBonusPercent )
					iStatBonusPercent = 10;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}

			case 1:
			{
				// pre-AOS formula
				iDmgBonus += (Skill_GetBase(SKILL_TACTICS) - 500) / 10;

				iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 50;
				if ( Skill_GetBase(SKILL_ANATOMY) >= 1000 )
					iDmgBonus += 10;

				if ( pWeapon != NULL && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( !iStatBonus )
					iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
				if ( !iStatBonusPercent )
					iStatBonusPercent = 20;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}

			case 2:
			{
				// AOS formula
				iDmgBonus += Skill_GetBase(SKILL_TACTICS) / 16;
				if ( Skill_GetBase(SKILL_TACTICS) >= 1000 )
					iDmgBonus += 6;		//6.25

				iDmgBonus += Skill_GetBase(SKILL_ANATOMY) / 20;
				if ( Skill_GetBase(SKILL_ANATOMY) >= 1000 )
					iDmgBonus += 5;

				if ( pWeapon != NULL && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( Stat_GetAdjusted(STAT_STR) >= 100 )
					iDmgBonus += 5;

				if ( !iStatBonus )
					iStatBonus = static_cast<STAT_TYPE>(STAT_STR);
				if ( !iStatBonusPercent )
					iStatBonusPercent = 30;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}
		}

		iDmgMin += iDmgMin * iDmgBonus / 100;
		iDmgMax += iDmgMax * iDmgBonus / 100;
	}

	if ( bNoRandom )
		return( bGetMax ? iDmgMax : iDmgMin );
	else
		return( Calc_GetRandVal2(iDmgMin, iDmgMax) );
}

// Clear all my active targets. Toggle out of war mode.
void CChar::Fight_Clear()
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	if ( IsTrigUsed(TRIGGER_COMBATEND) )
		OnTrigger(CTRIG_CombatEnd, this, 0);

	m_lastAttackers.clear();
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ.InitUID();
	}
	if ( m_pNPC )
	{
		StatFlag_Clear(STATF_War);
		UpdateModeFlag();
	}
}

// We want to attack some one.
// But they won't notice til we actually hit them.
// This is just my intent.
// RETURN:
//  true = new attack is accepted.
bool CChar::Fight_Attack( const CChar *pCharTarg, bool btoldByMaster )
{
	ADDTOCALLSTACK("CChar::Fight_Attack");

	if ( !pCharTarg || (pCharTarg == this) || !pCharTarg->Fight_IsAttackable() || IsStatFlag(STATF_DEAD) )
	{
		// Not a valid target
		Attacker_Delete(const_cast<CChar *>(pCharTarg), true);
		return false;
	}
	if ( m_pNPC && !CanSeeLOS(pCharTarg) )
	{
		// The NPC can't see his target
		if ( !Calc_GetRandVal(20) )		// give up if target still out of LOS after many tries
		{
			Skill_Start(SKILL_NONE);
			return false;
		}
	}

	CChar *pTarget = const_cast<CChar *>(pCharTarg);
	if ( pCharTarg->m_pPlayer && (GetPrivLevel() <= PLEVEL_Guest) && (pCharTarg->GetPrivLevel() > PLEVEL_Guest) )
	{
		SysMessageDefault(DEFMSG_MSG_GUEST);
		Attacker_Delete(pTarget);
		return false;
	}

	if ( g_Cfg.m_fAttackingIsACrime && (pCharTarg->Noto_GetFlag(this) == NOTO_GOOD) )
		CheckCrimeSeen(SKILL_NONE, pTarget, NULL, NULL);

	INT64 threat = 0;
	if ( btoldByMaster )
		threat = 1000 + Attacker_GetHighestThreat();

	if ( (IsTrigUsed(TRIGGER_ATTACK) || IsTrigUsed(TRIGGER_CHARATTACK)) && (m_Fight_Targ != pCharTarg->GetUID()) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = threat;
		if ( OnTrigger(CTRIG_Attack, pTarget, &Args) == TRIGRET_RET_TRUE )
			return false;
		threat = Args.m_iN1;
	}

	if ( !Attacker_Add(pTarget, threat) )
		return false;

	// I'm attacking (or defending)
	if ( !IsStatFlag(STATF_War) )
	{
		StatFlag_Set(STATF_War);
		UpdateModeFlag();
		if ( m_pClient )
			m_pClient->addPlayerWarMode();
	}

	SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();
	SKILL_TYPE skillActive = Skill_GetActive();

	if ( (skillActive == skillWeapon) && (m_Fight_Targ == pCharTarg->GetUID()) )		// already attacking this same target using the same skill
		return true;
	else if ( g_Cfg.IsSkillFlag(skillActive, SKF_MAGIC) )	// don't start another fight skill when already casting spells
		return true;

	if ( m_pNPC && !btoldByMaster )		// call FindBestTarget when this CChar is a NPC and was not commanded to attack, otherwise it attack directly
		pTarget = NPC_FightFindBestTarget();

	m_Fight_Targ = pTarget ? pTarget->GetUID() : static_cast<CGrayUID>(UID_UNUSED);
	Skill_Start(skillWeapon);
	return true;
}

// A timer has expired so try to take a hit.
// I am ready to swing or already swinging.
// but i might not be close enough.
void CChar::Fight_HitTry()
{
	ADDTOCALLSTACK("CChar::Fight_HitTry");

	ASSERT(Fight_IsActive());
	ASSERT(m_atFight.m_Swing_State == (WAR_SWING_READY|WAR_SWING_SWINGING));

	CChar *pCharTarg = m_Fight_Targ.CharFind();
	if ( !pCharTarg || !pCharTarg->Fight_IsAttackable() )
	{
		// I can't hit this target, try switch to another one
		if ( !Fight_Attack(NPC_FightFindBestTarget()) )
		{
			Skill_Start(SKILL_NONE);
			m_Fight_Targ.InitUID();
			if ( m_pNPC )
				StatFlag_Clear(STATF_War);
		}
		return;
	}

	switch ( Fight_Hit(pCharTarg) )
	{
		case WAR_SWING_INVALID:		// target is invalid
		{
			Attacker_Delete(pCharTarg);
			if ( m_pNPC )
				Fight_Attack(NPC_FightFindBestTarget());
			return;
		}
		case WAR_SWING_EQUIPPING:	// keep hitting the same target
		{
			Skill_Start(Skill_GetActive());
			return;
		}
		case WAR_SWING_READY:		// probably too far away, can't take my swing right now
		{
			if ( m_pNPC )
				Fight_Attack(NPC_FightFindBestTarget());
			return;
		}
		case WAR_SWING_SWINGING:	// must come back here again to complete
			return;
	}

	ASSERT(0);
}

// Add some enemy to my Attacker list
bool CChar::Attacker_Add( CChar * pChar, INT64 threat )
{
	ADDTOCALLSTACK("CChar::Attacker_Add");
	CGrayUID uid = pChar->GetUID();
	if ( m_lastAttackers.size() )	// Must only check for existing attackers if there are any attacker already.
	{
		for ( std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
		{
			LastAttackers & refAttacker = *it;
			if ( refAttacker.charUID == uid )
				return true;	// found one, no actions needed so we skip
		}
	}
	else if ( IsTrigUsed(TRIGGER_COMBATSTART) )
	{
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatStart, pChar, 0);
		if ( tRet == TRIGRET_RET_TRUE )
			return false;
	}

	CScriptTriggerArgs Args;
	Args.m_iN1 = threat;
	if ( IsTrigUsed(TRIGGER_COMBATADD) )
	{
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatAdd, pChar, &Args);
		if ( tRet == TRIGRET_RET_TRUE )
			return false;
		threat = Args.m_iN1;
	}

	LastAttackers attacker;
	attacker.amountDone = 0;
	attacker.charUID = uid;
	attacker.elapsed = 0;
	attacker.threat = m_pPlayer ? 0 : threat;
	m_lastAttackers.push_back(attacker);

	// Record the start of the fight.
	Memory_Fight_Start(pChar);
	char *z = Str_GetTemp();

	//if ( GetTopSector()->GetCharComplexity() < 7 )
	//{
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKO), GetName(), pChar->GetName());
		UpdateObjMessage(z, NULL, pChar->m_pClient, HUE_TEXT_DEF, TALKMODE_EMOTE);
	//}

	if ( pChar->m_pClient && pChar->CanSee(this) )
	{
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKS), GetName());
		pChar->m_pClient->addBarkParse(z, this, HUE_TEXT_DEF, TALKMODE_EMOTE);
	}
	return true;
}

// Retrieves damage done to nID enemy
INT64 CChar::Attacker_GetDam( int id)
{
	ADDTOCALLSTACK("CChar::Attacker_GetDam");
	if ( ! m_lastAttackers.size() )
		return -1;
	if (  static_cast<int>(m_lastAttackers.size()) <= id )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(id);
	return refAttacker.amountDone;
}

// Retrieves the amount of time elapsed since the last hit to nID enemy
INT64 CChar::Attacker_GetElapsed( int id)
{
	ADDTOCALLSTACK("CChar::Attacker_GetElapsed");
	if ( ! m_lastAttackers.size() )
		return -1;
	if ( static_cast<int>(m_lastAttackers.size()) <= id )
		return -1;
	if ( id < 0 )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(id);
	return refAttacker.elapsed;
}

// Retrieves Threat value that nID enemy represents against me
INT64 CChar::Attacker_GetThreat( int id)
{
	ADDTOCALLSTACK("CChar::Attacker_GetThreat");
	if ( ! m_lastAttackers.size() )
		return -1;
	if ( static_cast<int>(m_lastAttackers.size()) <= id )
		return -1;
	LastAttackers & refAttacker = m_lastAttackers.at(id);
	return refAttacker.threat ? refAttacker.threat : 0;
}

// Retrieves the character with most Threat
INT64 CChar::Attacker_GetHighestThreat()
{
	if ( !m_lastAttackers.size() )
		return -1;
	INT64 highThreat = 0;
	for ( size_t count = 0; count < m_lastAttackers.size(); count++ )
	{
		LastAttackers & refAttacker = m_lastAttackers.at(count);
		if ( refAttacker.threat > highThreat )
			highThreat = refAttacker.threat;
	}
	return highThreat;
}

// Retrieves the last character that I hit
CChar * CChar::Attacker_GetLast()
{
	INT64 dwLastTime = INT_MAX, dwCurTime = 0;

	CChar * retChar = NULL;
	for (size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker)
	{
		LastAttackers & refAttacker = m_lastAttackers.at(iAttacker);
		dwCurTime = refAttacker.elapsed;
		if (dwCurTime <= dwLastTime)
		{
			dwLastTime = dwCurTime;
			retChar = static_cast<CGrayUID>(refAttacker.charUID).CharFind();
		}
	}
	return retChar;
}

// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed( CChar * pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetElapsed(CChar)");

	return Attacker_SetElapsed( Attacker_GetID(pChar), value);
}


// Set elapsed time (refreshing it?)
void CChar::Attacker_SetElapsed( int pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetElapsed(int)");
	if (pChar < 0)
		return;
	if ( ! m_lastAttackers.size() )
		return;
	if ( static_cast<int>(m_lastAttackers.size()) <= pChar )
		return;
	LastAttackers & refAttacker = m_lastAttackers.at( pChar );
	refAttacker.elapsed = value;
}

// Damaged pChar
void CChar::Attacker_SetDam( CChar * pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetDam(CChar)");

	return Attacker_SetDam( Attacker_GetID(pChar), value );
}


// Damaged pChar
void CChar::Attacker_SetDam( int pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetDam(int)");
	if (pChar < 0)
		return;
	if ( ! m_lastAttackers.size() )
		return;
	if ( static_cast<int>(m_lastAttackers.size()) <= pChar )
		return;
	LastAttackers & refAttacker = m_lastAttackers.at( pChar );
	refAttacker.amountDone = value;
}

// New Treat level
void CChar::Attacker_SetThreat(CChar * pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetThreat(CChar)");

	return Attacker_SetThreat(Attacker_GetID(pChar), value);
}

// New Treat level
void CChar::Attacker_SetThreat(int pChar, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetThreat(int)");
	if (pChar < 0)
		return;
	if (m_pPlayer)
		return;
	if (!m_lastAttackers.size())
		return;
	if (static_cast<int>(m_lastAttackers.size()) <= pChar)
		return;
	LastAttackers & refAttacker = m_lastAttackers.at(pChar);
	refAttacker.threat = value;
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::Attacker_GetID(CChar)");
	if ( !pChar )
		return -1;
	if ( ! m_lastAttackers.size() )
		return -1;
	int count = 0;
	for ( std::vector<LastAttackers>::iterator it = m_lastAttackers.begin(); it != m_lastAttackers.end(); ++it )
	{
		LastAttackers & refAttacker = m_lastAttackers.at(count);
		CGrayUID uid = refAttacker.charUID;
		if ( ! uid || !uid.CharFind() )
			continue;
		CChar * pMe = uid.CharFind()->GetChar();
		if ( ! pMe )
			continue;
		if ( pMe == pChar )
			return count;
	
		count++;
	}
	return -1;
}

// Get nID value of attacker list from the given pChar
int CChar::Attacker_GetID( CGrayUID pChar )
{
	ADDTOCALLSTACK("CChar::Attacker_GetID(CGrayUID)");
	return Attacker_GetID( pChar.CharFind()->GetChar() );
}

// Get UID value of attacker list from the given pChar
CChar * CChar::Attacker_GetUID( int index )
{
	ADDTOCALLSTACK("CChar::Attacker_GetUID");
	if ( ! m_lastAttackers.size() )
		return NULL;
	if ( static_cast<int>(m_lastAttackers.size()) <= index )
		return NULL;
	LastAttackers & refAttacker = m_lastAttackers.at(index);
	CChar * pChar = static_cast<CChar*>( static_cast<CGrayUID>( refAttacker.charUID ).CharFind() );
	return pChar;
}

// Removing nID from list
bool CChar::Attacker_Delete( int index, bool bForced, ATTACKER_CLEAR_TYPE type )
{
	ADDTOCALLSTACK("CChar::Attacker_Delete(int)");
	if ( !m_lastAttackers.size() || index < 0 || static_cast<int>(m_lastAttackers.size()) <= index )
		return false;
	LastAttackers &refAttacker = m_lastAttackers.at(index);
	CChar *pChar = static_cast<CGrayUID>(refAttacker.charUID).CharFind();
	if ( !pChar )
		return false;
	
	if ( IsTrigUsed(TRIGGER_COMBATDELETE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = bForced;
		Args.m_iN2 = static_cast<int>(type);
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_CombatDelete,pChar,&Args);
		if ( tRet == TRIGRET_RET_TRUE )
			return false;
	}

	std::vector<LastAttackers>::iterator it = m_lastAttackers.begin() + index;
	m_lastAttackers.erase(it);

	CItemMemory *pMemory = Memory_FindObj(pChar);
	if ( pMemory )
		Memory_ClearTypes(pMemory, MEMORY_FIGHT|MEMORY_IAGGRESSOR|MEMORY_AGGREIVED);

	if ( m_Fight_Targ == pChar->GetUID() )
	{
		m_Fight_Targ.InitUID();
		if ( m_pNPC )
			Fight_Attack(NPC_FightFindBestTarget());
	}
	if ( !m_lastAttackers.size() )
		Fight_Clear();
	return true;
}

// Removing pChar from list
bool CChar::Attacker_Delete(CChar *pChar, bool bForced, ATTACKER_CLEAR_TYPE type)
{		
	ADDTOCALLSTACK("CChar::Attacker_Delete(CChar)");
	return Attacker_Delete(Attacker_GetID(pChar), bForced, type);
}

// Removing everyone
void CChar::Attacker_RemoveChar()
{
	ADDTOCALLSTACK("CChar::Attacker_RemoveChar");
	if ( m_lastAttackers.size() )
	{
		for ( size_t count = 0; count < m_lastAttackers.size(); count++ )
		{
			LastAttackers &refAttacker = m_lastAttackers.at(count);
			CChar *pEnemy = static_cast<CGrayUID>(refAttacker.charUID).CharFind();
			if ( !pEnemy )
				continue;
			pEnemy->Attacker_Delete(pEnemy->Attacker_GetID(this), false, ATTACKER_CLEAR_REMOVEDCHAR);
		}
	}
}

// Checking if Elapsed > serv.AttackerTimeout
void CChar::Attacker_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::Attacker_CheckTimeout");
	if ( m_lastAttackers.size() )
	{
		for ( size_t count = 0; count < m_lastAttackers.size(); count++ )
		{
			LastAttackers &refAttacker = m_lastAttackers.at(count);
			CChar *pEnemy = static_cast<CGrayUID>(refAttacker.charUID).CharFind();
			if ( !pEnemy )
				continue;
			if ( ++refAttacker.elapsed > g_Cfg.m_iAttackerTimeout )
			{
				Attacker_Delete(static_cast<int>(count), true, ATTACKER_CLEAR_ELAPSED);
				if ( m_pPlayer && pEnemy->m_pPlayer )	// only show retreat msg on PvP
				{
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_1), pEnemy->GetName());
					pEnemy->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_2), GetName());
				}
				return;
			}
		}
	}
}

// Distance from which I can hit
int CChar::CalcFightRange( CItem * pWeapon )
{
	ADDTOCALLSTACK("CChar::CalcFightRange");

	BYTE iCharRange = RangeL();
	BYTE iWeaponRange = pWeapon ? pWeapon->RangeL() : 0;

	return maximum(iCharRange, iWeaponRange);
}


// Attempt to hit our target.
// Calculating damage
// Damaging target ( OnTakeDamage() / @GetHit )
// pCharTarg = the target.
// RETURN:
//  WAR_SWING_INVALID	= target is invalid
//  WAR_SWING_EQUIPPING	= recoiling weapon / swing made
//  WAR_SWING_READY		= can't take my swing right now. but I'm ready to hit
//  WAR_SWING_SWINGING	= taking my swing now
WAR_SWING_TYPE CChar::Fight_Hit( CChar * pCharTarg )
{
	ADDTOCALLSTACK("CChar::Fight_Hit");

	if ( !pCharTarg || (pCharTarg == this) )
		return WAR_SWING_INVALID;

	DAMAGE_TYPE iTyp = DAMAGE_HIT_BLUNT;

	if ( IsTrigUsed(TRIGGER_HITCHECK) )
	{
		CScriptTriggerArgs pArgs;
		pArgs.m_iN1 = m_atFight.m_Swing_State;
		pArgs.m_iN2 = iTyp;
		TRIGRET_TYPE tRet = OnTrigger(CTRIG_HitCheck, pCharTarg, &pArgs);
		if ( tRet == TRIGRET_RET_TRUE )
			return static_cast<WAR_SWING_TYPE>(pArgs.m_iN1);
		if ( tRet == -1 )
			return WAR_SWING_INVALID;

		m_atFight.m_Swing_State = static_cast<WAR_SWING_TYPE>(pArgs.m_iN1);
		iTyp = static_cast<DAMAGE_TYPE>(pArgs.m_iN2);

		if ( (m_atFight.m_Swing_State == WAR_SWING_SWINGING) && (iTyp & DAMAGE_FIXED) )
		{
			if ( tRet == TRIGRET_RET_DEFAULT )
				return WAR_SWING_EQUIPPING;

			if ( iTyp == DAMAGE_HIT_BLUNT )		// if type did not change in the trigger, default iTyp is set
			{
				CItem *pWeapon = m_uidWeapon.ItemFind();
				if ( pWeapon )
				{
					CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
					if ( pDamTypeOverride )
						iTyp = static_cast<DAMAGE_TYPE>(pDamTypeOverride->GetValNum());
					else
					{
						CItemBase *pWeaponDef = pWeapon->Item_GetDef();
						switch ( pWeaponDef->GetType() )
						{
							case IT_WEAPON_SWORD:
							case IT_WEAPON_AXE:
							case IT_WEAPON_THROWING:
								iTyp |= DAMAGE_HIT_SLASH;
								break;
							case IT_WEAPON_FENCE:
							case IT_WEAPON_BOW:
							case IT_WEAPON_XBOW:
								iTyp |= DAMAGE_HIT_PIERCE;
								break;
						}
					}
				}
			}
			if ( iTyp & DAMAGE_FIXED )
				iTyp &= ~DAMAGE_FIXED;

			pCharTarg->OnTakeDamage(
				Fight_CalcDamage(m_uidWeapon.ItemFind()),
				this,
				iTyp,
				static_cast<int>(GetDefNum("DAMPHYSICAL")),
				static_cast<int>(GetDefNum("DAMFIRE")),
				static_cast<int>(GetDefNum("DAMCOLD")),
				static_cast<int>(GetDefNum("DAMPOISON")),
				static_cast<int>(GetDefNum("DAMENERGY"))
				);

			return WAR_SWING_EQUIPPING;
		}
	}

	// Very basic check on possibility to hit
	if ( IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Freeze|STATF_Stone) || !pCharTarg->Fight_IsAttackable() )
		return WAR_SWING_INVALID;
	if ( pCharTarg->m_pArea && pCharTarg->m_pArea->IsFlag(REGION_FLAG_SAFE) )
		return WAR_SWING_INVALID;

	int iDist = GetTopDist3D(pCharTarg);
	if ( iDist > GetSight() )
		return IsSetCombatFlags(COMBAT_STAYINRANGE) ? WAR_SWING_EQUIPPING : WAR_SWING_INVALID;

	// I am on ship. Should be able to combat only inside the ship to avoid free sea and ground characters hunting
	if ( (m_pArea != pCharTarg->m_pArea) && !IsSetCombatFlags(COMBAT_ALLOWHITFROMSHIP) )
	{
		if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_SHIP) )
		{
			SysMessageDefault(DEFMSG_COMBAT_OUTSIDESHIP);
			return WAR_SWING_INVALID;
		}
		if ( pCharTarg->m_pArea && pCharTarg->m_pArea->IsFlag(REGION_FLAG_SHIP) )
		{
			SysMessageDefault(DEFMSG_COMBAT_INSIDESHIP);
			return WAR_SWING_INVALID;
		}
	}

	// Fix of the bounce back effect with dir update for clients to be able to run in combat easily
	if ( m_pClient && IsSetCombatFlags(COMBAT_FACECOMBAT) )
	{
		DIR_TYPE dirOpponent = GetDir(pCharTarg, m_dirFace);
		if ( (dirOpponent != m_dirFace) && (dirOpponent != GetDirTurn(m_dirFace, -1)) && (dirOpponent != GetDirTurn(m_dirFace, 1)) )
			return WAR_SWING_READY;
	}

	if ( IsSetCombatFlags(COMBAT_PREHIT) && (m_atFight.m_Swing_State == WAR_SWING_READY) )
	{
		INT64 diff = GetKeyNum("LastHit") - g_World.GetCurrentTime().GetTimeRaw();
		if ( diff > 0 )
		{
			SetTimeout(minimum(diff, 50));
			return WAR_SWING_READY;
		}
	}

	CItem *pWeapon = m_uidWeapon.ItemFind();
	CItem *pAmmo = NULL;
	if ( pWeapon )
	{
		CVarDefCont *pDamTypeOverride = pWeapon->GetKey("OVERRIDE.DAMAGETYPE", true);
		if ( pDamTypeOverride )
			iTyp = static_cast<DAMAGE_TYPE>(pDamTypeOverride->GetValNum());
		else
		{
			CItemBase *pWeaponDef = pWeapon->Item_GetDef();
			switch ( pWeaponDef->GetType() )
			{
				case IT_WEAPON_SWORD:
				case IT_WEAPON_AXE:
				case IT_WEAPON_THROWING:
					iTyp |= DAMAGE_HIT_SLASH;
					break;
				case IT_WEAPON_FENCE:
				case IT_WEAPON_BOW:
				case IT_WEAPON_XBOW:
					iTyp |= DAMAGE_HIT_PIERCE;
					break;
			}
		}
	}

	SKILL_TYPE skill = Skill_GetActive();
	bool bSkillRanged = g_Cfg.IsSkillFlag(skill, SKF_RANGED);
	if ( bSkillRanged )
	{
		if ( IsStatFlag(STATF_HasShield) )		// this should never happen
		{
			SysMessageDefault(DEFMSG_ITEMUSE_BOW_SHIELD);
			return WAR_SWING_INVALID;
		}
		else if ( !IsSetCombatFlags(COMBAT_ARCHERYCANMOVE) && !IsStatFlag(STATF_ArcherCanMove) )
		{
			// Only start swing 1sec after the char stop moving	(TO-DO: add .ini option to customize this delay -> SE:250ms / AOS:500ms / pre-AOS:1000ms)
			if ( m_pClient && -g_World.GetTimeDiff(m_pClient->m_timeLastEventWalk) < TICK_PER_SEC )
				return WAR_SWING_EQUIPPING;
		}

		int	iMinDist = pWeapon ? pWeapon->RangeH() : g_Cfg.m_iArcheryMinDist;
		int	iMaxDist = pWeapon ? pWeapon->RangeL() : g_Cfg.m_iArcheryMaxDist;
		if ( !iMaxDist || (iMinDist == 0 && iMaxDist == 1) )
			iMaxDist = g_Cfg.m_iArcheryMaxDist;
		if ( !iMinDist )
			iMinDist = g_Cfg.m_iArcheryMinDist;

		if ( iDist < iMinDist )
		{
			SysMessageDefault(DEFMSG_COMBAT_ARCH_TOOCLOSE);
			return IsSetCombatFlags(COMBAT_STAYINRANGE) ? WAR_SWING_EQUIPPING : WAR_SWING_READY;
		}
		if ( iDist > iMaxDist )
			return IsSetCombatFlags(COMBAT_STAYINRANGE) ? WAR_SWING_EQUIPPING : WAR_SWING_READY;

		if ( pWeapon )
		{
			RESOURCE_ID_BASE ridAmmo = pWeapon->Weapon_GetRangedAmmoRes();
			if ( ridAmmo )
			{
				pAmmo = pWeapon->Weapon_FindRangedAmmo(ridAmmo);
				if ( !pAmmo && m_pPlayer )
				{
					SysMessageDefault(DEFMSG_COMBAT_ARCH_NOAMMO);
					return WAR_SWING_INVALID;
				}
			}
		}
	}
	else
	{
		int	iMinDist = pWeapon ? pWeapon->RangeH() : 0;
		int	iMaxDist = CalcFightRange(pWeapon);
		if ( iDist < iMinDist || iDist > iMaxDist )
			return IsSetCombatFlags(COMBAT_STAYINRANGE) ? WAR_SWING_EQUIPPING : WAR_SWING_READY;
	}

	// Start the swing
	if ( m_atFight.m_Swing_State == WAR_SWING_READY )
	{
		if ( !CanSeeLOS(pCharTarg) )
			return WAR_SWING_EQUIPPING;

		ANIM_TYPE anim = GenerateAnimate(ANIM_ATTACK_WEAPON);
		int animDelay = 7;		// attack speed is always 7ms and then the char keep waiting the remaining time
		int iSwingDelay = g_Cfg.Calc_CombatAttackSpeed(this, pWeapon) - 1;	// swings are started only on the next tick, so substract -1 to compensate that

		if ( IsTrigUsed(TRIGGER_HITTRY) )
		{
			CScriptTriggerArgs Args(iSwingDelay, 0, pWeapon);
			Args.m_VarsLocal.SetNum("Anim", static_cast<int>(anim));
			Args.m_VarsLocal.SetNum("AnimDelay", animDelay);
			if ( OnTrigger(CTRIG_HitTry, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_READY;

			iSwingDelay = static_cast<int>(Args.m_iN1);
			anim = static_cast<ANIM_TYPE>(Args.m_VarsLocal.GetKeyNum("Anim"));
			animDelay = static_cast<int>(Args.m_VarsLocal.GetKeyNum("AnimDelay"));
			if ( iSwingDelay < 0 )
				iSwingDelay = 0;
			if ( animDelay < 0 )
				animDelay = 0;
		}

		m_atFight.m_Swing_State = WAR_SWING_SWINGING;
		m_atFight.m_Swing_Delay = maximum(0, static_cast<BYTE>(iSwingDelay - animDelay));

		if ( IsSetCombatFlags(COMBAT_PREHIT) )
		{
			SetKeyNum("LastHit", g_World.GetCurrentTime().GetTimeRaw() + iSwingDelay);
			SetTimeout(0);
		}
		else
			SetTimeout(animDelay);

		Reveal();
		if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
			UpdateDir(pCharTarg);
		UpdateAnimate(anim, false, false, static_cast<BYTE>(animDelay / TICK_PER_SEC));
		return WAR_SWING_SWINGING;
	}

	// We made our swing, so we must recoil
	m_atFight.m_Swing_State = WAR_SWING_EQUIPPING;
	m_atFight.m_Swing_NextAction = CServTime::GetCurrentTime() + m_atFight.m_Swing_Delay;

	if ( bSkillRanged && pWeapon )
	{
		ITEMID_TYPE AnimID = ITEMID_NOTHING;
		DWORD AnimHue = 0, AnimRender = 0;
		pWeapon->Weapon_GetRangedAmmoAnim(AnimID, AnimHue, AnimRender);
		pCharTarg->Effect(EFFECT_BOLT, AnimID, this, 18, 1, false, AnimHue, AnimRender);

		if ( m_pClient && (skill == SKILL_THROWING) )		// throwing weapons also have anim of the weapon returning after throw it
		{
			m_pClient->m_timeLastSkillThrowing = CServTime::GetCurrentTime();
			m_pClient->m_pSkillThrowingTarg = pCharTarg;
			m_pClient->m_SkillThrowingAnimID = AnimID;
			m_pClient->m_SkillThrowingAnimHue = AnimHue;
			m_pClient->m_SkillThrowingAnimRender = AnimRender;
		}
	}

	// We missed
	if ( m_Act_Difficulty < 0 )
	{
		if ( IsTrigUsed(TRIGGER_HITMISS) )
		{
			CScriptTriggerArgs Args(0, 0, pWeapon);
			if ( pAmmo )
				Args.m_VarsLocal.SetNum("Arrow", pAmmo->GetUID());
			if ( OnTrigger(CTRIG_HitMiss, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_EQUIPPING;

			if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further!
				pAmmo = NULL;
		}

		if ( pAmmo && m_pPlayer && (40 >= Calc_GetRandVal(100)) )
		{
			pAmmo->UnStackSplit(1);
			pAmmo->MoveToDecay(pCharTarg->GetTopPoint(), g_Cfg.m_iDecay_Item);
		}

		if ( IsPriv(PRIV_DETAIL) )
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSS), pCharTarg->GetName());
		if ( pCharTarg->IsPriv(PRIV_DETAIL) )
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_MISSO), GetName());

		SOUND_TYPE iSound = SOUND_NONE;
		if ( pWeapon )
			iSound = pWeapon->Weapon_GetSoundMiss();
		if ( iSound == SOUND_NONE )
		{
			static const SOUND_TYPE sm_SoundMiss_Wrestling[] = { 0x238, 0x239, 0x23a };
			iSound = sm_SoundMiss_Wrestling[Calc_GetRandVal(COUNTOF(sm_SoundMiss_Wrestling))];
		}
		Sound(iSound);
		return WAR_SWING_EQUIPPING;
	}

	// We hit
	if ( !(iTyp & DAMAGE_GOD) )
	{
		// Check if target will block the hit
		// Legacy pre-SE formula
		CItem *pItemHit = NULL;
		int ParryChance = 0;
		if ( pCharTarg->IsStatFlag(STATF_HasShield) )	// parry using shield
		{
			pItemHit = pCharTarg->LayerFind(LAYER_HAND2);
			ParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 40;
		}
		else if ( pCharTarg->m_uidWeapon.IsItem() )		// parry using weapon
		{
			pItemHit = pCharTarg->m_uidWeapon.ItemFind();
			ParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 80;
		}

		if ( pCharTarg->Skill_GetBase(SKILL_PARRYING) >= 1000 )
			ParryChance += 5;

		int Dex = pCharTarg->Stat_GetAdjusted(STAT_DEX);
		if ( Dex < 80 )
			ParryChance = ParryChance * (20 + Dex) / 100;

		if ( pCharTarg->Skill_UseQuick(SKILL_PARRYING, ParryChance, true, false) )
		{
			if ( IsPriv(PRIV_DETAIL) )
				SysMessageDefault(DEFMSG_COMBAT_PARRY);
			if ( pItemHit )
				pItemHit->OnTakeDamage(1, this, iTyp);

			//Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);		// moved to scripts (@UseQuick on Parrying skill)
			return WAR_SWING_EQUIPPING;
		}
	}

	// Calculate base damage
	int	iDmg = Fight_CalcDamage(pWeapon);
	
	CScriptTriggerArgs Args(iDmg, iTyp, pWeapon);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 40);
	if ( pAmmo )
		Args.m_VarsLocal.SetNum("Arrow", pAmmo->GetUID());

	if ( IsTrigUsed(TRIGGER_SKILLSUCCESS) )
	{
		if ( Skill_OnCharTrigger(skill, CTRIG_SkillSuccess) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return WAR_SWING_EQUIPPING;		// ok, so no hit - skill failed. Pah!
		}
	}
	if ( IsTrigUsed(TRIGGER_SUCCESS) )
	{
		if ( Skill_OnTrigger(skill, SKTRIG_SUCCESS) == TRIGRET_RET_TRUE )
		{
			Skill_Cleanup();
			return WAR_SWING_EQUIPPING;		// ok, so no hit - skill failed. Pah!
		}
	}

	if ( IsTrigUsed(TRIGGER_HIT) )
	{
		if ( OnTrigger(CTRIG_Hit, pCharTarg, &Args) == TRIGRET_RET_TRUE )
			return WAR_SWING_EQUIPPING;

		if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further
			pAmmo = NULL;

		iDmg = static_cast<int>(Args.m_iN1);
		iTyp = static_cast<DAMAGE_TYPE>(Args.m_iN2);
	}

	// BAD BAD Healing fix.. Cant think of something else -- Radiant
	if ( pCharTarg->m_Act_SkillCurrent == SKILL_HEALING )
	{
		pCharTarg->SysMessageDefault(DEFMSG_HEALING_INTERRUPT);
		pCharTarg->Skill_Cleanup();
	}

	if ( pAmmo )
	{
		if ( pCharTarg->m_pNPC && (40 >= Calc_GetRandVal(100)) )
		{
			pAmmo->UnStackSplit(1);
			pCharTarg->ItemBounce(pAmmo, false);
		}
		else
			pAmmo->ConsumeAmount(1);
	}

	SoundChar(CRESND_HIT);

	if ( pWeapon )
	{
		// Check if the weapon is poisoned
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && pWeapon->m_itWeapon.m_poison_skill && pWeapon->m_itWeapon.m_poison_skill > Calc_GetRandVal(100) )
		{
			BYTE iPoisonDeliver = static_cast<BYTE>(Calc_GetRandVal(pWeapon->m_itWeapon.m_poison_skill));
			pCharTarg->SetPoison(10 * iPoisonDeliver, iPoisonDeliver / 5, this);

			pWeapon->m_itWeapon.m_poison_skill -= iPoisonDeliver / 2;	// reduce weapon poison charges
			pWeapon->UpdatePropertyFlag(AUTOTOOLTIP_FLAG_POISON);
		}

		// Check if the weapon will be damaged
		int iDamageChance = static_cast<int>(Args.m_VarsLocal.GetKeyNum("ItemDamageChance"));
		if ( iDamageChance > Calc_GetRandVal(100) )
			pWeapon->OnTakeDamage(iDmg, pCharTarg);
	}
	else if ( NPC_IsMonster() )
	{
		// Base poisoning for NPCs
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && 50 >= Calc_GetRandVal(100) )
		{
			int iPoisoningSkill = Skill_GetBase(SKILL_POISONING);
			if ( iPoisoningSkill )
				pCharTarg->SetPoison(Calc_GetRandVal(iPoisoningSkill), Calc_GetRandVal(iPoisoningSkill / 50), this);
		}
	}

	// Took my swing. Do Damage !
	iDmg = pCharTarg->OnTakeDamage(iDmg, this, iTyp, static_cast<int>(GetDefNum("DAMPHYSICAL", true)), static_cast<int>(GetDefNum("DAMFIRE", true)), static_cast<int>(GetDefNum("DAMCOLD", true)), static_cast<int>(GetDefNum("DAMPOISON", true)), static_cast<int>(GetDefNum("DAMENERGY", true)));

	if ( iDmg > 0 )
	{
		CItem *pCurseWeapon = LayerFind(LAYER_SPELL_Curse_Weapon);
		int iHitLifeLeech = static_cast<int>(GetDefNum("HitLeechLife"));
		if ( pWeapon && pCurseWeapon )
			iHitLifeLeech += pCurseWeapon->m_itSpell.m_spelllevel;

		bool bMakeLeechSound = false;
		if ( iHitLifeLeech )
		{
			iHitLifeLeech = Calc_GetRandVal2(0, (iDmg * iHitLifeLeech * 30) / 10000);	// leech 0% ~ 30% of damage value
			UpdateStatVal(STAT_STR, iHitLifeLeech, Stat_GetMax(STAT_STR));
			bMakeLeechSound = true;
		}

		int iHitManaLeech = static_cast<int>(GetDefNum("HitLeechMana"));
		if ( iHitManaLeech )
		{
			iHitManaLeech = Calc_GetRandVal2(0, (iDmg * iHitManaLeech * 40) / 10000);	// leech 0% ~ 40% of damage value
			UpdateStatVal(STAT_INT, iHitManaLeech, Stat_GetMax(STAT_INT));
			bMakeLeechSound = true;
		}

		if ( GetDefNum("HitLeechStam") > Calc_GetRandLLVal(100) )
		{
			UpdateStatVal(STAT_DEX, iDmg, Stat_GetMax(STAT_DEX));	// leech 100% of damage value
			bMakeLeechSound = true;
		}

		int iManaDrain = 0;
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && pPoly->m_itSpell.m_spell == SPELL_Wraith_Form )
				iManaDrain += 5 + (15 * Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);
		}
		if ( GetDefNum("HitManaDrain") > Calc_GetRandLLVal(100) )
			iManaDrain += IMULDIV(iDmg, 20, 100);	// leech 20% of damage value

		int iTargMana = pCharTarg->Stat_GetVal(STAT_INT);
		if ( iManaDrain > iTargMana )
			iManaDrain = iTargMana;

		if ( iManaDrain > 0 )
		{
			pCharTarg->UpdateStatVal(STAT_INT, iTargMana - iManaDrain);
			UpdateStatVal(STAT_INT, iManaDrain, Stat_GetMax(STAT_INT));
			bMakeLeechSound = true;
		}

		if ( bMakeLeechSound )
			Sound(0x44d);

		// Make blood effects
		if ( pCharTarg->m_wBloodHue != static_cast<HUE_TYPE>(-1) )
		{
			static const ITEMID_TYPE sm_Blood[] = { ITEMID_BLOOD1, ITEMID_BLOOD2, ITEMID_BLOOD3, ITEMID_BLOOD4, ITEMID_BLOOD5, ITEMID_BLOOD6, ITEMID_BLOOD_SPLAT };
			int iBloodQty = (g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE) ? Calc_GetRandVal2(4, 5) : Calc_GetRandVal2(1, 2);

			for ( int i = 0; i < iBloodQty; i++ )
			{
				ITEMID_TYPE iBloodID = sm_Blood[Calc_GetRandVal(COUNTOF(sm_Blood))];
				CItem *pBlood = CItem::CreateBase(iBloodID);
				ASSERT(pBlood);
				pBlood->SetHue(pCharTarg->m_wBloodHue);
				pBlood->MoveNear(pCharTarg->GetTopPoint(), 1);
				pBlood->SetDecayTime(5 * TICK_PER_SEC);
			}
		}

		// Check for passive skill gain
		if ( m_pPlayer && !pCharTarg->m_pArea->IsFlag(REGION_FLAG_NO_PVP) )
		{
			Skill_Experience(skill, m_Act_Difficulty);
			Skill_Experience(SKILL_TACTICS, m_Act_Difficulty);
		}
	}

	return WAR_SWING_EQUIPPING;
}
