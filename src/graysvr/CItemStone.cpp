#include "graysvr.h"	// predef header

///////////////////////////////////////////////////////////
// CStoneMember

CStoneMember::CStoneMember(CItemStone *pStone, CGrayUID uidLinkTo, STONEPRIV_TYPE priv, LPCTSTR pszTitle, CGrayUID uidLoyalTo, bool fVal1, bool fVal2, int iAccountGold)
{
	m_priv = priv;
	m_uidLinkTo = uidLinkTo;
	m_uidLoyalTo = uidLoyalTo;
	m_sTitle = pszTitle;

	if ( priv == STONEPRIV_ENEMY )
	{
		m_Enemy.m_fTheyDeclared = fVal1;
		m_Enemy.m_fWeDeclared = fVal2;
	}
	else if ( priv == STONEPRIV_ALLY )
	{
		m_Ally.m_fTheyDeclared = fVal1;
		m_Ally.m_fWeDeclared = fVal2;
	}
	else
	{
		m_Member.m_fAbbrev = fVal1;
		m_Member.m_iVoteTally = fVal2;
	}

	m_Member.m_iAccountGold = iAccountGold;

	if ( !g_Serv.IsLoading() && pStone->GetMemoryType() )
	{
		CChar *pChar = uidLinkTo.CharFind();
		if ( pChar )
		{
			pChar->Memory_AddObjTypes(pStone, static_cast<WORD>(pStone->GetMemoryType()));
			if ( pStone->IsTopLevel() )
				pChar->m_ptHome = pStone->GetTopPoint();
		}
	}

	pStone->InsertTail(this);
}

CStoneMember::~CStoneMember()
{
	CItemStone *pStone = GetParentStone();
	if ( !pStone )
		return;

	RemoveSelf();

	if ( m_priv == STONEPRIV_ENEMY )
	{
		CItemStone *pStoneEnemy = static_cast<CItemStone *>(m_uidLinkTo.ItemFind());
		if ( pStoneEnemy )
			pStoneEnemy->TheyDeclarePeace(pStone, true);
	}
	else if ( pStone->GetMemoryType() )
	{
		pStone->ElectMaster();

		CChar *pChar = m_uidLinkTo.CharFind();
		if ( pChar )
			pChar->Memory_ClearTypes(static_cast<WORD>(pStone->GetMemoryType()));
	}
}

LPCTSTR CStoneMember::GetPrivName() const
{
	ADDTOCALLSTACK("CStoneMember::GetPrivName");
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "STONECONFIG_PRIVNAME_PRIVID-%d", m_priv);

	CVarDefCont *pVar = g_Exp.m_VarDefs.GetKey(pszTemp);
	if ( pVar )
		return pVar->GetValStr();

	pVar = g_Exp.m_VarDefs.GetKey("STONECONFIG_PRIVNAME_PRIVUNK");
	if ( pVar )
		return pVar->GetValStr();

	return "";
}

bool CStoneMember::SetLoyalTo(const CChar *pChar)
{
	ADDTOCALLSTACK("CStoneMember::SetLoyalTo");
	CChar *pCharSrc = m_uidLinkTo.CharFind();
	if ( !pCharSrc )	// on shutdown
		return false;

	m_uidLoyalTo = m_uidLinkTo;		// set to self by default
	if ( !pChar )
		return true;

	if ( !IsPrivMember() )
	{
		pCharSrc->SysMessage("Candidates aren't elligible to vote.");
		return false;
	}

	CItemStone *pStone = GetParentStone();
	if ( !pStone )
		return false;

	CStoneMember *pMember = pStone->GetMember(pChar);
	if ( !pMember || !pMember->IsPrivMember() )
	{
		pCharSrc->SysMessage("Can only vote for full members.");
		return false;
	}

	m_uidLoyalTo = pChar->GetUID();
	pStone->ElectMaster();
	return true;
}

CItemStone *CStoneMember::GetParentStone() const
{
	ADDTOCALLSTACK("CStoneMember::GetParentStone");
	return dynamic_cast<CItemStone *>(GetParent());
}

bool CStoneMember::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

enum STMMV_TYPE
{
	#define ADD(a,b) STMMV_##a,
	#include "../tables/CStoneMember_functions.tbl"
	#undef ADD
	STMMV_QTY
};

LPCTSTR const CStoneMember::sm_szVerbKeys[STMMV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CStoneMember_functions.tbl"
	#undef ADD
	NULL
};

bool CStoneMember::r_Verb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CStoneMember::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
	{
		if ( r_LoadVal(s) )
			return true;
	}

	if ( m_uidLinkTo.IsChar() )
	{
		CScriptObj *pRef = m_uidLinkTo.CharFind();
		return pRef->r_Verb(s, pSrc);
	}
	else if ( m_uidLinkTo.IsItem() )
	{
		CScriptObj *pRef = m_uidLinkTo.ItemFind();
		return pRef->r_Verb(s, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

enum STMM_TYPE
{
	#define ADD(a,b) STMM_##a,
	#include "../tables/CStoneMember_props.tbl"
	#undef ADD
	STMM_QTY
};

LPCTSTR const CStoneMember::sm_szLoadKeys[STMM_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CStoneMember_props.tbl"
	#undef ADD
	NULL
};

bool CStoneMember::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CStoneMember::r_LoadVal");
	EXC_TRY("LoadVal");

	int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( m_uidLinkTo.IsChar() )
	{
		switch ( index )
		{
			case STMM_ACCOUNTGOLD:
				m_Member.m_iAccountGold = s.GetArgVal();
				break;
			case STMM_LOYALTO:
				SetLoyalTo(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
				break;
			case STMM_PRIV:
				m_priv = static_cast<STONEPRIV_TYPE>(s.GetArgVal());
				break;
			case STMM_SHOWABBREV:
				m_Member.m_fAbbrev = s.GetArgVal() ? true : false;
				break;
			case STMM_TITLE:
				m_sTitle = s.GetArgStr();
				break;
			default:
				return false;
		}
	}
	else if ( m_uidLinkTo.IsItem() )
	{
		switch ( index )
		{
			case STMM_GUILD_THEYALLIANCE:
				m_Ally.m_fTheyDeclared = s.GetArgVal() ? true : false;
				break;
			case STMM_GUILD_THEYWAR:
				m_Enemy.m_fTheyDeclared = s.GetArgVal() ? true : false;
				break;
			case STMM_GUILD_WEALLIANCE:
				m_Ally.m_fWeDeclared = s.GetArgVal() ? true : false;
				break;
			case STMM_GUILD_WEWAR:
				m_Enemy.m_fWeDeclared = s.GetArgVal() ? true : false;
				break;
			default:
				return false;
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CStoneMember::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CStoneMember::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( m_uidLinkTo.IsChar() )
	{
		switch ( index )
		{
			case STMM_ACCOUNTGOLD:
				sVal.FormatVal(m_Member.m_iAccountGold);
				break;
			case STMM_ISCANDIDATE:
				sVal.FormatVal((m_priv == STONEPRIV_CANDIDATE));
				break;
			case STMM_ISMASTER:
				sVal.FormatVal((m_priv == STONEPRIV_MASTER));
				break;
			case STMM_ISMEMBER:
				sVal.FormatVal(IsPrivMember());
				break;
			case STMM_LOYALTO:
				sVal.FormatHex(m_uidLoyalTo);
				break;
			case STMM_PRIV:
				sVal.FormatVal(m_priv);
				break;
			case STMM_PRIVNAME:
				sVal = GetPrivName();
				break;
			case STMM_SHOWABBREV:
				sVal.FormatVal(IsAbbrevOn());
				break;
			case STMM_TITLE:
				sVal = GetTitle();
				break;
			default:
			{
				CScriptObj *pRef = m_uidLinkTo.CharFind();
				return pRef->r_WriteVal(pszKey, sVal, pSrc);
			}
		}
	}
	else if ( m_uidLinkTo.IsItem() )
	{
		switch ( index )
		{
			case STMM_GUILD_ISALLY:
				sVal.FormatVal(m_Ally.m_fWeDeclared && m_Ally.m_fTheyDeclared);
				break;
			case STMM_GUILD_ISENEMY:
				sVal.FormatVal(m_Enemy.m_fWeDeclared && m_Enemy.m_fTheyDeclared);
				break;
			case STMM_GUILD_THEYALLIANCE:
				sVal.FormatVal(m_Ally.m_fTheyDeclared);
				break;
			case STMM_GUILD_THEYWAR:
				sVal.FormatVal(m_Enemy.m_fTheyDeclared);
				break;
			case STMM_GUILD_WEALLIANCE:
				sVal.FormatVal(m_Ally.m_fWeDeclared);
				break;
			case STMM_GUILD_WEWAR:
				sVal.FormatVal(m_Enemy.m_fWeDeclared);
				break;
			default:
			{
				CScriptObj *pRef = m_uidLinkTo.ItemFind();
				return pRef->r_WriteVal(pszKey, sVal, pSrc);
			}
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////
// CItemStone

CItemStone::CItemStone(ITEMID_TYPE id, CItemBase *pItemDef) : CItem(id, pItemDef)
{
	m_itStone.m_align = STONEALIGN_STANDARD;
	g_World.m_Stones.Add(this);
}

CItemStone::~CItemStone()
{
	SetAmount(0);		// the stone will be removed anyway, so set amount=0 to skip redundant checks
	DeletePrepare();	// must remove early because virtuals will fail in child destructor
	g_World.m_Stones.RemovePtr(this);
	Empty();			// do this manually to preserve parents type cast
}

MEMORY_TYPE CItemStone::GetMemoryType() const
{
	ADDTOCALLSTACK("CItemStone::GetMemoryType");
	switch ( GetType() )
	{
		case IT_STONE_TOWN:
			return MEMORY_TOWN;
		case IT_STONE_GUILD:
			return MEMORY_GUILD;
		default:
			return MEMORY_NONE;
	}
}

LPCTSTR CItemStone::GetTypeName() const
{
	ADDTOCALLSTACK("CItemStone::GetTypeName");
	TCHAR *pszTemp = Str_GetTemp();
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			pszTemp = "STONECONFIG_TYPENAME_GUILD";
			break;
		case IT_STONE_TOWN:
			pszTemp = "STONECONFIG_TYPENAME_TOWN";
			break;
		default:
			pszTemp = "STONECONFIG_TYPENAME_UNK";
			break;
	}

	CVarDefCont *pVar = g_Exp.m_VarDefs.GetKey(pszTemp);
	return pVar ? pVar->GetValStr() : "";
}

LPCTSTR CItemStone::GetAlignName() const
{
	ADDTOCALLSTACK("CItemStone::GetAlignName");
	TCHAR *pszTemp = Str_GetTemp();
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			sprintf(pszTemp, "GUILDCONFIG_ALIGN_%d", m_itStone.m_align);
			break;
		case IT_STONE_TOWN:
			sprintf(pszTemp, "TOWNSCONFIG_ALIGN_%d", m_itStone.m_align);
			break;
	}

	CVarDefCont *pVar = g_Exp.m_VarDefs.GetKey(pszTemp);
	return pVar ? pVar->GetValStr() : "";
}

CStoneMember *CItemStone::GetMasterMember() const
{
	ADDTOCALLSTACK("CItemStone::GetMasterMember");
	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
	{
		if ( pMember->m_priv == STONEPRIV_MASTER )
			return pMember;
	}
	return NULL;
}

CChar *CItemStone::GetMaster() const
{
	ADDTOCALLSTACK("CItemStone::GetMaster");
	CStoneMember *pMember = GetMasterMember();
	return pMember ? pMember->m_uidLinkTo.CharFind() : NULL;
}

CStoneMember *CItemStone::GetMember(const CObjBase *pObj) const
{
	ADDTOCALLSTACK("CItemStone::GetMember");
	if ( pObj )
	{
		CGrayUID uid = pObj->GetUID();
		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( pMember->m_uidLinkTo == uid )
				return pMember;
		}
	}
	return NULL;
}

CStoneMember *CItemStone::AddRecruit(const CChar *pChar, STONEPRIV_TYPE priv, bool fFull)
{
	ADDTOCALLSTACK("CItemStone::AddRecruit");
	// CLIMODE_TARG_STONE_RECRUIT

	if ( !pChar || !pChar->m_pPlayer )
	{
		Speak("Only players can be members!");
		return NULL;
	}

	TCHAR *pszTemp = Str_GetTemp();
	const CItemStone *pStone = pChar->Guild_Find(GetMemoryType());
	if ( pStone && (pStone != this) )
	{
		sprintf(pszTemp, "%s appears to belong to %s. Must resign previous %s", pChar->GetName(), pStone->GetName(), GetTypeName());
		Speak(pszTemp);
		return NULL;
	}

	if ( IsType(IT_STONE_TOWN) && (priv == STONEPRIV_CANDIDATE) && IsAttr(ATTR_OWNED) )
		priv = STONEPRIV_MEMBER;

	CStoneMember *pMember = GetMember(pChar);
	if ( pMember )
	{
		if ( (priv == pMember->m_priv) || (priv == STONEPRIV_CANDIDATE) )
		{
			sprintf(pszTemp, "%s is already %s %s.", pChar->GetName(), pMember->GetPrivName(), GetName());
			Speak(pszTemp);
			return NULL;
		}
		pMember->m_priv = priv;
	}
	else
	{
		pMember = new CStoneMember(this, pChar->GetUID(), priv);
		if ( fFull )
		{
			pMember->m_priv = STONEPRIV_MEMBER;
			pMember->m_Member.m_fAbbrev = true;
		}
	}

	if ( pMember->IsPrivMember() )
	{
		pMember->SetLoyalTo(pChar);
		ElectMaster();
	}

	sprintf(pszTemp, "%s is now %s %s", pChar->GetName(), pMember->GetPrivName(), GetName());
	Speak(pszTemp);
	return pMember;
}

void CItemStone::ElectMaster()
{
	ADDTOCALLSTACK("CItemStone::ElectMaster");
	if ( GetAmount() == 0 )		// no reason to proceed if the stone is going to be removed
		return;

	FixWeirdness();		// try to eliminate bad members
	/*int iResultCode = FixWeirdness();
	if ( iResultCode )
	{
		// The stone is bad ?
	}*/

	CStoneMember *pMaster = NULL;
	int iCountMembers = 0;

	// Reset vote count
	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
	{
		if ( pMember->m_priv == STONEPRIV_MASTER )
			pMaster = pMember;
		else if ( pMember->m_priv != STONEPRIV_MEMBER )
			continue;

		pMember->m_Member.m_iVoteTally = 0;
		++iCountMembers;
	}

	// Count vote of all members
	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
	{
		if ( !pMember->IsPrivMember() )
			continue;

		CStoneMember *pMemberVote = GetMember(pMember->m_uidLoyalTo.CharFind());
		if ( pMemberVote )
		{
			++pMemberVote->m_Member.m_iVoteTally;
			continue;
		}

		pMember->SetLoyalTo(NULL);
		++pMember->m_Member.m_iVoteTally;
	}

	// Check who have most votes
	bool fTie = false;
	CStoneMember *pMemberHighest = NULL;
	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
	{
		if ( !pMember->IsPrivMember() )
			continue;

		if ( !pMemberHighest )
		{
			pMemberHighest = pMember;
			continue;
		}

		if ( pMember->m_Member.m_iVoteTally == pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = true;
		}
		else if ( pMember->m_Member.m_iVoteTally > pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = false;
			pMemberHighest = pMember;
		}
	}

	// In the event of a tie, leave the current master as is
	if ( !fTie && pMemberHighest )
	{
		if ( pMaster )
			pMaster->m_priv = STONEPRIV_MEMBER;
		pMemberHighest->m_priv = STONEPRIV_MASTER;
	}

	// No more members, declare peace (by force)
	if ( !iCountMembers )
	{
		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			WeDeclarePeace(static_cast<CItemStone *>(pMember->m_uidLinkTo.ItemFind()), true);
		}
	}
}

bool CItemStone::CheckValidMember(CStoneMember *pMember)
{
	ADDTOCALLSTACK("CItemStone::CheckValidMember");
	if ( GetAmount() == 0 )		// no reason to proceed if the stone is going to be removed
		return true;

	switch ( pMember->m_priv )
	{
		case STONEPRIV_MASTER:
		case STONEPRIV_MEMBER:
		case STONEPRIV_CANDIDATE:
		case STONEPRIV_ACCEPTED:
		{
			if ( GetMemoryType() )
			{
				// Make sure the member has a memory that links them back here
				CChar *pChar = pMember->m_uidLinkTo.CharFind();
				if ( !pChar || (pChar->Guild_Find(GetMemoryType()) != this) )
					break;
			}
			return true;
		}
		case STONEPRIV_ENEMY:
		{
			CItemStone *pStone = static_cast<CItemStone *>(pMember->m_uidLinkTo.ItemFind());
			if ( !pStone )
				break;

			CStoneMember *pMember = pStone->GetMember(this);
			if ( !pMember )
				break;
			if ( pMember->m_Enemy.m_fWeDeclared && !pMember->m_Enemy.m_fTheyDeclared )
				break;
			if ( pMember->m_Enemy.m_fTheyDeclared && !pMember->m_Enemy.m_fWeDeclared )
				break;
			return true;
		}
		case STONEPRIV_ALLY:
		{
			CItemStone *pStone = static_cast<CItemStone *>(pMember->m_uidLinkTo.ItemFind());
			if ( !pStone )
				break;

			CStoneMember *pMember = pStone->GetMember(this);
			if ( !pMember )
				break;
			if ( pMember->m_Ally.m_fWeDeclared && !pMember->m_Ally.m_fTheyDeclared )
				break;
			if ( pMember->m_Ally.m_fTheyDeclared && !pMember->m_Ally.m_fWeDeclared )
				break;
			return true;
		}
	}

	DEBUG_ERR(("Stone UID=0%lx has mislinked member UID=0%lx\n", static_cast<DWORD>(GetUID()), static_cast<DWORD>(pMember->m_uidLinkTo)));
	return false;
}

int CItemStone::FixWeirdness()
{
	ADDTOCALLSTACK("CItemStone::FixWeirdness");
	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
		return iResultCode;

	bool fChanged = false;
	CStoneMember *pMemberNext = NULL;
	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMemberNext )
	{
		pMemberNext = pMember->GetNext();
		if ( !CheckValidMember(pMember) )
		{
			IT_TYPE oldType = GetType();
			SetAmount(0);	// turn off validation for now
			delete pMember;
			SetAmount(1);
			SetType(oldType);
			fChanged = true;
		}
	}

	if ( fChanged )
		ElectMaster();

	return 0;
}

bool CItemStone::IsAlliedWith(const CItemStone *pStone) const
{
	ADDTOCALLSTACK("CItemStone::IsAlliedWith");
	if ( !pStone )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pStone);
	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isalliedwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
			return false;
		else if ( tr == TRIGRET_RET_TRUE )
			return true;
	}

	// Check if both stones declared alliance
	CStoneMember *pMember = GetMember(pStone);
	return (pMember && pMember->m_Ally.m_fTheyDeclared && pMember->m_Ally.m_fWeDeclared);
}

bool CItemStone::IsAtWarWith(const CItemStone *pStone) const
{
	ADDTOCALLSTACK("CItemStone::IsAtWarWith");
	if ( !pStone )
		return false;

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pStone);
	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isatwarwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
			return false;
		else if ( tr == TRIGRET_RET_TRUE )
			return true;
	}

	// Check if stone is of opposite align
	if ( IsType(IT_STONE_GUILD) && (m_itStone.m_align != STONEALIGN_STANDARD) && (pStone->m_itStone.m_align != STONEALIGN_STANDARD) )
		return (m_itStone.m_align != pStone->m_itStone.m_align);

	// Check if both stones declared war
	CStoneMember *pMember = GetMember(pStone);
	return (pMember && pMember->m_Enemy.m_fTheyDeclared && pMember->m_Enemy.m_fWeDeclared);
}

bool CItemStone::WeDeclareWar(CItemStone *pStone)
{
	ADDTOCALLSTACK("CItemStone::WeDeclareWar");
	if ( !pStone )
		return false;

	// Declare war to the other stone
	CStoneMember *pMember = GetMember(pStone);
	if ( !pMember )
		pMember = new CStoneMember(this, pStone->GetUID(), STONEPRIV_ENEMY);

	if ( pMember->m_Enemy.m_fWeDeclared )
		return true;
	pMember->m_Enemy.m_fWeDeclared = true;

	// Inform the other stone that we had declared war with it
	CStoneMember *pEnemyMember = pStone->GetMember(this);
	if ( !pEnemyMember ) // Not yet it seems
		pEnemyMember = new CStoneMember(pStone, GetUID(), STONEPRIV_ENEMY);

	pEnemyMember->m_Enemy.m_fTheyDeclared = true;
	return true;
}

void CItemStone::TheyDeclarePeace(CItemStone *pStone, bool fForcePeace)
{
	ADDTOCALLSTACK("CItemStone::TheyDeclarePeace");
	CStoneMember *pMember = GetMember(pStone);
	if ( !pMember )
		return;

	if ( !pMember->m_Enemy.m_fWeDeclared || fForcePeace )
		delete pMember;
	else
		pMember->m_Enemy.m_fTheyDeclared = false;
}

void CItemStone::WeDeclarePeace(CItemStone *pStone, bool fForcePeace)
{
	ADDTOCALLSTACK("CItemStone::WeDeclarePeace");
	CStoneMember *pMember = GetMember(pStone);
	if ( !pMember )
		return;

	if ( !pMember->m_Enemy.m_fTheyDeclared || fForcePeace )
		delete pMember;
	else
		pMember->m_Enemy.m_fWeDeclared = false;

	pStone->TheyDeclarePeace(this, fForcePeace);
}

void CItemStone::SetTownName()
{
	ADDTOCALLSTACK("CItemStone::SetTownName");
	if ( !IsTopLevel() )
		return;

	CRegionBase *pArea = GetTopPoint().GetRegion((IsType(IT_STONE_TOWN)) ? REGION_TYPE_AREA : REGION_TYPE_ROOM);
	if ( pArea )
		pArea->SetName(GetIndividualName());
}

bool CItemStone::MoveTo(CPointMap pt, bool fForceFix)
{
	ADDTOCALLSTACK("CItemStone::MoveTo");
	if ( IsType(IT_STONE_TOWN) )
		SetTownName();

	return CItem::MoveTo(pt, fForceFix);
}

bool CItemStone::SetName(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CItemStone::SetName");
	if ( !CItem::SetName(pszName) )
		return false;

	if ( IsTopLevel() && IsType(IT_STONE_TOWN) )
		SetTownName();
	return true;
}

bool CItemStone::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CItemStone::r_GetRef");
	if ( !strnicmp("MEMBER.", pszKey, 7) )
	{
		pszKey += 7;
		if ( !*pszKey )
			return false;

		int i = 0;
		int iNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( !pMember->m_uidLinkTo.IsChar() )
				continue;

			if ( iNumber == i )
			{
				pRef = pMember;
				return true;
			}
			++i;
		}
	}
	else if ( !strnicmp("MEMBERFROMUID.", pszKey, 14) )
	{
		pszKey += 14;
		if ( !*pszKey )
			return false;

		CStoneMember *pMember = GetMember(static_cast<CGrayUID>(Exp_GetVal(pszKey)).CharFind());
		SKIP_SEPARATORS(pszKey);
		if ( pMember )
		{
			pRef = pMember;
			return true;
		}
	}
	else if ( !strnicmp("GUILD.", pszKey, 6) )
	{
		pszKey += 6;
		if ( !*pszKey )
			return false;

		int i = 0;
		int iNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( pMember->m_uidLinkTo.IsChar() )
				continue;

			if ( iNumber == i )
			{
				pRef = pMember;
				return true;
			}
			++i;
		}
	}
	else if ( !strnicmp("GUILDFROMUID.", pszKey, 13) )
	{
		pszKey += 13;
		if ( !*pszKey )
			return false;

		CStoneMember *pMember = GetMember(static_cast<CGrayUID>(Exp_GetVal(pszKey)).ItemFind());
		SKIP_SEPARATORS(pszKey);
		if ( pMember )
		{
			pRef = pMember;
			return true;
		}
	}

	return CItem::r_GetRef(pszKey, pRef);
}

enum ISV_TYPE
{
	#define ADD(a,b) ISV_##a,
	#include "../tables/CItemStone_functions.tbl"
	#undef ADD
	ISV_QTY
};

LPCTSTR const CItemStone::sm_szVerbKeys[ISV_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CItemStone_functions.tbl"
	#undef ADD
	NULL
};

bool CItemStone::r_Verb(CScript &s, CTextConsole *pSrc)		// execute command from script
{
	ADDTOCALLSTACK("CItemStone::r_Verb");
	// NOTE:: Only call this from CChar::r_Verb
	// Little to no security checks here
	EXC_TRY("Verb");
	ASSERT(pSrc);

	int index = FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
		return CItem::r_Verb(s, pSrc);

	switch ( index )
	{
		case ISV_ALLGUILDS:
		{
			if ( s.HasArgs() )
			{
				TCHAR *pszArgs = s.GetArgRaw();
				int iFlags = Exp_GetVal(pszArgs);
				SKIP_ARGSEP(pszArgs);
				CScript script(pszArgs);
				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( !pMember->m_uidLinkTo.IsItem() )
						continue;

					if ( iFlags == 0 )
						pMember->r_Verb(script, pSrc);
					else if ( (iFlags == 1) && pMember->m_Enemy.m_fWeDeclared && !pMember->m_Enemy.m_fTheyDeclared )
						pMember->r_Verb(script, pSrc);
					else if ( (iFlags == 2) && !pMember->m_Enemy.m_fWeDeclared && pMember->m_Enemy.m_fTheyDeclared )
						pMember->r_Verb(script, pSrc);
					else if ( (iFlags == 3) && pMember->m_Enemy.m_fWeDeclared && pMember->m_Enemy.m_fTheyDeclared )
						pMember->r_Verb(script, pSrc);
				}
			}
			break;
		}
		case ISV_ALLMEMBERS:
		{
			if ( s.HasArgs() )
			{
				TCHAR *pszArgs = s.GetArgRaw();
				STONEPRIV_TYPE priv = static_cast<STONEPRIV_TYPE>(Exp_GetVal(pszArgs));
				SKIP_ARGSEP(pszArgs);
				CScript script(pszArgs);
				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( !pMember->m_uidLinkTo.IsChar() )
						continue;

					if ( (priv == -1) || (pMember->m_priv == priv) )
						pMember->r_Verb(script, pSrc);
				}
			}
			break;
		}
		case ISV_APPLYTOJOIN:
		{
			if ( s.HasArgs() )
			{
				CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
				if ( pChar )
					AddRecruit(pChar, STONEPRIV_CANDIDATE);
			}
			break;
		}
		case ISV_CHANGEALIGN:
		{
			if ( s.HasArgs() )
			{
				m_itStone.m_align = static_cast<STONEALIGN_TYPE>(s.GetArgVal());
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, "%s is now a %s %s\n", GetName(), GetAlignName(), GetTypeName());
				Speak(pszTemp);
			}
			break;
		}
		case ISV_DECLAREPEACE:
		{
			WeDeclarePeace(static_cast<CItemStone *>(static_cast<CGrayUID>(s.GetArgVal()).ItemFind()));
			break;
		}
		case ISV_DECLAREWAR:
		{
			WeDeclareWar(static_cast<CItemStone *>(static_cast<CGrayUID>(s.GetArgVal()).ItemFind()));
			break;
		}
		case ISV_ELECTMASTER:
		{
			ElectMaster();
			break;
		}
		case ISV_INVITEWAR:
		{
			if ( s.HasArgs() )
			{
				INT64 piCmd[2];
				size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty == 2 )
				{
					CGrayUID uidStone = static_cast<CGrayUID>(piCmd[0]);
					CItem *pStone = uidStone.ItemFind();
					if ( pStone && (pStone->IsType(IT_STONE_GUILD) || pStone->IsType(IT_STONE_TOWN)) )
					{
						CStoneMember *pMember = GetMember(pStone);
						if ( !pMember )
							pMember = new CStoneMember(this, uidStone, STONEPRIV_ENEMY);

						if ( piCmd[1] != 0 )
							pMember->m_Enemy.m_fWeDeclared = true;
						else
							pMember->m_Enemy.m_fTheyDeclared = true;
					}
				}
			}
			break;
		}
		case ISV_JOINASMEMBER:
		{
			AddRecruit(static_cast<CGrayUID>(s.GetArgVal()).CharFind(), STONEPRIV_MEMBER);
			break;
		}
		case ISV_RESIGN:
		{
			CStoneMember *pMember = GetMember(s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()).CharFind() : pSrc->GetChar());
			if ( pMember )
				delete pMember;
			break;
		}
		case ISV_TOGGLEABBREVIATION:
		{
			CStoneMember *pMemberSrc = GetMember(pSrc->GetChar());
			CGrayUID uidMember = s.HasArgs() ? static_cast<CGrayUID>(s.GetArgVal()) : (pMemberSrc ? pMemberSrc->m_uidLinkTo : static_cast<CGrayUID>(UID_CLEAR));
			CChar *pChar = uidMember.CharFind();

			CStoneMember *pMember = GetMember(pChar);
			if ( pMember )
			{
				pMember->m_Member.m_fAbbrev = !pMember->m_Member.m_fAbbrev;
				pChar->UpdatePropertyFlag();
			}
			break;
		}
		default:
		{
			return false;
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CItemStone::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemStone::r_Write");
	CItem::r_Write(s);
	s.WriteKeyVal("ALIGN", m_itStone.m_align);

	if ( !m_sAbbrev.IsEmpty() )
		s.WriteKey("ABBREV", m_sAbbrev);

	TCHAR *pszTemp = Str_GetTemp();
	for ( size_t i = 0; i < COUNTOF(m_sCharter); ++i )
	{
		if ( !m_sCharter[i].IsEmpty() )
		{
			sprintf(pszTemp, "CHARTER%" FMTSIZE_T, i);
			s.WriteKey(pszTemp, m_sCharter[i]);
		}
	}

	if ( !m_sWebPageURL.IsEmpty() )
		s.WriteKey("WEBPAGE", m_sWebPageURL);

	for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
	{
		if ( pMember->m_uidLinkTo.IsValidUID() )
		{
			s.WriteKeyFormat("MEMBER",
				"0%lx,%s,%d,0%lx,%d,%d,%d",
				static_cast<DWORD>(pMember->m_uidLinkTo | (pMember->m_uidLinkTo.IsItem() ? UID_F_ITEM : 0)),
				pMember->GetTitle(),
				pMember->m_priv,
				static_cast<DWORD>(pMember->m_uidLoyalTo),
				pMember->m_Unk.m_iVal1,
				pMember->m_Unk.m_iVal2,
				pMember->m_Member.m_iAccountGold);
		}
	}
}

enum STC_TYPE
{
	#define ADD(a,b) STC_##a,
	#include "../tables/CItemStone_props.tbl"
	#undef ADD
	STC_QTY
};

LPCTSTR const CItemStone::sm_szLoadKeys[STC_QTY + 1] =
{
	#define ADD(a,b) b,
	#include "../tables/CItemStone_props.tbl"
	#undef ADD
	NULL
};

bool CItemStone::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemStone::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp("MEMBER.", pszKey, 7) )
	{
		LPCTSTR pszCmd = pszKey + 7;
		if ( !strnicmp("COUNT", pszCmd, 5) )
		{
			pszCmd += 5;
			int i = 0;
			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				STONEPRIV_TYPE priv = static_cast<STONEPRIV_TYPE>(Exp_GetVal(pszCmd));

				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( !pMember->m_uidLinkTo.IsChar() )
						continue;
					if ( pMember->m_priv != priv )
						continue;
					++i;
				}
			}
			else
			{
				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( !pMember->m_uidLinkTo.IsChar() )
						continue;
					++i;
				}
			}
			sVal.FormatVal(i);
			return true;
		}

		int i = 0;
		int iNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);
		sVal.FormatVal(0);

		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( !pMember->m_uidLinkTo.IsChar() )
				continue;

			if ( iNumber == i )
			{
				if ( !pszCmd[0] )
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}
			++i;
		}
		return true;
	}
	else if ( !strnicmp("MEMBERFROMUID.", pszKey, 14) )
	{
		LPCTSTR pszCmd = pszKey + 14;
		CStoneMember *pMember = GetMember(static_cast<CGrayUID>(Exp_GetVal(pszCmd)).CharFind());
		SKIP_SEPARATORS(pszCmd);
		if ( pMember )
			return pMember->r_WriteVal(pszCmd, sVal, pSrc);

		sVal.FormatVal(0);
		return true;
	}
	else if ( !strnicmp("GUILD.", pszKey, 6) )
	{
		LPCTSTR pszCmd = pszKey + 6;
		if ( !strnicmp("COUNT", pszCmd, 5) )
		{
			pszCmd += 5;
			int i = 0;
			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				int iFlags = Exp_GetVal(pszCmd);

				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( pMember->m_uidLinkTo.IsChar() )
						continue;

					if ( (iFlags == 1) && pMember->m_Enemy.m_fWeDeclared && !pMember->m_Enemy.m_fTheyDeclared )
						++i;
					else if ( (iFlags == 2) && !pMember->m_Enemy.m_fWeDeclared && pMember->m_Enemy.m_fTheyDeclared )
						++i;
					else if ( (iFlags == 3) && pMember->m_Enemy.m_fWeDeclared && pMember->m_Enemy.m_fTheyDeclared )
						++i;
				}
			}
			else
			{
				for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
				{
					if ( pMember->m_uidLinkTo.IsChar() )
						continue;
					++i;
				}
			}
			sVal.FormatVal(i);
			return true;
		}

		int i = 0;
		int iNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);
		sVal.FormatVal(0);

		for ( CStoneMember *pMember = static_cast<CStoneMember *>(GetHead()); pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( pMember->m_uidLinkTo.IsChar() )
				continue;

			if ( iNumber == i )
			{
				if ( !pszCmd[0] )
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}
			++i;
		}
		return true;
	}
	else if ( !strnicmp("GUILDFROMUID.", pszKey, 13) )
	{
		LPCTSTR pszCmd = pszKey + 13;
		CStoneMember *pMember = GetMember(static_cast<CGrayUID>(Exp_GetVal(pszCmd)).ItemFind());
		SKIP_SEPARATORS(pszCmd);
		if ( pMember )
			return pMember->r_WriteVal(pszCmd, sVal, pSrc);

		sVal.FormatVal(0);
		return true;
	}
	else if ( !strnicmp("CHARTER", pszKey, 7) )
	{
		size_t i = static_cast<size_t>(ATOI(pszKey + 7));
		sVal = (i < COUNTOF(m_sCharter)) ? m_sCharter[i] : NULL;
		return true;
	}

	int index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case STC_ABBREV:
		{
			sVal = m_sAbbrev;
			return true;
		}
		case STC_ALIGN:
		{
			sVal.FormatVal(m_itStone.m_align);
			return true;
		}
		case STC_WEBPAGE:
		{
			sVal = m_sWebPageURL;
			return true;
		}
		case STC_ABBREVIATIONTOGGLE:
		{
			CStoneMember *pMember = GetMember(pSrc->GetChar());
			CVarDefCont *pVar = NULL;
			if ( pMember )
				pVar = pMember->IsAbbrevOn() ? g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVON") : g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVOFF");
			else
				pVar = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");

			sVal = pVar ? pVar->GetValStr() : "";
			return true;
		}
		case STC_ALIGNTYPE:
		{
			sVal = GetAlignName();
			return true;
		}
		case STC_LOYALTO:
		{
			CStoneMember *pMember = GetMember(pSrc->GetChar());
			CVarDefCont *pVar = NULL;
			if ( pMember )
			{
				CChar *pChar = pMember->m_uidLoyalTo.CharFind();
				if ( !pChar || (pChar == pSrc->GetChar()) )
					pVar = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_YOURSELF");
				else
				{
					sVal = pChar->GetName();
					return true;
				}
			}
			else
				pVar = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");

			sVal = pVar ? pVar->GetValStr() : "";
			return true;
		}
		case STC_MASTER:
		{
			CChar *pChar = GetMaster();
			sVal = pChar ? pChar->GetName() : g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_PENDVOTE");
			return true;
		}
		case STC_MASTERGENDERTITLE:
		{
			CChar *pChar = GetMaster();
			if ( !pChar )
				sVal = "";
			else if ( pChar->Char_GetDef()->IsFemale() )
				sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERFEMALE");
			else
				sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERMALE");
			return true;
		}
		case STC_MASTERTITLE:
		{
			CStoneMember *pMember = GetMasterMember();
			sVal = pMember ? pMember->GetTitle() : "";
			return true;
		}
		case STC_MASTERUID:
		{
			CChar *pChar = GetMaster();
			sVal.FormatHex(pChar ? static_cast<DWORD>(pChar->GetUID()) : UID_CLEAR);
			return true;
		}
		default:
		{
			return CItem::r_WriteVal(pszKey, sVal, pSrc);
		}
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemStone::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemStone::r_LoadVal");
	EXC_TRY("LoadVal");

	int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case STC_ABBREV:
		{
			m_sAbbrev = s.GetArgStr();
			return true;
		}
		case STC_ALIGN:
		{
			m_itStone.m_align = static_cast<STONEALIGN_TYPE>(s.GetArgVal());
			return true;
		}
		case STC_MASTERUID:
		{
			CStoneMember *pNewMaster = GetMember(static_cast<CGrayUID>(s.GetArgVal()).CharFind());
			if ( !pNewMaster )
				return false;

			CStoneMember *pCurrentMaster = GetMasterMember();
			if ( pCurrentMaster )
				pCurrentMaster->m_priv = STONEPRIV_MEMBER;

			pNewMaster->m_priv = STONEPRIV_MASTER;
			return true;
		}
		case STC_MEMBER:
		{
			TCHAR *ppArgs[8];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs), ",");
			if ( iArgQty < 1 )		// must provide at least the member uid
				return false;

			new CStoneMember(
				this,
				ahextoi(ppArgs[0]),
				(iArgQty > 2) ? static_cast<STONEPRIV_TYPE>(ATOI(ppArgs[2])) : STONEPRIV_CANDIDATE,
				(iArgQty > 1) ? ppArgs[1] : "",
				(iArgQty > 3) ? ahextoi(ppArgs[3]) : UID_CLEAR,
				(iArgQty > 4) ? (ATOI(ppArgs[4]) != 0) : false,
				(iArgQty > 5) ? (ATOI(ppArgs[5]) != 0) : false,
				(iArgQty > 6) ? ATOI(ppArgs[6]) : 0);
			return true;
		}
		case STC_WEBPAGE:
		{
			m_sWebPageURL = s.GetArgStr();
			return true;
		}
	}

	if ( s.IsKeyHead("CHARTER", 7) )
	{
		size_t i = static_cast<size_t>(ATOI(s.GetKey() + 7));
		if ( i < COUNTOF(m_sCharter) )
		{
			m_sCharter[i] = s.GetArgStr();
			return true;
		}
		return false;
	}

	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
