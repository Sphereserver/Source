#include "graysvr.h"	// predef header
#include "../network/send.h"

CItemStone *CChar::Guild_Find(MEMORY_TYPE MemType) const
{
	ADDTOCALLSTACK("CChar::Guild_Find");
	// Get my guild stone for my guild
	// ARGS:
	//  MemType = MEMORY_GUILD or MEMORY_TOWN

	if ( !m_pPlayer )
		return NULL;

	CItemMemory *pMemory = Memory_FindTypes(static_cast<WORD>(MemType));
	if ( !pMemory )
		return NULL;

	CItemStone *pStone = dynamic_cast<CItemStone *>(pMemory->m_uidLink.ItemFind());
	if ( !pStone )
	{
		// Memory not linked to a valid stone, just clear it
		const_cast<CChar *>(this)->Memory_ClearTypes(static_cast<WORD>(MemType));
		return NULL;
	}
	return pStone;
}

CStoneMember *CChar::Guild_FindMember(MEMORY_TYPE MemType) const
{
	ADDTOCALLSTACK("CChar::Guild_FindMember");
	// Get my member record for my guild

	CItemStone *pStone = Guild_Find(MemType);
	if ( !pStone )
		return NULL;

	CStoneMember *pMember = pStone->GetMember(this);
	if ( !pMember )
	{
		// Memory not linked to a valid stone, just clear it
		const_cast<CChar *>(this)->Memory_ClearTypes(static_cast<WORD>(MemType));
		return NULL;
	}
	return pMember;
}

void CChar::Guild_Resign(MEMORY_TYPE MemType)
{
	ADDTOCALLSTACK("CChar::Guild_Resign");
	// Resign me from my guild

	CStoneMember *pMember = Guild_FindMember(MemType);
	if ( pMember )
		delete pMember;
}

LPCTSTR CChar::Guild_Abbrev(MEMORY_TYPE MemType) const
{
	ADDTOCALLSTACK("CChar::Guild_Abbrev");
	// Get my guild abbrev (if I have chosen to turn it on)

	CStoneMember *pMember = Guild_FindMember(MemType);
	if ( !pMember || !pMember->IsAbbrevOn() )
		return NULL;

	CItemStone *pStone = pMember->GetParentStone();
	if ( !pStone || !pStone->GetAbbrev()[0] )
		return NULL;

	return pStone->GetAbbrev();
}

///////////////////////////////////////////////////////////

bool CChar::Noto_IsMurderer() const
{
	ADDTOCALLSTACK("CChar::Noto_IsMurderer");
	return (m_pPlayer && (m_pPlayer->m_wMurders > g_Cfg.m_iMurderMinCount));
}

bool CChar::Noto_IsEvil() const
{
	ADDTOCALLSTACK("CChar::Noto_IsEvil");
	int iKarma = Stat_GetAdjusted(STAT_KARMA);

	if ( m_pArea && m_pArea->IsGuarded() && m_pArea->m_TagDefs.GetKeyNum("RED") )
	{
		// Red zone is opposite to blue: evil is normal and non-evil is bad people
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

NOTO_TYPE CChar::Noto_GetFlag(const CChar *pCharViewer, bool fAllowInvul, bool fGetColor) const
{
	ADDTOCALLSTACK("CChar::Noto_GetFlag");
	if ( !pCharViewer )
		return NOTO_INVALID;

	CChar *pSrc = const_cast<CChar *>(this);
	if ( g_Cfg.m_iPetsInheritNotoriety && m_pNPC && NPC_PetGetOwner() )	// If I'm a pet and have owner I redirect noto to him.
		pSrc = NPC_PetGetOwner();

	CChar *pTarget = const_cast<CChar *>(pCharViewer);
	int iNotoSaveID = pSrc->NotoSave_GetID(pTarget);
	if ( iNotoSaveID != -1 )
	{
		NotoSaves refNoto = pSrc->m_notoSaves.at(iNotoSaveID);
		return fGetColor ? refNoto.color : refNoto.value;
	}

	NOTO_TYPE iNoto = NOTO_INVALID;
	NOTO_TYPE iColor = NOTO_INVALID;

	if ( IsTrigUsed(TRIGGER_NOTOSEND) )
	{
		CScriptTriggerArgs Args;
		pSrc->OnTrigger(CTRIG_NotoSend, pTarget, &Args);
		iNoto = static_cast<NOTO_TYPE>(Args.m_iN1);
		iColor = static_cast<NOTO_TYPE>(Args.m_iN2);
		if ( iNoto < NOTO_INVALID )
			iNoto = NOTO_INVALID;
	}

	if ( iNoto == NOTO_INVALID )
		iNoto = Noto_CalcFlag(pTarget, fAllowInvul);
	if ( iColor == NOTO_INVALID )
		iColor = iNoto;

	pSrc->NotoSave_Add(pTarget, iNoto, iColor);
	return fGetColor ? iColor : iNoto;
}

NOTO_TYPE CChar::Noto_CalcFlag(const CChar *pCharViewer, bool fAllowInvul) const
{
	ADDTOCALLSTACK("CChar::Noto_CalcFlag");
	if ( !pCharViewer )
		return NOTO_INVALID;

	NOTO_TYPE NotoFlag = static_cast<NOTO_TYPE>(m_TagDefs.GetKeyNum("OVERRIDE.NOTO"));
	if ( NotoFlag != NOTO_INVALID )
		return NotoFlag;

	if ( IsStatFlag(STATF_Incognito) )
		return NOTO_NEUTRAL;

	if ( fAllowInvul && IsStatFlag(STATF_INVUL) )
		return NOTO_INVUL;

	if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_ARENA) )	// everyone is neutral here
		return NOTO_NEUTRAL;

	if ( pCharViewer != this )
	{
		if ( g_Cfg.m_iPetsInheritNotoriety != 0 )
		{
			const CChar *pMaster = NPC_PetGetOwner();
			if ( pMaster && (pMaster != pCharViewer) )	// master doesn't want to see their own status
			{
				// Protect against infinite loop
				static int sm_iReentrant = 0;
				if ( sm_iReentrant < 32 )
				{
					// Get master's notoriety
					++sm_iReentrant;
					NOTO_TYPE NotoMaster = pMaster->Noto_GetFlag(pCharViewer, fAllowInvul);
					--sm_iReentrant;

					// Check if this notoriety is inheritable based on bitmask settings
					int iNotoFlags = 1 << (NotoMaster - 1);
					if ( (g_Cfg.m_iPetsInheritNotoriety & iNotoFlags) == iNotoFlags )
						return NotoMaster;
				}
				else
				{
					// Too many owners, return the notoriety for however far we got down the chain
					DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring notoriety towards %s UID=0%" FMTDWORDH "\n", pMaster->GetName(), pMaster->GetUID().GetPrivateUID()));
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
			const CItemStone *pMyGuild = Guild_Find(MEMORY_GUILD);
			if ( pMyGuild )
			{
				const CItemStone *pViewerGuild = pCharViewer->Guild_Find(MEMORY_GUILD);
				if ( pViewerGuild )
				{
					if ( (pViewerGuild == pMyGuild) || pMyGuild->IsAlliedWith(pViewerGuild) )
						return NOTO_GUILD_SAME;
					if ( pMyGuild->IsAtWarWith(pViewerGuild) )
						return NOTO_GUILD_WAR;
				}
			}

			// Check the town stuff
			const CItemStone *pMyTown = Guild_Find(MEMORY_TOWN);
			if ( pMyTown )
			{
				const CItemStone *pViewerTown = pCharViewer->Guild_Find(MEMORY_TOWN);
				if ( pViewerTown )
				{
					if ( pMyTown->IsAtWarWith(pViewerTown) )
						return NOTO_GUILD_WAR;
				}
			}
		}

		// If they saw me commit a crime or I am their aggressor then criminal to just them
		const CItemMemory *pMemory = pCharViewer->Memory_FindObjTypes(this, MEMORY_SAWCRIME|MEMORY_AGGREIVED|MEMORY_HARMEDBY);
		if ( pMemory )
			return NOTO_CRIMINAL;
	}

	if ( IsStatFlag(STATF_Criminal) )	// criminal to everyone
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

	if ( !IsPriv(PRIV_PRIV_NOSHOW) )
	{
		if ( IsPriv(PRIV_GM) )
		{
			switch ( GetPrivLevel() )
			{
				case PLEVEL_Owner:
					return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_OWNER);
				case PLEVEL_Admin:
					return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_ADMIN);
				case PLEVEL_Dev:
					return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_DEV);
				case PLEVEL_GM:
					return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_GM);
			}
		}
		switch ( GetPrivLevel() )
		{
			case PLEVEL_Seer:
				return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_SEER);
			case PLEVEL_Counsel:
				return g_Cfg.GetDefaultMsg(DEFMSG_TITLE_COUNSEL);
		}
	}

	if ( (m_pPlayer || !g_Cfg.m_NPCNoFameTitle) && (Stat_GetAdjusted(STAT_FAME) >= 10000) )
		return g_Cfg.GetDefaultMsg(Char_GetDef()->IsFemale() ? DEFMSG_TITLE_LADY : DEFMSG_TITLE_LORD);

	return "";
}

int CChar::Noto_GetLevel() const
{
	ADDTOCALLSTACK("CChar::Noto_GetLevel");

	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	size_t iKarmaLevel = 0;
	while ( (iKarmaLevel < g_Cfg.m_NotoKarmaLevels.GetCount()) && (iKarma < g_Cfg.m_NotoKarmaLevels.GetAt(iKarmaLevel)) )
		++iKarmaLevel;

	int iFame = Stat_GetAdjusted(STAT_FAME);
	size_t iFameLevel = 0;
	while ( (iFameLevel < g_Cfg.m_NotoFameLevels.GetCount()) && (iFame > g_Cfg.m_NotoFameLevels.GetAt(iFameLevel)) )
		++iFameLevel;

	return (iKarmaLevel * (g_Cfg.m_NotoFameLevels.GetCount() + 1)) + iFameLevel;
}

LPCTSTR CChar::Noto_GetTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetTitle");

	LPCTSTR pszTitle = Noto_IsMurderer() ? g_Cfg.GetDefaultMsg(DEFMSG_TITLE_MURDERER) : (IsStatFlag(STATF_Criminal) ? g_Cfg.GetDefaultMsg(DEFMSG_TITLE_CRIMINAL) : g_Cfg.GetNotoTitle(Noto_GetLevel(), Char_GetDef()->IsFemale()));
	LPCTSTR pszFameTitle = GetKeyStr("NAME.PREFIX");
	if ( !*pszFameTitle )
		pszFameTitle = Noto_GetFameTitle();

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s%s%s%s%s%s",
		pszTitle[0] ? g_Cfg.GetDefaultMsg(Char_GetDef()->IsFemale() ? DEFMSG_TITLE_ARTICLE_FEMALE : DEFMSG_TITLE_ARTICLE_MALE) : "",
		pszTitle,
		pszTitle[0] ? " " : "",
		pszFameTitle,
		GetName(),
		GetKeyStr("NAME.SUFFIX"));

	return pszTemp;
}

void CChar::Noto_Murder()
{
	ADDTOCALLSTACK("CChar::Noto_Murder");
	if ( Noto_IsMurderer() )
		SysMessageDefault(DEFMSG_MSG_MURDERER);

	if ( m_pPlayer && m_pPlayer->m_wMurders )
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Murders, 0, g_Cfg.m_iMurderDecayTime);
}

void CChar::Noto_Criminal(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::Noto_Criminal");
	if ( m_pNPC || IsPriv(PRIV_GM) )
		return;

	int iDecay = g_Cfg.m_iCriminalTimer;

	if ( IsTrigUsed(TRIGGER_CRIMINAL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = iDecay;
		Args.m_pO1 = pChar;
		if ( OnTrigger(CTRIG_Criminal, this, &Args) == TRIGRET_RET_TRUE )
			return;

		iDecay = static_cast<int>(Args.m_iN1);
	}

	CItem *pMemory = LayerFind(LAYER_FLAG_Criminal);
	if ( pMemory )
		pMemory->SetTimeout(iDecay);
	else
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Criminal, 0, iDecay);
}

#define NOTO_DEGREES	8
#define NOTO_FACTOR		(300 / NOTO_DEGREES)

void CChar::Noto_ChangeDeltaMsg(int iDelta, LPCTSTR pszType)
{
	ADDTOCALLSTACK("CChar::Noto_ChangeDeltaMsg");
	if ( !iDelta )
		return;

	static LPCTSTR const sm_szNotoDelta[8] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_1),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_2),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_3),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_4),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_5),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_6),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_7),
		g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_8)
	};

	int iDegree = minimum(abs(iDelta) / NOTO_FACTOR, 7);
	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_CHANGE_0), g_Cfg.GetDefaultMsg((iDelta > 0) ? DEFMSG_MSG_NOTO_CHANGE_GAIN : DEFMSG_MSG_NOTO_CHANGE_LOST), sm_szNotoDelta[iDegree], pszType);
}

void CChar::Noto_ChangeNewMsg(int iPrvLevel)
{
	ADDTOCALLSTACK("CChar::Noto_ChangeNewMsg");
	if ( iPrvLevel != Noto_GetLevel() )		// reached a new title level?
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_NOTO_GETTITLE), Noto_GetTitle());
}

void CChar::Noto_Fame(int iFameChange)
{
	ADDTOCALLSTACK("CChar::Noto_Fame");
	if ( !iFameChange )
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
		CScriptTriggerArgs Args(iFameChange);	// ARGN1 = fame change modifier
		if ( OnTrigger(CTRIG_FameChange, this, &Args) == TRIGRET_RET_TRUE )
			return;
		iFameChange = static_cast<int>(Args.m_iN1);
	}

	if ( !iFameChange )
		return;

	iFame += iFameChange;
	Noto_ChangeDeltaMsg(iFame - Stat_GetAdjusted(STAT_FAME), g_Cfg.GetDefaultMsg(DEFMSG_NOTO_FAME));
	Stat_SetBase(STAT_FAME, iFame);
}

void CChar::Noto_Karma(int iKarmaChange, int iBottom, bool fMessage)
{
	ADDTOCALLSTACK("CChar::Noto_Karma");

	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	iKarmaChange = g_Cfg.Calc_KarmaScale(iKarma, iKarmaChange);

	if ( iKarmaChange > 0 )
	{
		if ( iKarma + iKarmaChange > g_Cfg.m_iMaxKarma )
			iKarmaChange = g_Cfg.m_iMaxKarma - iKarma;
	}
	else
	{
		if ( iBottom == INT_MIN )
			iBottom = g_Cfg.m_iMinKarma;
		if ( iKarma + iKarmaChange < iBottom )
			iKarmaChange = iBottom - iKarma;
	}

	if ( IsTrigUsed(TRIGGER_KARMACHANGE) )
	{
		CScriptTriggerArgs Args(iKarmaChange);	// ARGN1 - Karma change modifier
		if ( OnTrigger(CTRIG_KarmaChange, this, &Args) == TRIGRET_RET_TRUE )
			return;
		iKarmaChange = static_cast<int>(Args.m_iN1);
	}

	if ( !iKarmaChange )
		return;

	iKarma += iKarmaChange;
	Noto_ChangeDeltaMsg(iKarma - Stat_GetAdjusted(STAT_KARMA), g_Cfg.GetDefaultMsg(DEFMSG_NOTO_KARMA));
	Stat_SetBase(STAT_KARMA, iKarma);
	NotoSave_Update();
	if ( fMessage )
	{
		int iPrvLevel = Noto_GetLevel();
		Noto_ChangeNewMsg(iPrvLevel);
	}
}

extern unsigned int Calc_ExpGet_Exp(unsigned int);

void CChar::Noto_Kill(CChar *pKill, bool fPetKill, int iTotalKillers)
{
	ADDTOCALLSTACK("CChar::Noto_Kill");
	if ( !pKill )
		return;

	// What was there noto to me?
	NOTO_TYPE NotoThem = pKill->Noto_GetFlag(this);

	// Fight is over now that i have won (if i was fighting at all)
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
	else if ( m_pPlayer && (NotoThem < NOTO_GUILD_SAME) )
	{
		// The player murdered someone innocent
		if ( !IsPriv(PRIV_GM) )
		{
			CScriptTriggerArgs Args;
			Args.m_iN1 = static_cast<INT64>(m_pPlayer->m_wMurders) + 1;
			Args.m_iN2 = true;

			if ( IsTrigUsed(TRIGGER_MURDERMARK) )
			{
				OnTrigger(CTRIG_MurderMark, this, &Args);
				if ( Args.m_iN1 < 0 )
					Args.m_iN1 = 0;
			}

			m_pPlayer->m_wMurders = static_cast<WORD>(Args.m_iN1);
			NotoSave_Update();
			if ( Args.m_iN2 )
				Noto_Criminal();

			Noto_Murder();
		}
	}

	// No fame/karma/exp gain on these conditions
	if ( fPetKill || (NotoThem == NOTO_GUILD_SAME) || pKill->IsStatFlag(STATF_Conjured) || (pKill->m_pNPC && pKill->m_pNPC->m_bonded) )
		return;

	int iPrvLevel = Noto_GetLevel();	// store title before fame/karma changes to check if it got changed
	Noto_Fame(g_Cfg.Calc_FameKill(pKill) / iTotalKillers);
	Noto_Karma(g_Cfg.Calc_KarmaKill(pKill, NotoThem) / iTotalKillers);

	if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_RAISE_COMBAT) )
	{
		int iChange = (pKill->m_exp / 10) / iTotalKillers;
		if ( iChange )
			iChange = (iChange * ((m_pPlayer && pKill->m_pPlayer) ? g_Cfg.m_iExperienceKoefPVP : g_Cfg.m_iExperienceKoefPVM)) / 100;

		if ( iChange )
		{
			// Bonuses of different experiences
			if ( (m_exp * 4) < pKill->m_exp )		// 200%		[exp < 1/4 of killed]
				iChange *= 2;
			else if ( (m_exp * 2) < pKill->m_exp )	// 150%		[exp < 1/2 of killed]
				iChange = (iChange * 3) / 2;
			else if ( m_exp <= pKill->m_exp )		// 100%		[exp <= killed]
				;
			else if ( m_exp < (pKill->m_exp * 2) )	//  50%		[exp < 2 * killed]
				iChange /= 2;
			else if ( m_exp < (pKill->m_exp * 3) )	//  25%		[exp < 3 * killed]
				iChange /= 4;
			else									//  10%		[exp >= 3 * killed]
				iChange /= 10;
		}

		ChangeExperience(iChange, pKill);
	}

	Noto_ChangeNewMsg(iPrvLevel);	// inform any title changes
}

void CChar::NotoSave_Add(CChar *pChar, NOTO_TYPE value, NOTO_TYPE color)
{
	ADDTOCALLSTACK("CChar::NotoSave_Add");
	if ( !pChar )
		return;

	NotoSaves refNoto;
	refNoto.charUID = pChar->GetUID();
	refNoto.elapsed = 0;
	refNoto.value = value;
	refNoto.color = color;
	m_notoSaves.push_back(refNoto);
}

void CChar::NotoSave_Delete(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NotoSave_Delete");
	if ( !pChar )
		return;

	for ( size_t i = 0; i < m_notoSaves.size(); ++i )
	{
		NotoSaves &refNoto = m_notoSaves.at(i);
		if ( refNoto.charUID == pChar->GetUID() )
		{
			m_notoSaves.erase(m_notoSaves.begin() + i);
			return;
		}
	}
}

void CChar::NotoSave_Clear()
{
	ADDTOCALLSTACK("CChar::NotoSave_Clear");
	m_notoSaves.clear();
}

void CChar::NotoSave_Update()
{
	ADDTOCALLSTACK("CChar::NotoSave_Update");
	NotoSave_Clear();
	Update();
}

int CChar::NotoSave_GetID(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::NotoSave_GetID");
	if ( pChar )
	{
		for ( size_t i = 0; i < m_notoSaves.size(); ++i )
		{
			NotoSaves &refNoto = m_notoSaves.at(i);
			if ( refNoto.charUID == pChar->GetUID() )
				return i;
		}
	}
	return -1;
}

void CChar::NotoSave_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::NotoSave_CheckTimeout");

	for ( size_t i = 0; i < m_notoSaves.size(); ++i )
	{
		NotoSaves &refNoto = m_notoSaves.at(i);
		if ( ++refNoto.elapsed > g_Cfg.m_iNotoTimeout )
		{
			m_notoSaves.erase(m_notoSaves.begin() + i);
			return;
		}
	}
}

///////////////////////////////////////////////////////////

bool CChar::Memory_UpdateFlags(CItemMemory *pMemory)
{
	ADDTOCALLSTACK("CChar::Memory_UpdateFlags");
	// Reset the check timer based on the given pMemory
	ASSERT(pMemory);

	WORD wMemTypes = pMemory->GetMemoryTypes();
	if ( wMemTypes & MEMORY_IPET )
		StatFlag_Set(STATF_Pet);

	INT64 iCheckTime;
	if ( wMemTypes & MEMORY_FIGHT )		// update more often to check for retreat
		iCheckTime = 30 * TICK_PER_SEC;
	else if ( wMemTypes & (MEMORY_IPET|MEMORY_GUARD|MEMORY_GUILD|MEMORY_TOWN) )
		iCheckTime = -1;	// never go away
	else if ( m_pNPC )	// MEMORY_SPEAK
		iCheckTime = 5 * 60 * TICK_PER_SEC;
	else
		iCheckTime = 20 * 60 * TICK_PER_SEC;
	pMemory->SetTimeout(iCheckTime);

	CChar *pCharLink = pMemory->m_uidLink.CharFind();
	if ( pCharLink )
	{
		pCharLink->NotoSave_Update();	// clear my notoriety from the target
		NotoSave_Update();				// iAggressor is stored in the other char, so the call should be reverted
	}
	return true;
}

void CChar::Memory_AddTypes(CItemMemory *pMemory, WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::Memory_AddTypes");
	// Add flag to the given pMemory

	if ( pMemory )
	{
		pMemory->SetMemoryTypes(pMemory->GetMemoryTypes() | wMemTypes);
		pMemory->m_itEqMemory.m_pt = GetTopPoint();		// where did the fight start?
		pMemory->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());
		Memory_UpdateFlags(pMemory);
	}
}

bool CChar::Memory_ClearTypes(CItemMemory *pMemory, WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	// Clear flag from the given pMemory

	if ( pMemory )
	{
		if ( wMemTypes & MEMORY_IPET )
			StatFlag_Clear(STATF_Pet);

		if ( pMemory->SetMemoryTypes(pMemory->GetMemoryTypes() & ~wMemTypes) == 0 )
			pMemory->Delete();
		else
			Memory_UpdateFlags(pMemory);

		return true;
	}
	return false;
}

CItemMemory *CChar::Memory_CreateObj(CGrayUID uid, WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::Memory_CreateObj");
	// Create a memory about this object
	// NOTE: Does not check if object already has a memory (assume it does not)

	CItemMemory *pMemory = dynamic_cast<CItemMemory *>(CItem::CreateBase(ITEMID_MEMORY));
	if ( !pMemory )
		return NULL;

	pMemory->SetType(IT_EQ_MEMORY_OBJ);
	pMemory->SetAttr(ATTR_NEWBIE);
	pMemory->m_uidLink = uid;

	Memory_AddTypes(pMemory, wMemTypes);
	LayerAdd(pMemory, LAYER_SPECIAL);
	return pMemory;
}

void CChar::Memory_ClearTypes(WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	// Remove all the memories of this type

	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !pItem->IsMemoryTypes(wMemTypes) )
			continue;

		CItemMemory *pMemory = dynamic_cast<CItemMemory *>(pItem);
		if ( !pMemory )
			continue;

		Memory_ClearTypes(pMemory, wMemTypes);
	}
}

CItemMemory *CChar::Memory_FindObj(CGrayUID uid) const
{
	ADDTOCALLSTACK("CChar::Memory_FindObj");
	// Do I have a memory/link for this object?

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) || (pItem->m_uidLink != uid) )
			continue;
		return static_cast<CItemMemory *>(pItem);
	}
	return NULL;
}

CItemMemory *CChar::Memory_FindTypes(WORD wMemTypes) const
{
	ADDTOCALLSTACK("CChar::Memory_FindTypes");
	// Do I have a certain type of memory?
	// Just find the first one

	if ( wMemTypes )
	{
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( !pItem->IsMemoryTypes(wMemTypes) )
				continue;
			return static_cast<CItemMemory *>(pItem);
		}
	}
	return NULL;
}

TRIGRET_TYPE CChar::OnCharTrigForMemTypeLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::OnCharTrigForMemTypeLoop");
	// Loop through all memories (ForCharMemoryType)

	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;
	TRIGRET_TYPE iRet;

	if ( wMemTypes )
	{
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
			if ( !pItem->IsMemoryTypes(wMemTypes) )
				continue;

			iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
				return iRet;

			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext(StartContext);
		}
	}

	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// Just skip to the end
		iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);

	return TRIGRET_ENDIF;
}

CItemMemory *CChar::Memory_AddObjTypes(CGrayUID uid, WORD wMemTypes)
{
	ADDTOCALLSTACK("CChar::Memory_AddObjTypes");
	// Adding a new value for this memory, updating notoriety

	CItemMemory *pMemory = Memory_FindObj(uid);
	if ( !pMemory )
		return Memory_CreateObj(uid, wMemTypes);

	Memory_AddTypes(pMemory, wMemTypes);
	NotoSave_Delete(uid.CharFind());
	return pMemory;
}

bool CChar::Memory_OnTick(CItemMemory *pMemory)
{
	ADDTOCALLSTACK("CChar::Memory_OnTick");
	// NOTE: Do not return true unless u update the timer
	// RETURN:
	//  false = delete this memory
	ASSERT(pMemory);

	CObjBase *pObj = pMemory->m_uidLink.ObjFind();
	if ( !pObj )
		return false;

	if ( pMemory->IsMemoryTypes(MEMORY_FIGHT) )
		return Memory_Fight_OnTick(pMemory);

	return pMemory->IsMemoryTypes(MEMORY_IPET|MEMORY_GUARD|MEMORY_GUILD|MEMORY_TOWN);
}

//////////////////////////////////////////////////////////////////////////////

void CChar::OnNoticeCrime(CChar *pCriminal, const CChar *pCharMark)
{
	ADDTOCALLSTACK("CChar::OnNoticeCrime");
	// I noticed a crime

	if ( !pCriminal || !pCharMark || (pCriminal == this) || (pCriminal == pCharMark) || pCriminal->IsPriv(PRIV_GM) || (pCriminal->GetNPCBrain(false) == NPCBRAIN_GUARD) )
		return;

	NOTO_TYPE iNoto = pCharMark->Noto_GetFlag(pCriminal);
	if ( (iNoto == NOTO_CRIMINAL) || (iNoto == NOTO_EVIL) )
		return;

	if ( m_pPlayer )
	{
		// I have the option of attacking the criminal. or calling the guards.
		bool fCriminal = true;
		if ( IsTrigUsed(TRIGGER_SEECRIME) )
		{
			CScriptTriggerArgs Args;
			Args.m_iN1 = fCriminal;
			Args.m_pO1 = const_cast<CChar *>(pCharMark);
			OnTrigger(CTRIG_SeeCrime, pCriminal, &Args);
			fCriminal = Args.m_iN1 ? true : false;
		}
		if ( fCriminal )
			Memory_AddObjTypes(pCriminal, MEMORY_SAWCRIME);
		return;
	}

	// NPCs can take other actions
	ASSERT(m_pNPC);

	if ( this != pCharMark )
	{
		if ( NPC_IsOwnedBy(pCriminal) )		// don't report its own master
			return;
	}
	else
	{
		// I being the victim can retaliate.
		Memory_AddObjTypes(pCriminal, MEMORY_SAWCRIME);
		OnHarmedBy(pCriminal);
	}

	if ( m_pArea && m_pArea->IsGuarded() )
	{
		if ( GetNPCBrain(false) == NPCBRAIN_GUARD )
		{
			// Guard NPCs should attack the criminal
			NPC_LookAtCharGuard(pCriminal);
		}
		else if ( (GetNPCBrain() == NPCBRAIN_HUMAN) && NPC_CanSpeak() )
		{
			// Human NPCs should call for guards
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_CRIM));
			CallGuards(pCriminal);
		}
	}
}

bool CChar::CheckCrimeSeen(SKILL_TYPE skill, CChar *pCharMark, const CObjBase *pItem, LPCTSTR pszAction)
{
	ADDTOCALLSTACK("CChar::CheckCrimeSeen");
	// I'm commiting a crime
	// Did others see me commit or try to commit the crime
	// RETURN:
	//  true = someone saw me

	bool fSeen = false;
	CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIZE);
	for (;;)
	{
		CChar *pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )
			continue;
		if ( !pChar->CanSeeLOS(this, LOS_NB_WINDOWS) )	// what if I was standing behind a window when I saw a crime? :)
			continue;
		if ( pChar->GetPrivLevel() >= PLEVEL_Counsel )	// if a GM sees you it's not a crime
			continue;

		fSeen = true;
		bool fYour = (pChar == pCharMark);

		TCHAR *pszMsg = Str_GetTemp();
		if ( pItem && pszAction )
		{
			if ( pCharMark )
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_2), GetName(), pszAction, fYour ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_YOUR) : pCharMark->GetName(), fYour ? "" : g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_S), pItem->GetName());
			else
				sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_MSG_YOUNOTICE_1), GetName(), pszAction, pItem->GetName());
			pChar->ObjMessage(pszMsg, this);
		}

		// They are not a criminal til someone calls the guards
		if ( skill == SKILL_SNOOPING )
		{
			if ( IsTrigUsed(TRIGGER_SEESNOOP) )
			{
				CScriptTriggerArgs Args(pszAction);
				Args.m_iN1 = skill;
				Args.m_iN2 = pItem ? static_cast<DWORD>(pItem->GetUID()) : 0;
				Args.m_pO1 = pCharMark;

				TRIGRET_TYPE iRet = pChar->OnTrigger(CTRIG_SeeSnoop, this, &Args);
				if ( iRet == TRIGRET_RET_TRUE )
					continue;
				if ( iRet == TRIGRET_RET_DEFAULT )
				{
					if ( !g_Cfg.Calc_CrimeSeen(this, pChar, skill, fYour) )
						continue;
				}
			}

			// Off chance of being a criminal. (hehe)
			if ( Calc_GetRandVal(100) < g_Cfg.m_iSnoopCriminal )
				pChar->OnNoticeCrime(this, pCharMark);
			if ( pChar->m_pNPC )
				pChar->NPC_OnNoticeSnoop(this, pCharMark);
		}
		else
			pChar->OnNoticeCrime(this, pCharMark);
	}
	return fSeen;
}

bool CChar::Skill_Snoop_Check(const CItemContainer *pItem)
{
	ADDTOCALLSTACK("CChar::Skill_Snoop_Check");
	// Assume the container is not locked
	// RETURN:
	//  true = snoop or can't open at all
	//  false = instant open

	if ( !pItem )
		return true;

	ASSERT(pItem->IsItem());
	if ( !g_Cfg.m_iTradeWindowSnooping && pItem->IsContainer() && pItem->IsItemInTrade() )
		return false;

	if ( !IsPriv(PRIV_GM) )
	{
		switch ( pItem->GetType() )
		{
			case IT_CONTAINER_LOCKED:
			case IT_SHIP_HOLD_LOCK:
				if ( !GetContainerCreate(LAYER_PACK)->ContentFindKeyFor(dynamic_cast<CItem *>(const_cast<CItemContainer *>(pItem))) )
				{
					SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_LOCKED));
					return true;
				}
				break;
			case IT_EQ_BANK_BOX:
				// Some sort of cheater
				return false;
		}
	}

	CChar *pCharMark;
	if ( !IsTakeCrime(pItem, &pCharMark) || !pCharMark )
		return false;

	if ( Skill_Wait(SKILL_SNOOPING) )
		return true;

	m_Act_Targ = pItem->GetUID();
	Skill_Start(SKILL_SNOOPING);
	return true;
}

int CChar::Skill_Snooping(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Snooping");
	// SKILL_SNOOPING
	// m_Act_Targ = object to snoop into.
	// RETURN:
	//  -SKTRIG_QTY = no chance. and not a crime
	//  -SKTRIG_FAIL = no chance and caught
	//  0-100 = difficulty = percent chance of failure

	if ( stage == SKTRIG_STROKE )
		return 0;

	// Assume the container is not locked
	CItemContainer *pCont = dynamic_cast<CItemContainer *>(m_Act_Targ.ItemFind());
	if ( !pCont )
		return -SKTRIG_QTY;

	CChar *pCharMark;
	if ( !IsTakeCrime(pCont, &pCharMark) || !pCharMark )
		return 0;	// not really a crime

	if ( GetTopDist3D(pCharMark) > 1 )
	{
		SysMessageDefault(DEFMSG_SNOOPING_REACH);
		return -SKTRIG_QTY;
	}

	if ( !CanTouch(pCont) )
	{
		SysMessageDefault(DEFMSG_SNOOPING_CANT);
		return -SKTRIG_QTY;
	}

	if ( stage == SKTRIG_START )
	{
		if ( GetPrivLevel() < pCharMark->GetPrivLevel() )
		{
			SysMessageDefault(DEFMSG_SNOOPING_CANT);
			return -SKTRIG_QTY;
		}

		// Return the difficulty
		return (Skill_GetAdjusted(SKILL_SNOOPING) < Calc_GetRandVal(1000)) ? 100 : 0;
	}

	// Did someone see this?
	CheckCrimeSeen(SKILL_SNOOPING, pCharMark, pCont, g_Cfg.GetDefaultMsg(DEFMSG_SNOOPING_ATTEMPTING));
	Noto_Karma(-4, INT_MIN, true);

	if ( stage == SKTRIG_SUCCESS )
	{
		// Open the container
		if ( m_pClient )
			m_pClient->addContainerSetup(pCont);
	}
	else if ( stage == SKTRIG_FAIL )
	{
		SysMessageDefault(DEFMSG_SNOOPING_FAILED);
		if ( Skill_GetAdjusted(SKILL_HIDING) / 2 < Calc_GetRandVal(1000) )
			Reveal();
	}
	return 0;
}

int CChar::Skill_Stealing(SKTRIG_TYPE stage)
{
	ADDTOCALLSTACK("CChar::Skill_Stealing");
	// SKILL_STEALING
	// m_Act_Targ = object to steal
	// RETURN:
	//  -SKTRIG_QTY = no chance. and not a crime
	//  -SKTRIG_FAIL = no chance and caught
	//  0-100 = difficulty = percent chance of failure

	if ( stage == SKTRIG_STROKE )
		return 0;

	CChar *pCharMark = NULL;
	CItem *pItem = m_Act_Targ.ItemFind();
	if ( !pItem )
	{
		// If targ is an char, steal a random item from its backpack
		pCharMark = m_Act_Targ.CharFind();
		if ( !pCharMark )
		{
			SysMessageDefault(DEFMSG_STEALING_NOTHING);
			return -SKTRIG_QTY;
		}
		CItemContainer *pPack = pCharMark->GetContainer(LAYER_PACK);
		if ( !pPack )
		{
		cantsteal:
			SysMessageDefault(DEFMSG_STEALING_EMPTY);
			return -SKTRIG_QTY;
		}

		pItem = pPack->GetAt(Calc_GetRandVal(pPack->GetCount()));		// random item on backpack
		if ( !pItem )
			goto cantsteal;

		m_Act_Targ = pItem->GetUID();
	}

	// Special cases
	CItem *pItemTop = dynamic_cast<CItem *>(pItem->GetTopLevelObj());
	if ( pItemTop )
	{
		if ( pItemTop->IsType(IT_CORPSE) )
		{
			SysMessageDefault(DEFMSG_STEALING_CORPSE);
			return -SKTRIG_ABORT;
		}
		if ( pItemTop->IsType(IT_EQ_TRADE_WINDOW) )
		{
			SysMessageDefault(DEFMSG_STEALING_TRADE);
			return -SKTRIG_ABORT;
		}
		if ( pItemTop->IsType(IT_GAME_BOARD) )
		{
			SysMessageDefault(DEFMSG_STEALING_GAMEBOARD);
			return -SKTRIG_ABORT;
		}
	}
	if ( pItem->IsType(IT_TRAIN_PICKPOCKET) )
	{
		SysMessageDefault(DEFMSG_STEALING_PICKPOCKET);
		return -SKTRIG_QTY;
	}
	if ( !CanTouch(pItem) )
	{
		SysMessageDefault(DEFMSG_STEALING_REACH);
		return -SKTRIG_ABORT;
	}
	if ( !CanMove(pItem) || !CanCarry(pItem) )
	{
		SysMessageDefault(DEFMSG_STEALING_HEAVY);
		return -SKTRIG_ABORT;
	}
	if ( !IsTakeCrime(pItem, &pCharMark) )
	{
		SysMessageDefault(DEFMSG_STEALING_NONEED);
		return -SKTRIG_QTY;
	}
	if ( m_pArea->IsFlag(REGION_FLAG_SAFE) )
	{
		SysMessageDefault(DEFMSG_STEALING_STOP);
		return -SKTRIG_QTY;
	}

	Reveal();

	bool fGround = false;
	if ( pCharMark )
	{
		if ( GetTopDist3D(pCharMark) > UO_MAP_DIST_INTERACT )
		{
			SysMessageDefault(DEFMSG_STEALING_MARK);
			return -SKTRIG_QTY;
		}
		if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pCharMark->m_pPlayer && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault(DEFMSG_STEALING_SAFE);
			return -1;
		}
		if ( GetPrivLevel() < pCharMark->GetPrivLevel() )
		{
			return -SKTRIG_FAIL;
		}
		if ( stage == SKTRIG_START )
		{
			return g_Cfg.Calc_StealingItem(this, pItem, pCharMark);
		}
	}
	else
	{
		// Stealing off the ground should always succeed
		// It's just a matter of getting caught
		if ( stage == SKTRIG_START )
			return 1;

		fGround = true;
	}

	// Deliver the goods
	if ( (stage == SKTRIG_SUCCESS) || fGround )
	{
		pItem->ClrAttr(ATTR_OWNED);
		CItemContainer *pPack = GetContainer(LAYER_PACK);
		if ( pPack && (pItem->GetParent() != pPack) )
		{
			pItem->RemoveFromView();
			pPack->ContentAdd(pItem);
		}
	}

	if ( m_Act_Difficulty > 0 )
	{
		// You should only be able to go down to -1000 karma by stealing
		if ( CheckCrimeSeen(SKILL_STEALING, pCharMark, pItem, g_Cfg.GetDefaultMsg((stage == SKTRIG_FAIL) ? DEFMSG_STEALING_YOUR : DEFMSG_STEALING_SOMEONE)) )
			Noto_Karma(-100, -1000, true);
	}
	return 0;
}

void CChar::CallGuards()
{
	ADDTOCALLSTACK("CChar::CallGuards");
	if ( !m_pArea || !m_pArea->IsGuarded() || IsStatFlag(STATF_DEAD) )
		return;
	if ( -g_World.GetTimeDiff(m_timeLastCallGuards) < TICK_PER_SEC )	// flood check to avoid waste CPU performance with excessive calls of this function
		return;

	// We don't have any target yet, let's check everyone nearby
	CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIZE);
	for (;;)
	{
		CChar *pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )
			continue;
		if ( !pChar->m_pArea->IsGuarded() )
			continue;
		if ( !CanDisturb(pChar) )	// don't allow guards to be called on someone we can't disturb
			continue;

		// Mark person as criminal if I saw him criming
		// Only players call guards this way. NPC's flag criminal instantly
		if ( m_pPlayer && Memory_FindObjTypes(pChar, MEMORY_SAWCRIME) )
			pChar->Noto_Criminal();

		if ( pChar->IsStatFlag(STATF_Criminal) || (g_Cfg.m_fGuardsOnMurderers && pChar->Noto_IsEvil()) )
			CallGuards(pChar);
	}
	m_timeLastCallGuards = CServTime::GetCurrentTime();
}

void CChar::CallGuards(CChar *pCriminal)
{
	ADDTOCALLSTACK("CChar::CallGuards2");
	if ( !pCriminal || (pCriminal == this) )
		return;
	if ( !m_pArea || !pCriminal->m_pArea || !pCriminal->m_pArea->IsGuarded() || IsStatFlag(STATF_DEAD) )
		return;
	if ( pCriminal->IsStatFlag(STATF_INVUL|STATF_DEAD) || pCriminal->IsPriv(PRIV_GM|PRIV_JAILED) )
		return;

	// Search for a wandering guard nearby
	CChar *pGuard = NULL;
	CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIZE);
	for (;;)
	{
		CChar *pChar = AreaChars.GetChar();
		if ( !pChar )
			break;

		if ( pChar->m_pNPC && (pChar->m_pNPC->m_Brain == NPCBRAIN_GUARD) && !pChar->IsStatFlag(STATF_War) )
		{
			pGuard = pChar;
			break;
		}
	}

	CVarDefCont *pVar = pCriminal->m_pArea->m_TagDefs.GetKey("OVERRIDE.GUARDS");
	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_CHARDEF, pVar ? pVar->GetValStr() : "GUARDS");
	if ( IsTrigUsed(TRIGGER_CALLGUARDS) )
	{
		CScriptTriggerArgs Args(pGuard);
		Args.m_iN1 = rid.GetResIndex();
		Args.m_iN2 = 0;
		Args.m_VarObjs.Insert(1, pCriminal, true);

		if ( OnTrigger(CTRIG_CallGuards, pCriminal, &Args) == TRIGRET_RET_TRUE )
			return;

		if ( Args.m_iN1 != rid.GetResIndex() )
			rid = RESOURCE_ID(RES_CHARDEF, static_cast<int>(Args.m_iN1));
		if ( Args.m_iN2 > 0 )	// if set to 1, a new guard will be spawned regardless of whether a nearby guard is available
			pGuard = NULL;
	}

	if ( !pGuard )		// spawn a new guard
	{
		if ( !rid.IsValidUID() )
			return;
		pGuard = CChar::CreateNPC(static_cast<CREID_TYPE>(rid.GetResIndex()));
		if ( !pGuard || !pGuard->m_pNPC )
			return;

		if ( pCriminal->m_pArea->m_TagDefs.GetKeyNum("RED") )
			pGuard->m_TagDefs.SetNum("NAME.HUE", g_Cfg.m_iColorNotoEvil, true);
		pGuard->Spell_Effect_Create(SPELL_Summon, LAYER_SPELL_Summon, 1000, g_Cfg.m_iGuardLingerTime);
		pGuard->Spell_Teleport(pCriminal->GetTopPoint(), false, false);
		pGuard->m_ptHome = GetTopPoint();
		pGuard->m_pNPC->m_Home_Dist_Wander = UO_MAP_VIEW_SIZE;
	}
	pGuard->NPC_LookAtCharGuard(pCriminal);
}

void CChar::OnHarmedBy(CChar *pCharSrc)
{
	ADDTOCALLSTACK("CChar::OnHarmedBy");
	// I notice a crime or attack against me
	// Actual harm has taken place
	// Attack back

	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY);
	if ( Fight_IsActive() && m_Fight_Targ.CharFind() )		// already fighting
	{
		if ( m_pPlayer )
			return;
		if ( Calc_GetRandVal(10) )	// check if NPC will keep attacking this same target or switch to another one
			return;
	}

	if ( !IsSetCombatFlags(COMBAT_NOPETDESERT) && NPC_IsOwnedBy(pCharSrc, false) )
		NPC_PetRelease();

	Fight_Attack(pCharSrc);
}

bool CChar::OnAttackedBy(CChar *pCharSrc, bool fCommandPet, bool fShouldReveal)
{
	ADDTOCALLSTACK("CChar::OnAttackedBy");
	// We have been attacked by pCharSrc
	// Might not actually be doing any real damage (yet)
	// RETURN:
	//  true = ok
	//  false = we are immune to this char (or they to us)

	if ( !pCharSrc || (pCharSrc == this) )
		return true;
	if ( IsStatFlag(STATF_DEAD) )
		return false;

	if ( fShouldReveal )
		pCharSrc->Reveal();

	if ( Fight_IsActive() && (m_Fight_Targ == pCharSrc->GetUID()) )		// already fighting with this target
		return true;

	Memory_AddObjTypes(pCharSrc, MEMORY_HARMEDBY|MEMORY_IRRITATEDBY);
	Attacker_Add(pCharSrc);

	// Are they a criminal for it? Is attacking me a crime?
	if ( Noto_GetFlag(pCharSrc) == NOTO_GOOD )
	{
		if ( m_pClient )
		{
			// I decide if this is a crime
			OnNoticeCrime(pCharSrc, this);
		}
		else
		{
			// If it is a pet then this a crime others can report
			pCharSrc->CheckCrimeSeen(Skill_GetActive(), IsStatFlag(STATF_Pet) ? NPC_PetGetOwner() : this, NULL, NULL);
		}
	}

	if ( !fCommandPet )
		OnHarmedBy(pCharSrc);	// possibly retaliate (auto defend)

	return true;
}

enum ARMOR_TYPE
{
	ARMOR_HEAD,
	ARMOR_NECK,
	ARMOR_BACK,
	ARMOR_CHEST,
	ARMOR_ARMS,
	ARMOR_HANDS,
	ARMOR_LEGS,
	ARMOR_FEET,
	ARMOR_QTY
};

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
	const LAYER_TYPE *m_pLayers;
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

WORD CChar::CalcArmorDefense() const
{
	ADDTOCALLSTACK("CChar::CalcArmorDefense");
	// Calculate total AR equipped on char
	// Only used by old AR property (new RES* properties will update RES* values directly)

	WORD wArmorCount = 0;
	WORD wDefense = 0;
	WORD wDefenseTotal = 0;
	WORD wArmorRegionMax[ARMOR_QTY];
	for ( int i = 0; i < ARMOR_QTY; ++i )
		wArmorRegionMax[i] = 0;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		wDefense = static_cast<WORD>(pItem->Armor_GetDefense());
		if ( !wDefense && !pItem->IsType(IT_SPELL) )
			continue;

		// Reverse of sm_ArmorLayers
		switch ( pItem->GetEquipLayer() )
		{
			case LAYER_HELM:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_HEAD] += wDefense;
				else
					wArmorRegionMax[ARMOR_HEAD] = maximum(wArmorRegionMax[ARMOR_HEAD], wDefense);
				break;
			case LAYER_COLLAR:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_NECK] += wDefense;
				else
					wArmorRegionMax[ARMOR_NECK] = maximum(wArmorRegionMax[ARMOR_NECK], wDefense);
				break;
			case LAYER_SHIRT:
			case LAYER_CHEST:
			case LAYER_TUNIC:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					wArmorRegionMax[ARMOR_CHEST] += wDefense;
					wArmorRegionMax[ARMOR_BACK] += wDefense;
				}
				else
				{
					wArmorRegionMax[ARMOR_CHEST] = maximum(wArmorRegionMax[ARMOR_CHEST], wDefense);
					wArmorRegionMax[ARMOR_BACK] = maximum(wArmorRegionMax[ARMOR_BACK], wDefense);
				}
				break;
			case LAYER_ARMS:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_ARMS] += wDefense;
				else
					wArmorRegionMax[ARMOR_ARMS] = maximum(wArmorRegionMax[ARMOR_ARMS], wDefense);
				break;
			case LAYER_PANTS:
			case LAYER_SKIRT:
			case LAYER_HALF_APRON:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_LEGS] += wDefense;
				else
					wArmorRegionMax[ARMOR_LEGS] = maximum(wArmorRegionMax[ARMOR_LEGS], wDefense);
				break;
			case LAYER_SHOES:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_FEET] += wDefense;
				else
					wArmorRegionMax[ARMOR_FEET] = maximum(wArmorRegionMax[ARMOR_FEET], wDefense);
				break;
			case LAYER_GLOVES:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
					wArmorRegionMax[ARMOR_HANDS] += wDefense;
				else
					wArmorRegionMax[ARMOR_HANDS] = maximum(wArmorRegionMax[ARMOR_HANDS], wDefense);
				break;
			case LAYER_CAPE:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					wArmorRegionMax[ARMOR_BACK] += wDefense;
					wArmorRegionMax[ARMOR_ARMS] += wDefense;
				}
				else
				{
					wArmorRegionMax[ARMOR_BACK] = maximum(wArmorRegionMax[ARMOR_BACK], wDefense);
					wArmorRegionMax[ARMOR_ARMS] = maximum(wArmorRegionMax[ARMOR_ARMS], wDefense);
				}
				break;
			case LAYER_ROBE:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					wArmorRegionMax[ARMOR_CHEST] += wDefense;
					wArmorRegionMax[ARMOR_BACK] += wDefense;
					wArmorRegionMax[ARMOR_ARMS] += wDefense;
					wArmorRegionMax[ARMOR_LEGS] += wDefense;
				}
				else
				{
					wArmorRegionMax[ARMOR_CHEST] = maximum(wArmorRegionMax[ARMOR_CHEST], wDefense);
					wArmorRegionMax[ARMOR_BACK] = maximum(wArmorRegionMax[ARMOR_BACK], wDefense);
					wArmorRegionMax[ARMOR_ARMS] = maximum(wArmorRegionMax[ARMOR_ARMS], wDefense);
					wArmorRegionMax[ARMOR_LEGS] = maximum(wArmorRegionMax[ARMOR_LEGS], wDefense);
				}
				break;
			case LAYER_LEGS:
				if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
				{
					wArmorRegionMax[ARMOR_LEGS] += wDefense;
					wArmorRegionMax[ARMOR_FEET] += wDefense;
				}
				else
				{
					wArmorRegionMax[ARMOR_LEGS] = maximum(wArmorRegionMax[ARMOR_LEGS], wDefense);
					wArmorRegionMax[ARMOR_FEET] = maximum(wArmorRegionMax[ARMOR_FEET], wDefense);
				}
				break;
			case LAYER_HAND2:
				if ( pItem->IsType(IT_SHIELD) )
				{
					if ( IsSetCombatFlags(COMBAT_STACKARMOR) )
						wArmorRegionMax[ARMOR_HANDS] += wDefense;
					else
						wArmorRegionMax[ARMOR_HANDS] = maximum(wArmorRegionMax[ARMOR_HANDS], wDefense);
				}
				break;
			case LAYER_SPELL_Protection:
				wDefenseTotal += pItem->m_itSpell.m_spelllevel * 100;
				break;
			default:
				continue;
		}
		++wArmorCount;
	}

	if ( wArmorCount )
	{
		for ( int i = 0; i < ARMOR_QTY; ++i )
			wDefenseTotal += sm_ArmorLayers[i].m_wCoverage * wArmorRegionMax[i];
	}

	return maximum(0, (wDefenseTotal / 100) + m_ModAr);
}

int CChar::OnTakeDamage(int iDmg, CChar *pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy)
{
	ADDTOCALLSTACK("CChar::OnTakeDamage");
	// Someone hit us
	// iDmg is already defined, here we just apply armor related calculations
	// ARGS:
	//  iDmg[Physical/Fire/Cold/Poison/Energy]: % of each type to split the damage into (eg iDmgPhysical=70 + iDmgCold=30 will split iDmg value into 70% physical + 30% cold)
	//  uType: damage flags (DAMAGE_GOD, DAMAGE_HIT_BLUNT, etc)
	// RETURN: damage done
	//  -1 = already dead / invalid target
	//  0 = no damage
	//  INT_MAX	= killed

	if ( IsStatFlag(STATF_DEAD) )	// already dead
		return -1;

	if ( !pSrc )
		pSrc = this;

	if ( !(uType & DAMAGE_GOD) )
	{
		if ( IsStatFlag(STATF_INVUL|STATF_Stone) )
		{
		effect_bounce:
			Effect(EFFECT_OBJ, ITEMID_FX_GLOW, this, 10, 16);
			return 0;
		}
		if ( (uType & DAMAGE_FIRE) && Can(CAN_C_FIRE_IMMUNE) )
			goto effect_bounce;
		if ( m_pArea )
		{
			if ( m_pArea->IsFlag(REGION_FLAG_SAFE) )
				goto effect_bounce;
			if ( m_pArea->IsFlag(REGION_FLAG_NO_PVP) && pSrc && ((IsStatFlag(STATF_Pet) && (NPC_PetGetOwner() == pSrc)) || (m_pPlayer && (pSrc->m_pPlayer || pSrc->IsStatFlag(STATF_Pet)))) )
				goto effect_bounce;
		}
	}

	// Notoriety checks
	if ( !OnAttackedBy(pSrc, false, !(uType & DAMAGE_NOREVEAL)) )
		return 0;

	// Apply Necromancy cursed effects
	if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
	{
		CItem *pEvilOmen = LayerFind(LAYER_SPELL_Evil_Omen);
		if ( pEvilOmen )
		{
			iDmg += iDmg / 4;
			pEvilOmen->Delete();
		}

		CItem *pBloodOath = LayerFind(LAYER_SPELL_Blood_Oath);
		if ( pBloodOath && (pBloodOath->m_uidLink == pSrc->GetUID()) && !(uType & DAMAGE_FIXED) )	// if DAMAGE_FIXED is set we are already receiving a reflected damage, so we must stop here to avoid an infinite loop
		{
			iDmg += iDmg / 10;
			pSrc->OnTakeDamage(iDmg * (100 - pBloodOath->m_itSpell.m_spelllevel) / 100, this, DAMAGE_MAGIC|DAMAGE_FIXED);
		}
	}

	CCharBase *pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// Apply armor calculation
	if ( !(uType & (DAMAGE_GOD|DAMAGE_FIXED)) && !((uType & DAMAGE_MAGIC) && IsSetMagicFlags(MAGICF_IGNOREAR)) )
	{
		if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
		{
			// AOS elemental combat
			if ( iDmgPhysical == 0 )		// if physical damage is not set, assume it as the remaining value
				iDmgPhysical = 100 - (iDmgFire + iDmgCold + iDmgPoison + iDmgEnergy);

			int iPhysicalDamage = iDmg * iDmgPhysical * (100 - (m_ResPhysicalMax ? minimum(m_ResPhysical, m_ResPhysicalMax) : m_ResPhysical));
			int iFireDamage = iDmg * iDmgFire * (100 - (m_ResFireMax ? minimum(m_ResFire, m_ResFireMax) : m_ResFire));
			int iColdDamage = iDmg * iDmgCold * (100 - (m_ResColdMax ? minimum(m_ResCold, m_ResColdMax) : m_ResCold));
			int iPoisonDamage = iDmg * iDmgPoison * (100 - (m_ResPoisonMax ? minimum(m_ResPoison, m_ResPoisonMax) : m_ResPoison));
			int iEnergyDamage = iDmg * iDmgEnergy * (100 - (m_ResEnergyMax ? minimum(m_ResEnergy, m_ResEnergyMax) : m_ResEnergy));

			iDmg = (iPhysicalDamage + iFireDamage + iColdDamage + iPoisonDamage + iEnergyDamage) / 10000;
		}
		else
		{
			// pre-AOS armor rating (AR)
			int iArMax = (pCharDef->m_defense + m_defense) * Calc_GetRandVal(7, 35) / 100;
			int iArMin = iArMax / 2;

			int iDef = Calc_GetRandVal(iArMin, (iArMax - iArMin) + 1);
			if ( uType & DAMAGE_MAGIC )		// magical damage halves effectiveness of defense
				iDef /= 2;

			iDmg -= iDef;
		}
	}

	CScriptTriggerArgs Args(iDmg, uType, static_cast<INT64>(0));
	if ( !(uType & DAMAGE_POISON) )
	{
		Args.m_VarsLocal.SetNum("ItemDamageLayer", sm_ArmorDamageLayers[Calc_GetRandVal(COUNTOF(sm_ArmorDamageLayers))]);
		Args.m_VarsLocal.SetNum("ItemDamageChance", 25);
	}

	if ( IsTrigUsed(TRIGGER_GETHIT) )
	{
		if ( OnTrigger(CTRIG_GetHit, pSrc, &Args) == TRIGRET_RET_TRUE )
			return 0;

		iDmg = static_cast<int>(Args.m_iN1);
		uType = static_cast<DAMAGE_TYPE>(Args.m_iN2);
	}

	// Check if the armor will be damaged
	LAYER_TYPE iItemDamageLayer = static_cast<LAYER_TYPE>(Args.m_VarsLocal.GetKeyNum("ItemDamageLayer"));
	if ( iItemDamageLayer && (Args.m_VarsLocal.GetKeyNum("ItemDamageChance") > Calc_GetRandVal(100)) && !pCharDef->Can(CAN_C_NONHUMANOID) )
	{
		CItem *pItemHit = LayerFind(iItemDamageLayer);
		if ( pItemHit )
			pItemHit->OnTakeDamage(iDmg, pSrc, uType);
	}

	// Remove stuck/paralyze effect
	if ( !(uType & DAMAGE_NOUNPARALYZE) )
	{
		CItem *pParalyze = LayerFind(LAYER_SPELL_Paralyze);
		if ( pParalyze )
			pParalyze->Delete();

		CItem *pStuck = LayerFind(LAYER_FLAG_Stuck);
		if ( pStuck )
			pStuck->Delete();

		if ( IsStatFlag(STATF_Freeze) )
		{
			StatFlag_Clear(STATF_Freeze);
			if ( m_pClient )
				m_pClient->addCharMove(this);
		}
	}

	if ( pSrc != this )
	{
		// Disturb magic spells (only players can be disturbed)
		if ( m_pPlayer && !(uType & DAMAGE_NODISTURB) && g_Cfg.IsSkillFlag(Skill_GetActive(), SKF_MAGIC) )
		{
			// Check if my spell can be interrupted
			int iSkill;
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_atMagery.m_Spell);
			if ( pSpellDef && pSpellDef->GetPrimarySkill(&iSkill) )
			{
				int iDisturbChance = pSpellDef->m_Interrupt.GetLinear(Skill_GetBase(static_cast<SKILL_TYPE>(iSkill)));
				if ( iDisturbChance )
				{
					if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
					{
						// Protection spell can cancel the disturb
						CItem *pProtectionSpell = LayerFind(LAYER_SPELL_Protection);
						if ( pProtectionSpell && (pProtectionSpell->m_itSpell.m_spelllevel > static_cast<WORD>(Calc_GetRandVal(1000))) )
							iDisturbChance = 0;
					}

					if ( iDisturbChance > Calc_GetRandVal(1000) )
						Skill_Fail();
				}
			}
		}

		if ( pSrc )
		{
			// Update attacker list
			int iAttackerID = Attacker_GetID(pSrc);
			if ( iAttackerID != -1 )
			{
				LastAttackers &refAttacker = m_lastAttackers.at(iAttackerID);
				refAttacker.elapsed = 0;
				refAttacker.damage += maximum(0, iDmg);
				refAttacker.threat += maximum(0, iDmg);
			}

			// A physical blow of some sort
			if ( uType & (DAMAGE_HIT_BLUNT|DAMAGE_HIT_PIERCE|DAMAGE_HIT_SLASH) )
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
						pSrc->OnTakeDamage(iReactiveDamage, this, static_cast<DAMAGE_TYPE>(DAMAGE_FIXED), iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
						pSrc->Sound(0x1F1);
						pSrc->Effect(EFFECT_OBJ, ITEMID_FX_CURSE_EFFECT, this, 10, 16);
					}
				}
			}
		}
	}

	if ( iDmg <= 0 )
		return 0;

	// Apply damage
	SoundChar(CRESND_GETHIT);
	UpdateStatVal(STAT_STR, -iDmg);
	if ( pSrc->m_pClient )
		pSrc->m_pClient->addHitsUpdate(this);		// always send updates to src

	if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_DAMAGE )
	{
		if ( m_pClient )
			m_pClient->addShowDamage(GetUID(), iDmg);
		else if ( m_pNPC )
		{
			const CChar *pOwner = NPC_PetGetOwner();
			if ( pOwner && pOwner->m_pClient )
				pOwner->m_pClient->addShowDamage(GetUID(), iDmg);
		}

		if ( pSrc->m_pClient && (pSrc != this) )
			pSrc->m_pClient->addShowDamage(GetUID(), iDmg);
		else if ( pSrc->m_pNPC )
		{
			const CChar *pSrcOwner = pSrc->NPC_PetGetOwner();
			if ( pSrcOwner && pSrcOwner->m_pClient )
				pSrcOwner->m_pClient->addShowDamage(GetUID(), iDmg);
		}
	}

	if ( Stat_GetVal(STAT_STR) <= 0 )
	{
		// We will die from this. Make sure the killer is set correctly, otherwise the person we are currently attacking will get credit for killing us.
		m_Fight_Targ = pSrc->GetUID();
		return iDmg;
	}

	if ( m_pNPC && IsStatFlag(STATF_Hovering) )		// flying NPCs should land when get hit
		ToggleFlying();
	else if ( m_atFight.m_Swing_State != WAR_SWING_SWINGING )	// don't interrupt swing animation
		UpdateAnimate(ANIM_GET_HIT);

	return iDmg;
}

void CChar::OnTakeDamageArea(int iDmg, CChar *pSrc, DAMAGE_TYPE uType, int iDmgPhysical, int iDmgFire, int iDmgCold, int iDmgPoison, int iDmgEnergy, HUE_TYPE effectHue, SOUND_TYPE effectSound)
{
	ADDTOCALLSTACK("CChar::OnTakeDamageArea");

	bool fMakeSound = false;
	CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIGHT);
	for (;;)
	{
		CChar *pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( (pChar == this) || (pChar == pSrc) )
			continue;

		pChar->OnTakeDamage(iDmg, pSrc, uType, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy);
		pChar->Effect(EFFECT_OBJ, ITEMID_FX_SPARKLE_2, this, 1, 15, false, effectHue);
		fMakeSound = true;
	}
	if ( fMakeSound && (effectSound != SOUND_NONE) )
		Sound(effectSound);
}

///////////////////////////////////////////////////////////
// Fight related memories

bool CChar::Memory_Fight_OnTick(CItemMemory *pMemory)
{
	ADDTOCALLSTACK("CChar::Memory_Fight_OnTick");
	// Tick fight memory status
	// RETURN:
	//  true = skip it
	//  false = delete the memory

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

void CChar::Memory_Fight_Start(CChar *pTarg)
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Start");
	// I'm attacking this creature
	// I might be the aggressor or just retaliating
	// This is just the "intent" to fight, maybe no damage done yet

	ASSERT(pTarg);
	if ( Fight_IsActive() && (m_Fight_Targ == pTarg->GetUID()) )		// already fighting with this target
		return;

	WORD wMemTypes = 0;
	CItemMemory *pMemory = Memory_FindObj(pTarg);
	if ( !pMemory )
	{
		// No memory created, so the fight will start now
		CItemMemory *pTargMemory = pTarg->Memory_FindObj(this);
		if ( pTargMemory )
		{
			// The target remembers me
			if ( pTargMemory->IsMemoryTypes(MEMORY_IAGGRESSOR) )
				wMemTypes = MEMORY_HARMEDBY;
			else if ( pTargMemory->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED) )
				wMemTypes = MEMORY_IAGGRESSOR;
		}
		else
		{
			// I'm the agressor
			wMemTypes = MEMORY_IAGGRESSOR;
		}
		pMemory = Memory_CreateObj(pTarg, MEMORY_FIGHT|wMemTypes);
	}
	else
	{
		if ( Attacker_GetID(const_cast<CChar *>(pTarg)) )	// target is already on my attacker list, no need of more code
			return;
		if ( !pMemory->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED) )	// I am defending myself rightly
			wMemTypes = MEMORY_IAGGRESSOR;

		Memory_AddTypes(pMemory, MEMORY_FIGHT|wMemTypes);	// update the fight status
	}

	if ( m_pClient && (m_Fight_Targ == pTarg->GetUID()) && !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
	{
		// This may be a useless command. How do I say the fight is over?
		// This causes the funny turn to the target during combat!
		new PacketSwing(m_pClient, pTarg);
	}

	if ( IsSetCombatFlags(COMBAT_ATTACKONNOTICE) )
	{
		// Make target instantly attack back
		if ( pTarg->m_pClient )
			pTarg->m_pClient->Event_Attack(GetUID());
		else
			pTarg->OnHarmedBy(this);
	}
}

///////////////////////////////////////////////////////////

SKILL_TYPE CChar::Fight_GetWeaponSkill() const
{
	ADDTOCALLSTACK("CChar::Fight_GetWeaponSkill");

	CItem *pWeapon = m_uidWeapon.ItemFind();
	if ( pWeapon )
		return pWeapon->Weapon_GetSkill();

	return SKILL_WRESTLING;
}

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
	return !IsStatFlag(STATF_INVUL|STATF_DEAD|STATF_Invisible|STATF_Insubstantial|STATF_Stone|STATF_Hidden);
}

int CChar::Fight_CalcDamage(const CItem *pWeapon, bool fNoRandom, bool fGetMax) const
{
	ADDTOCALLSTACK("CChar::Fight_CalcDamage");
	// Calculate attack damage (also used to show damage on char status gump)

	int iDmgMin = 0;
	int iDmgMax = 0;

	if ( pWeapon )
	{
		iDmgMin = pWeapon->Weapon_GetAttack(false);
		iDmgMax = pWeapon->Weapon_GetAttack(true);
	}
	else
	{
		iDmgMin = m_attackBase;
		iDmgMax = iDmgMin + m_attackRange;

		// Horrific Beast (necro spell) changes char base damage to 5-15
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && (pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast) )
			{
				iDmgMin += pPoly->m_itSpell.m_PolyStr;
				iDmgMax += pPoly->m_itSpell.m_PolyDex;
			}
		}
	}

	if ( m_pPlayer )	// only players can have damage bonus
	{
		int iDmgBonus = minimum(m_DamIncrease, 100);		// Damage Increase is capped at 100%

		// Racial bonus (Berserk), gargoyles gains +15% Damage Increase per each 20 HP lost
		if ( (g_Cfg.m_iRacialFlags & RACIALF_GARG_BERSERK) && IsGargoyle() )
			iDmgBonus += minimum(15 * ((Stat_GetMax(STAT_STR) - Stat_GetVal(STAT_STR)) / 20), 60);		// value is capped at 60%

		// Horrific Beast (necro spell) add +25% Damage Increase
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && (pPoly->m_itSpell.m_spell == SPELL_Horrific_Beast) )
				iDmgBonus += 25;
		}

		STAT_TYPE iStatBonus = static_cast<STAT_TYPE>(GetDefNum("COMBATBONUSSTAT"));
		int iStatBonusPercent = static_cast<int>(GetDefNum("COMBATBONUSPERCENT"));

		switch ( g_Cfg.m_iCombatDamageEra )
		{
			default:
			case 0:
			{
				// Sphere custom formula
				if ( !iStatBonus )
					iStatBonus = STAT_STR;
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

				if ( pWeapon && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( !iStatBonus )
					iStatBonus = STAT_STR;
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

				if ( pWeapon && pWeapon->IsType(IT_WEAPON_AXE) )
				{
					iDmgBonus += Skill_GetBase(SKILL_LUMBERJACKING) / 50;
					if ( Skill_GetBase(SKILL_LUMBERJACKING) >= 1000 )
						iDmgBonus += 10;
				}

				if ( Stat_GetAdjusted(STAT_STR) >= 100 )
					iDmgBonus += 5;

				if ( !iStatBonus )
					iStatBonus = STAT_STR;
				if ( !iStatBonusPercent )
					iStatBonusPercent = 30;
				iDmgBonus += Stat_GetAdjusted(iStatBonus) * iStatBonusPercent / 100;
				break;
			}
		}

		iDmgMin += iDmgMin * iDmgBonus / 100;
		iDmgMax += iDmgMax * iDmgBonus / 100;
	}

	if ( fNoRandom )
		return fGetMax ? iDmgMax : iDmgMin;
	else
		return Calc_GetRandVal(iDmgMin, iDmgMax);
}

void CChar::Fight_Clear()
{
	ADDTOCALLSTACK("CChar::Fight_Clear");
	// Clear all active targets and toggle out war mode

	if ( IsTrigUsed(TRIGGER_COMBATEND) )
		OnTrigger(CTRIG_CombatEnd, this);

	m_lastAttackers.clear();
	if ( Fight_IsActive() )
	{
		Skill_Start(SKILL_NONE);
		m_Fight_Targ.InitUID();
	}

	if ( m_pNPC )
	{
		if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )
			SetSight(GetSight() - 10);

		StatFlag_Clear(STATF_War);
		UpdateModeFlag();
	}
}

bool CChar::Fight_Attack(CChar *pCharTarg, bool fToldByMaster)
{
	ADDTOCALLSTACK("CChar::Fight_Attack");
	// We want to attack some one
	// But they won't notice til we actually hit them
	// This is just my intent
	// RETURN:
	//  true = new attack is accepted

	if ( !pCharTarg || (pCharTarg == this) || (m_pNPC && !pCharTarg->Fight_IsAttackable()) || IsStatFlag(STATF_DEAD) )
	{
		// Not a valid target
		Attacker_Delete(const_cast<CChar *>(pCharTarg), true);
		return false;
	}

	if ( g_Cfg.m_fAttackingIsACrime && (pCharTarg->Noto_GetFlag(this) == NOTO_GOOD) )
		CheckCrimeSeen(SKILL_NONE, pCharTarg, NULL, NULL);

	INT64 iThreat = fToldByMaster ? LLONG_MAX : 0;

	if ( (IsTrigUsed(TRIGGER_ATTACK) || IsTrigUsed(TRIGGER_CHARATTACK)) && (m_Fight_Targ != pCharTarg->GetUID()) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = iThreat;
		if ( OnTrigger(CTRIG_Attack, pCharTarg, &Args) == TRIGRET_RET_TRUE )
			return false;
		iThreat = Args.m_iN1;
	}

	if ( !Attacker_Add(pCharTarg, iThreat) )
		return false;

	// I'm attacking (or defending)
	if ( !IsStatFlag(STATF_War) )
	{
		if ( m_pNPC )
		{
			if ( IsStatFlag(STATF_Hovering) )	// flying NPCs should land when enter warmode
				ToggleFlying();
			if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )		// increase view range during combat to make NPC follow targets for longer distances
				SetSight(GetSight() + 10);
		}

		StatFlag_Set(STATF_War);
		UpdateModeFlag();
		if ( m_pClient )
			m_pClient->addPlayerWarMode();
	}

	SKILL_TYPE skillActive = Skill_GetActive();
	SKILL_TYPE skillWeapon = Fight_GetWeaponSkill();

	if ( (skillActive == skillWeapon) && (m_Fight_Targ == pCharTarg->GetUID()) )	// already attacking this same target using the same skill
		return true;
	else if ( g_Cfg.IsSkillFlag(skillActive, SKF_MAGIC) )	// don't start another fight skill when already casting spells
		return true;

	if ( m_pNPC && !fToldByMaster )
		pCharTarg = NPC_FightFindBestTarget();

	m_Fight_Targ = pCharTarg ? pCharTarg->GetUID() : static_cast<CGrayUID>(UID_UNUSED);
	Skill_Start(skillWeapon);
	return true;
}

void CChar::Fight_HitTry()
{
	ADDTOCALLSTACK("CChar::Fight_HitTry");
	// A timer has expired so try to take a hit
	// I'm ready to swing or already swinging (but I might not be close enough)
	ASSERT(Fight_IsActive());
	ASSERT(m_atFight.m_Swing_State == (WAR_SWING_READY|WAR_SWING_SWINGING));

	CChar *pCharTarg = m_Fight_Targ.CharFind();
	if ( !pCharTarg || !pCharTarg->Fight_IsAttackable() )
	{
		// Can't hit the current target
		if ( m_pNPC )
		{
			CChar *pNewTarg = NPC_FightFindBestTarget();
			if ( pNewTarg != pCharTarg )
			{
				// Switch to another target
				Fight_Attack(pNewTarg);
			}
			else
			{
				// There's no other target, so keep trying to attack the current target before give up
				if ( !Calc_GetRandVal(10) )
					Fight_Clear();
			}
		}
		else
		{
			Skill_Start(SKILL_NONE);
			m_Fight_Targ.InitUID();
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

bool CChar::Attacker_Add(CChar *pChar, INT64 iThreat)
{
	ADDTOCALLSTACK("CChar::Attacker_Add");
	if ( !pChar )
		return false;

	if ( Attacker_GetID(pChar) != -1 )		// char is already on attacker list
		return true;

	if ( IsTrigUsed(TRIGGER_COMBATSTART) )
	{
		if ( OnTrigger(CTRIG_CombatStart, pChar) == TRIGRET_RET_TRUE )
			return false;
	}

	CScriptTriggerArgs Args;
	Args.m_iN1 = iThreat;
	if ( IsTrigUsed(TRIGGER_COMBATADD) )
	{
		if ( OnTrigger(CTRIG_CombatAdd, pChar, &Args) == TRIGRET_RET_TRUE )
			return false;
		iThreat = Args.m_iN1;
	}

	LastAttackers refAttacker;
	refAttacker.charUID = pChar->GetUID();
	refAttacker.elapsed = 0;
	refAttacker.damage = 0;
	refAttacker.threat = m_pPlayer ? 0 : iThreat;
	m_lastAttackers.push_back(refAttacker);

	// Record the start of the fight
	Memory_Fight_Start(pChar);

	if ( (m_pNPC || pChar->m_pNPC) && IsSetCombatFlags(COMBAT_NPC_NOATTACKMSG) )
		return true;

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKO), GetName(), pChar->GetName());
	UpdateObjMessage(pszMsg, NULL, pChar->m_pClient, HUE_TEXT_DEF, TALKMODE_EMOTE);

	if ( pChar->m_pClient && pChar->CanSee(this) )
	{
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_COMBAT_ATTACKS), GetName());
		pChar->m_pClient->addBarkParse(pszMsg, this, HUE_TEXT_DEF, TALKMODE_EMOTE);
	}
	return true;
}

bool CChar::Attacker_Delete(int id, bool fForced, ATTACKER_CLEAR_TYPE type)
{
	ADDTOCALLSTACK("CChar::Attacker_Delete");
	if ( (id < 0) || (id >= static_cast<int>(m_lastAttackers.size())) )
		return false;

	LastAttackers &refAttacker = m_lastAttackers.at(id);
	CChar *pChar = refAttacker.charUID.CharFind();
	if ( !pChar )
		return false;

	if ( IsTrigUsed(TRIGGER_COMBATDELETE) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = fForced;
		Args.m_iN2 = static_cast<INT64>(type);
		if ( OnTrigger(CTRIG_CombatDelete, pChar, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	m_lastAttackers.erase(m_lastAttackers.begin() + id);

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

bool CChar::Attacker_Delete(CChar *pChar, bool fForced, ATTACKER_CLEAR_TYPE type)
{
	ADDTOCALLSTACK("CChar::Attacker_Delete(2)");
	return Attacker_Delete(Attacker_GetID(pChar), fForced, type);
}

int CChar::Attacker_GetID(CChar *pChar)
{
	ADDTOCALLSTACK("CChar::Attacker_GetID");
	if ( pChar )
	{
		for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
		{
			LastAttackers &refAttacker = m_lastAttackers.at(i);
			if ( refAttacker.charUID == pChar->GetUID() )
				return i;
		}
	}
	return -1;
}

CChar *CChar::Attacker_GetHighestDam()
{
	ADDTOCALLSTACK("CChar::Attacker_GetHighestDam");
	CChar *pChar = NULL;
	INT64 iHighestDam = -1;
	for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
	{
		LastAttackers &refAttacker = m_lastAttackers.at(i);
		if ( refAttacker.damage > iHighestDam )
		{
			pChar = refAttacker.charUID.CharFind();
			iHighestDam = refAttacker.damage;
		}
	}
	return pChar;
}

CChar *CChar::Attacker_GetLowestElapsed()
{
	ADDTOCALLSTACK("CChar::Attacker_GetLowestElapsed");
	CChar *pChar = NULL;
	INT64 iLowestElapsed = LLONG_MAX;
	for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
	{
		LastAttackers &refAttacker = m_lastAttackers.at(i);
		if ( refAttacker.elapsed <= iLowestElapsed )
		{
			pChar = refAttacker.charUID.CharFind();
			iLowestElapsed = refAttacker.elapsed;
		}
	}
	return pChar;
}

void CChar::Attacker_SetElapsed(int id, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetElapsed");
	if ( (id < 0) || (id >= static_cast<int>(m_lastAttackers.size())) )
		return;

	LastAttackers &refAttacker = m_lastAttackers.at(id);
	refAttacker.elapsed = value;
}

void CChar::Attacker_SetDamage(int id, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetDamage");
	if ( (id < 0) || (id >= static_cast<int>(m_lastAttackers.size())) )
		return;

	LastAttackers &refAttacker = m_lastAttackers.at(id);
	refAttacker.damage = value;
}

void CChar::Attacker_SetThreat(int id, INT64 value)
{
	ADDTOCALLSTACK("CChar::Attacker_SetThreat");
	if ( (id < 0) || (id >= static_cast<int>(m_lastAttackers.size())) )
		return;

	LastAttackers &refAttacker = m_lastAttackers.at(id);
	refAttacker.threat = value;
}

void CChar::Attacker_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::Attacker_CheckTimeout");
	for ( size_t i = 0; i < m_lastAttackers.size(); ++i )
	{
		LastAttackers &refAttacker = m_lastAttackers.at(i);
		if ( ++refAttacker.elapsed > g_Cfg.m_iAttackerTimeout )
		{
			CChar *pEnemy = refAttacker.charUID.CharFind();
			if ( pEnemy )
			{
				Attacker_Delete(static_cast<int>(i), true, ATTACKER_CLEAR_ELAPSED);
				if ( m_pPlayer && pEnemy->m_pPlayer )	// only show retreat msg on PvP
				{
					SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_1), pEnemy->GetName());
					pEnemy->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_2), GetName());
				}
				return;
			}
			m_lastAttackers.erase(m_lastAttackers.begin() + i);
			return;
		}
	}
}

int CChar::CalcFightRange(CItem *pWeapon)
{
	ADDTOCALLSTACK("CChar::CalcFightRange");
	// Distance from which I can hit

	BYTE bCharRange = RangeL();
	BYTE bWeaponRange = pWeapon ? pWeapon->RangeL() : 0;

	return maximum(bCharRange, bWeaponRange);
}

WAR_SWING_TYPE CChar::Fight_Hit(CChar *pCharTarg)
{
	ADDTOCALLSTACK("CChar::Fight_Hit");
	// Attempt to hit our target
	// ARGS:
	//  pCharTarg = the target
	// RETURN:
	//  WAR_SWING_INVALID	= target is invalid
	//  WAR_SWING_EQUIPPING	= recoiling weapon / swing made
	//  WAR_SWING_READY		= can't take my swing right now (but I'm ready to hit)
	//  WAR_SWING_SWINGING	= taking my swing now

	if ( !pCharTarg || (pCharTarg == this) )
		return WAR_SWING_INVALID;

	DAMAGE_TYPE iTyp = DAMAGE_HIT_BLUNT;

	if ( IsTrigUsed(TRIGGER_HITCHECK) )
	{
		CScriptTriggerArgs pArgs;
		pArgs.m_iN1 = m_atFight.m_Swing_State;
		pArgs.m_iN2 = iTyp;
		TRIGRET_TYPE iRet = OnTrigger(CTRIG_HitCheck, pCharTarg, &pArgs);
		if ( iRet == TRIGRET_RET_TRUE )
			return static_cast<WAR_SWING_TYPE>(pArgs.m_iN1);
		if ( iRet == -1 )
			return WAR_SWING_INVALID;

		m_atFight.m_Swing_State = static_cast<WAR_SWING_TYPE>(pArgs.m_iN1);
		iTyp = static_cast<DAMAGE_TYPE>(pArgs.m_iN2);

		if ( (m_atFight.m_Swing_State == WAR_SWING_SWINGING) && (iTyp & DAMAGE_FIXED) )
		{
			if ( iRet == TRIGRET_RET_DEFAULT )
				return WAR_SWING_EQUIPPING;

			CItem *pWeapon = m_uidWeapon.ItemFind();
			if ( iTyp == DAMAGE_HIT_BLUNT )		// if type did not change in the trigger, default iTyp is set
			{
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

			pCharTarg->OnTakeDamage(Fight_CalcDamage(pWeapon), this, iTyp, m_DamPhysical, m_DamFire, m_DamCold, m_DamPoison, m_DamEnergy);
			return WAR_SWING_EQUIPPING;
		}
	}

	// Very basic check on possibility to hit
	if ( IsStatFlag(STATF_DEAD|STATF_Freeze|STATF_Stone) || !pCharTarg->Fight_IsAttackable() )
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
		INT64 iDiff = GetKeyNum("LastHit") - g_World.GetCurrentTime().GetTimeRaw();
		if ( iDiff > 0 )
		{
			SetTimeout(minimum(iDiff, 50));
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
	bool fSkillRanged = g_Cfg.IsSkillFlag(skill, SKF_RANGED);

	if ( !(m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_GUARD) && g_Cfg.m_fGuardsInstantKill) )	// guards should ignore distance checks when instant kill is enabled
	{
		if ( fSkillRanged )
		{
			if ( IsStatFlag(STATF_HasShield) )		// this should never happen
			{
				SysMessageDefault(DEFMSG_ITEMUSE_BOW_SHIELD);
				return WAR_SWING_INVALID;
			}
			else if ( !IsSetCombatFlags(COMBAT_ARCHERYCANMOVE) && !IsStatFlag(STATF_ArcherCanMove) )
			{
				// Only start next swing after the char stop moving for some time
				if ( m_pClient && (-g_World.GetTimeDiff(m_pClient->m_timeLastEventWalk) / TICK_PER_SEC < g_Cfg.m_iCombatArcheryMovementDelay) )
					return WAR_SWING_EQUIPPING;
			}

			int iMinDist = pWeapon ? pWeapon->RangeH() : g_Cfg.m_iArcheryMinDist;
			int iMaxDist = pWeapon ? pWeapon->RangeL() : g_Cfg.m_iArcheryMaxDist;
			if ( !iMaxDist || ((iMinDist == 0) && (iMaxDist == 1)) )
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
			int iMinDist = pWeapon ? pWeapon->RangeH() : 0;
			int iMaxDist = CalcFightRange(pWeapon);
			if ( (iDist < iMinDist) || (iDist > iMaxDist) )
				return IsSetCombatFlags(COMBAT_STAYINRANGE) ? WAR_SWING_EQUIPPING : WAR_SWING_READY;
		}
	}

	// Start the swing
	if ( m_atFight.m_Swing_State == WAR_SWING_READY )
	{
		if ( !CanSeeLOS(pCharTarg) )
			return WAR_SWING_EQUIPPING;

		ANIM_TYPE anim = GenerateAnimate(ANIM_ATTACK_WEAPON);
		int animDelay = 7;		// attack speed is always 7ms and then the char keep waiting the remaining time
		int iSwingDelay = g_Cfg.Calc_CombatAttackSpeed(this, pWeapon);

		if ( IsTrigUsed(TRIGGER_HITTRY) )
		{
			CScriptTriggerArgs Args(iSwingDelay, 0, pWeapon);
			Args.m_VarsLocal.SetNum("Anim", static_cast<int>(anim));
			Args.m_VarsLocal.SetNum("AnimDelay", animDelay);
			if ( OnTrigger(CTRIG_HitTry, pCharTarg, &Args) == TRIGRET_RET_TRUE )
				return WAR_SWING_READY;

			iSwingDelay = static_cast<int>(maximum(0, Args.m_iN1));
			anim = static_cast<ANIM_TYPE>(Args.m_VarsLocal.GetKeyNum("Anim"));
			animDelay = static_cast<int>(maximum(0, Args.m_VarsLocal.GetKeyNum("AnimDelay")));
		}

		m_atFight.m_Swing_State = WAR_SWING_SWINGING;
		m_atFight.m_Swing_Delay = static_cast<BYTE>(maximum(0, iSwingDelay - animDelay));

		if ( IsSetCombatFlags(COMBAT_PREHIT) )
		{
			SetKeyNum("LastHit", g_World.GetCurrentTime().GetTimeRaw() + iSwingDelay);
			SetTimeout(0);
		}
		else
			SetTimeout(animDelay);

		if ( !IsSetCombatFlags(COMBAT_NODIRCHANGE) )
			UpdateDir(pCharTarg);

		Reveal();
		UpdateAnimate(anim, false, false, static_cast<BYTE>(animDelay / TICK_PER_SEC));
		return WAR_SWING_SWINGING;
	}

	// We made our swing, so we must recoil
	m_atFight.m_Swing_State = WAR_SWING_EQUIPPING;
	m_atFight.m_Swing_NextAction = CServTime::GetCurrentTime() + m_atFight.m_Swing_Delay;

	if ( fSkillRanged && pWeapon )
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

	if ( m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_GUARD) && g_Cfg.m_fGuardsInstantKill )
	{
		iTyp |= DAMAGE_GOD;
		m_Act_Difficulty = 100;		// never miss
		pCharTarg->Stat_SetVal(STAT_STR, 1);
		pCharTarg->Effect(EFFECT_LIGHTNING, ITEMID_NOTHING, this);
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

			if ( Args.m_VarsLocal.GetKeyNum("ArrowHandled") != 0 )		// if arrow is handled by script, do nothing with it further
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

		SOUND_TYPE iSound = pWeapon ? pWeapon->Weapon_GetSoundMiss() : SOUND_NONE;
		if ( iSound == SOUND_NONE )
		{
			static const SOUND_TYPE sm_SoundMiss_Wrestling[] = { 0x238, 0x239, 0x23A };
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
		int iParryChance = 0;
		if ( pCharTarg->IsStatFlag(STATF_HasShield) )	// parry using shield
		{
			pItemHit = pCharTarg->LayerFind(LAYER_HAND2);
			iParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 40;
		}
		else if ( pCharTarg->m_uidWeapon.IsItem() )		// parry using weapon
		{
			pItemHit = pCharTarg->m_uidWeapon.ItemFind();
			iParryChance = pCharTarg->Skill_GetBase(SKILL_PARRYING) / 80;
		}

		if ( pCharTarg->Skill_GetBase(SKILL_PARRYING) >= 1000 )
			iParryChance += 5;

		if ( pCharTarg->m_pPlayer )
		{
			int iDex = pCharTarg->Stat_GetAdjusted(STAT_DEX);
			if ( iDex < 80 )
				iParryChance = iParryChance * (20 + iDex) / 100;
		}

		if ( pItemHit && (pItemHit->m_LastParryChance != iParryChance) )
		{
			pItemHit->m_LastParryChance = iParryChance;
			pItemHit->UpdatePropertyFlag();
		}

		if ( pCharTarg->Skill_UseQuick(SKILL_PARRYING, iParryChance, true, false) )
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
	int iDmg = Fight_CalcDamage(pWeapon);

	CScriptTriggerArgs Args(iDmg, iTyp, pWeapon);
	Args.m_VarsLocal.SetNum("ItemDamageChance", 25);
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
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && pWeapon->m_itWeapon.m_poison_skill && (pWeapon->m_itWeapon.m_poison_skill > Calc_GetRandVal(100)) )
		{
			BYTE bPoisonLevel = static_cast<BYTE>(Calc_GetRandVal(pWeapon->m_itWeapon.m_poison_skill));
			pCharTarg->SetPoison(10 * bPoisonLevel, bPoisonLevel / 5, this);

			pWeapon->m_itWeapon.m_poison_skill -= bPoisonLevel / 2;		// reduce weapon poison charges
			pWeapon->UpdatePropertyFlag();
		}

		// Check if the weapon will be damaged
		if ( Args.m_VarsLocal.GetKeyNum("ItemDamageChance") > Calc_GetRandVal(100) )
			pWeapon->OnTakeDamage(iDmg, pCharTarg);
	}
	else if ( NPC_IsMonster() )
	{
		// Base poisoning for NPCs
		if ( !IsSetCombatFlags(COMBAT_NOPOISONHIT) && (50 > Calc_GetRandVal(100)) )
		{
			int iPoisoningSkill = Skill_GetBase(SKILL_POISONING);
			if ( iPoisoningSkill )
				pCharTarg->SetPoison(Calc_GetRandVal(iPoisoningSkill), Calc_GetRandVal(iPoisoningSkill / 50), this);
		}
	}

	// Took my swing. Do Damage!
	iDmg = pCharTarg->OnTakeDamage(iDmg, this, iTyp, m_DamPhysical, m_DamFire, m_DamCold, m_DamPoison, m_DamEnergy);

	// Post-damage behavior
	if ( iDmg > 0 )
	{
		bool fLeechSound = false;
		int iHitLifeLeech = m_HitLifeLeech;
		if ( pWeapon )
		{
			const CItem *pCurseWeapon = LayerFind(LAYER_SPELL_Curse_Weapon);
			if ( pCurseWeapon )
				iHitLifeLeech += pCurseWeapon->m_itSpell.m_spelllevel;
		}
		if ( iHitLifeLeech > 0 )
		{
			UpdateStatVal(STAT_STR, Calc_GetRandVal((iDmg * iHitLifeLeech * 30) / 10000), Stat_GetMax(STAT_STR));	// leech 0% ~ 30% of damage value
			fLeechSound = true;
		}
		if ( m_HitManaLeech > 0 )
		{
			UpdateStatVal(STAT_INT, Calc_GetRandVal((iDmg * m_HitManaLeech * 40) / 10000), Stat_GetMax(STAT_INT));	// leech 0% ~ 40% of damage value
			fLeechSound = true;
		}
		if ( m_HitStaminaLeech > Calc_GetRandVal(100) )
		{
			UpdateStatVal(STAT_DEX, iDmg, Stat_GetMax(STAT_DEX));	// leech 100% of damage value
			fLeechSound = true;
		}
		if ( fLeechSound )
			Sound(0x44D);

		int iManaDrain = 0;
		if ( m_HitManaDrain > Calc_GetRandVal(100) )
			iManaDrain += iDmg * 20 / 100;	// leech 20% of damage value
		if ( g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B )
		{
			const CItem *pPoly = LayerFind(LAYER_SPELL_Polymorph);
			if ( pPoly && (pPoly->m_itSpell.m_spell == SPELL_Wraith_Form) )
				iManaDrain += 5 + (15 * Skill_GetBase(SKILL_SPIRITSPEAK) / 1000);
		}
		if ( iManaDrain > 0 )
		{
			int iTargMana = pCharTarg->Stat_GetVal(STAT_INT);
			if ( iManaDrain > iTargMana )
				iManaDrain = iTargMana;

			UpdateStatVal(STAT_INT, iManaDrain, Stat_GetMax(STAT_INT));
			pCharTarg->UpdateStatVal(STAT_INT, -iManaDrain);
			pCharTarg->Effect(EFFECT_OBJ, ITEMID_FX_VORTEX, this, 10, 25);
			pCharTarg->Sound(0x1F8);
		}

		if ( pCharTarg->m_ReflectPhysicalDamage )
			OnTakeDamage(iDmg * pCharTarg->m_ReflectPhysicalDamage / 100, this, iTyp, m_DamPhysical, m_DamFire, m_DamCold, m_DamPoison, m_DamEnergy);

		if ( pWeapon )
		{
			if ( pWeapon->m_HitPhysicalArea > Calc_GetRandVal(100) )
				pCharTarg->OnTakeDamageArea(iDmg / 2, this, DAMAGE_GENERAL, 100, 0, 0, 0, 0, static_cast<HUE_TYPE>(0x32), static_cast<SOUND_TYPE>(0x10E));

			if ( pWeapon->m_HitFireArea > Calc_GetRandVal(100) )
				pCharTarg->OnTakeDamageArea(iDmg / 2, this, DAMAGE_GENERAL, 0, 100, 0, 0, 0, static_cast<HUE_TYPE>(0x488), static_cast<SOUND_TYPE>(0x11D));

			if ( pWeapon->m_HitColdArea > Calc_GetRandVal(100) )
				pCharTarg->OnTakeDamageArea(iDmg / 2, this, DAMAGE_GENERAL, 0, 0, 100, 0, 0, static_cast<HUE_TYPE>(0x834), static_cast<SOUND_TYPE>(0xFC));

			if ( pWeapon->m_HitPoisonArea > Calc_GetRandVal(100) )
				pCharTarg->OnTakeDamageArea(iDmg / 2, this, DAMAGE_GENERAL, 0, 0, 0, 100, 0, static_cast<HUE_TYPE>(0x48E), static_cast<SOUND_TYPE>(0x205));

			if ( pWeapon->m_HitEnergyArea > Calc_GetRandVal(100) )
				pCharTarg->OnTakeDamageArea(iDmg / 2, this, DAMAGE_GENERAL, 0, 0, 0, 0, 100, static_cast<HUE_TYPE>(0x78), static_cast<SOUND_TYPE>(0x1F1));

			if ( pWeapon->m_HitDispel > Calc_GetRandVal(100) )
				pCharTarg->OnSpellEffect(SPELL_Dispel, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);

			if ( pWeapon->m_HitFireball > Calc_GetRandVal(100) )
				pCharTarg->OnSpellEffect(SPELL_Fireball, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);

			if ( pWeapon->m_HitHarm > Calc_GetRandVal(100) )
				pCharTarg->OnSpellEffect(SPELL_Harm, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);

			if ( pWeapon->m_HitLightning > Calc_GetRandVal(100) )
				pCharTarg->OnSpellEffect(SPELL_Lightning, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);

			if ( pWeapon->m_HitMagicArrow > Calc_GetRandVal(100) )
				pCharTarg->OnSpellEffect(SPELL_Magic_Arrow, this, Skill_GetAdjusted(SKILL_MAGERY), pWeapon);
		}

		// Make blood effects
		if ( pCharTarg->m_wBloodHue != static_cast<HUE_TYPE>(-1) )
		{
			static const ITEMID_TYPE sm_Blood[] = { ITEMID_BLOOD1, ITEMID_BLOOD2, ITEMID_BLOOD3, ITEMID_BLOOD4, ITEMID_BLOOD5, ITEMID_BLOOD6, ITEMID_BLOOD_SPLAT };
			int iBloodQty = (g_Cfg.m_iFeatureSE & FEATURE_SE_UPDATE) ? Calc_GetRandVal(4, 5) : Calc_GetRandVal(1, 2);

			for ( int i = 0; i < iBloodQty; ++i )
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
