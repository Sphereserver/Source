//
// CItemStone.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

//////////
// -CStoneMember

enum STMM_TYPE
{
#define ADD(a,b) STMM_##a,
#include "../tables/CStoneMember_props.tbl"
#undef ADD
	STMM_QTY
};

LPCTSTR const CStoneMember::sm_szLoadKeys[STMM_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CStoneMember_props.tbl"
#undef ADD
	NULL
};

enum STMMV_TYPE
{
#define ADD(a,b) STMMV_##a,
#include "../tables/CStoneMember_functions.tbl"
#undef ADD
	STMMV_QTY
};

LPCTSTR const CStoneMember::sm_szVerbKeys[STMMV_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CStoneMember_functions.tbl"
#undef ADD
	NULL
};

CStoneMember* CStoneMember::GetNext() const
{
	ADDTOCALLSTACK("CStoneMember::GetNext");
	return( STATIC_CAST <CStoneMember *>( CGObListRec::GetNext()));
}

CGrayUID CStoneMember::GetLinkUID() const { return m_uidLinkTo; }

STONEPRIV_TYPE CStoneMember::GetPriv() const { return m_iPriv; }
void CStoneMember::SetPriv(STONEPRIV_TYPE iPriv) { m_iPriv = iPriv; }
bool CStoneMember::IsPrivMaster() const { return m_iPriv == STONEPRIV_MASTER; }
bool CStoneMember::IsPrivMember() const { return( m_iPriv == STONEPRIV_MASTER || m_iPriv == STONEPRIV_MEMBER ); }

// If the member is really a war flag (STONEPRIV_ENEMY)
void CStoneMember::SetWeDeclared(bool f)
{
	ADDTOCALLSTACK("CStoneMember::SetWeDeclared");
	m_Enemy.m_fWeDeclared = f;
}
bool CStoneMember::GetWeDeclared() const
{
	ADDTOCALLSTACK("CStoneMember::GetWeDeclared");
	return ( m_Enemy.m_fWeDeclared ) ? true : false;
}
void CStoneMember::SetTheyDeclared(bool f)
{
	ADDTOCALLSTACK("CStoneMember::SetTheyDeclared");
	m_Enemy.m_fTheyDeclared = f;
}
bool CStoneMember::GetTheyDeclared() const
{
	ADDTOCALLSTACK("CStoneMember::GetTheyDeclared");
	return ( m_Enemy.m_fTheyDeclared ) ? true : false;
}

// Member
bool CStoneMember::IsAbbrevOn() const { return ( m_Member.m_fAbbrev ) ? true : false; }
void CStoneMember::ToggleAbbrev() { m_Member.m_fAbbrev = !m_Member.m_fAbbrev; }
void CStoneMember::SetAbbrev(bool mode) { m_Member.m_fAbbrev = mode; }

LPCTSTR CStoneMember::GetTitle() const
{
	ADDTOCALLSTACK("CStoneMember::GetTitle");
	return( m_sTitle );
}
void CStoneMember::SetTitle( LPCTSTR pTitle )
{
	ADDTOCALLSTACK("CStoneMember::SetTitle");
	m_sTitle = pTitle;
}

CGrayUID CStoneMember::GetLoyalToUID() const
{
	ADDTOCALLSTACK("CStoneMember::GetLoyalToUID");
	return( m_uidLoyalTo );
}

int CStoneMember::GetAccountGold() const
{
	ADDTOCALLSTACK("CStoneMember::GetAccountGold");
	return( m_Member.m_iAccountGold );
}
void CStoneMember::SetAccountGold( int iGold )
{
	ADDTOCALLSTACK("CStoneMember::SetAccountGold");
	m_Member.m_iAccountGold = iGold;
}

bool CStoneMember::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	UNREFERENCED_PARAMETER(pszKey);
	UNREFERENCED_PARAMETER(pRef);
	return false;
}

bool CStoneMember::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CStoneMember::r_Verb");
	EXC_TRY("Verb");

	ASSERT(pSrc);
	
	LPCTSTR pszKey = s.GetKey();
	int index = FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		if ( r_LoadVal(s) ) // if it's successful all ok, else go on verb.
		{
			return true;
		}
	}

	if ( GetLinkUID().IsChar() )
	{
		CScriptObj *pRef = GetLinkUID().CharFind();
		return pRef->r_Verb( s, pSrc );
	}
	else if ( GetLinkUID().IsItem() )
	{
		CScriptObj *pRef = GetLinkUID().ItemFind();
		return pRef->r_Verb( s, pSrc );
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}


bool CStoneMember::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CStoneMember::r_LoadVal");
	EXC_TRY("LoadVal");

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ACCOUNTGOLD:
				SetAccountGold(s.GetArgVal());
				break;

			case STMM_LOYALTO:
				{
					CGrayUID uid = s.GetArgVal();
					SetLoyalTo(uid.CharFind());
				}
				break;

			case STMM_PRIV:
				SetPriv(static_cast<STONEPRIV_TYPE>(s.GetArgVal()));
				break;

			case STMM_TITLE:
				SetTitle(s.GetArgStr());
				break;

			case STMM_SHOWABBREV:
				SetAbbrev( s.GetArgVal() ? 1 : 0 );
				break;

			default:
				return false;
		}
	} 
	else if ( GetLinkUID().IsItem() )
	{
		switch ( iIndex )
		{
			case STMM_GUILD_THEYALLIANCE:
				break;

			case STMM_GUILD_THEYWAR:
				SetTheyDeclared(s.GetArgVal() ? true : false);
				break;

			case STMM_GUILD_WEALLIANCE:
				break;

			case STMM_GUILD_WEWAR:
				SetWeDeclared(s.GetArgVal() ? true : false);
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


bool CStoneMember::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CStoneMember::r_WriteVal");
	EXC_TRY("WriteVal");

	STMM_TYPE iIndex = (STMM_TYPE) FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	if ( GetLinkUID().IsChar() )
	{
		switch ( iIndex )
		{
			case STMM_ACCOUNTGOLD:
				sVal.FormatVal(GetAccountGold());
				break;
			case STMM_ISMASTER:
				sVal.FormatVal(IsPrivMaster());
				break;
			case STMM_ISMEMBER:
				sVal.FormatVal(IsPrivMember());
				break;
			case STMM_LOYALTO:
				sVal.FormatHex(GetLoyalToUID());
				break;
			case STMM_PRIV:
				sVal.FormatVal(GetPriv());
				break;
			case STMM_PRIVNAME:
				sVal = GetPrivName();
				break;
			case STMM_TITLE:
				sVal = GetTitle();
				break;
			case STMM_SHOWABBREV:
				sVal.FormatVal(IsAbbrevOn());
				break;
			case STMM_ISCANDIDATE:
				sVal.FormatVal((GetPriv() == STONEPRIV_CANDIDATE) ? 1 : 0);
				break;
			default:
				{
					CScriptObj *pRef = GetLinkUID().CharFind();
					return pRef->r_WriteVal(pszKey,sVal,pSrc);
				}
		}
	}
	else if ( GetLinkUID().IsItem() )
	{
		switch ( iIndex )
		{
			case STMM_GUILD_ISENEMY:
				sVal.FormatVal(GetWeDeclared() && GetTheyDeclared());
				break;
			case STMM_GUILD_THEYWAR:
				sVal.FormatVal(GetTheyDeclared());
				break;
			case STMM_GUILD_WEWAR:
				sVal.FormatVal(GetWeDeclared());
				break;
			case STMM_GUILD_WEALLIANCE:
				sVal.FormatVal(0);
				break;
			case STMM_GUILD_THEYALLIANCE:
				sVal.FormatVal(0);
				break;
			default:
				{
					CScriptObj *pRef = GetLinkUID().ItemFind();
					return pRef->r_WriteVal(pszKey,sVal,pSrc);
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

CStoneMember::CStoneMember( CItemStone * pStone, CGrayUID uid, STONEPRIV_TYPE iType, LPCTSTR pTitle, CGrayUID loyaluid, bool fVal1, bool fVal2, int nAccountGold)
{
	m_uidLinkTo = uid;
	m_sTitle = pTitle;
	m_iPriv = iType;
	m_uidLoyalTo = loyaluid;

	// union.
	if ( iType == STONEPRIV_ENEMY )
	{
		m_Enemy.m_fTheyDeclared = fVal1;
		m_Enemy.m_fWeDeclared = fVal2;
	}
	else
	{
		m_Member.m_fAbbrev = fVal1;
		m_Member.m_iVoteTally = fVal2;		// Temporary space to calculate votes.
	}

	m_Member.m_iAccountGold = nAccountGold;

	if ( ! g_Serv.IsLoading() && pStone->GetMemoryType())
	{
		CChar * pChar = uid.CharFind();
		if ( pChar != NULL )
		{
			pChar->Memory_AddObjTypes( pStone, pStone->GetMemoryType());
			if ( pStone->IsTopLevel())
			{
				pChar->m_ptHome = pStone->GetTopPoint();	// Our new home.
			}
		}
	}

	pStone->InsertTail( this );
}

CStoneMember::~CStoneMember()
{
	CItemStone * pStone = GetParentStone();
	if ( ! pStone )
		return;

	RemoveSelf();
#ifndef _NEWGUILDSYSTEM
	pStone->ElectMaster();	
#endif

	if ( m_iPriv == STONEPRIV_ENEMY )
	{
		// same as declaring peace.
		CItemStone * pStoneEnemy = dynamic_cast <CItemStone *>( GetLinkUID().ItemFind());
		if ( pStoneEnemy != NULL )
		{
			pStoneEnemy->TheyDeclarePeace( pStone, true );
		}
	}
	else if ( pStone->GetMemoryType())
	{
#ifdef _NEWGUILDSYSTEM
		// If we remove a char with good loyalty we may have changed the vote count.
		pStone->ElectMaster();	
#endif

		CChar * pChar = GetLinkUID().CharFind();
		if ( pChar )
		{
			pChar->Memory_ClearTypes( pStone->GetMemoryType()); 	// Make them forget they were ever in this guild
		}
	}
}

CItemStone * CStoneMember::GetParentStone() const
{
	ADDTOCALLSTACK("CStoneMember::GetParentStone");
	return dynamic_cast <CItemStone *>( GetParent());
}

LPCTSTR CStoneMember::GetPrivName() const
{
	ADDTOCALLSTACK("CStoneMember::GetPrivName");

#ifndef _NEWGUILDSYSTEM
	switch ( GetPriv() )
	{
		case STONEPRIV_CANDIDATE: return "a candidate of";
		case STONEPRIV_MEMBER: return "a member of";
		case STONEPRIV_MASTER: return "the master of";
		case STONEPRIV_ACCEPTED: return "accepted in";
		case STONEPRIV_ENEMY: return "the enemy of";
		default: return "unknown";
	}

#else
	STONEPRIV_TYPE iPriv = GetPriv();

	TemporaryString sDefname;
	sprintf(sDefname, "STONECONFIG_PRIVNAME_PRIVID-%d", static_cast<int>(iPriv));
	
	CVarDefCont * pResult = g_Exp.m_VarDefs.GetKey(sDefname);
	if (pResult)
		return pResult->GetValStr();
	else
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_PRIVNAME_PRIVUNK");

	return ( pResult == NULL ) ? "" : pResult->GetValStr();
#endif
}

bool CStoneMember::SetLoyalTo( const CChar * pCharLoyal )
{
	ADDTOCALLSTACK("CStoneMember::SetLoyalTo");
	CChar * pCharMe = GetLinkUID().CharFind();
	if ( pCharMe == NULL )	// on shutdown
		return false;

	m_uidLoyalTo = GetLinkUID();	// set to self for default.
	if ( pCharLoyal == NULL )
		return true;

	if ( ! IsPrivMember())
	{
		// non members can't vote
		pCharMe->SysMessage("Candidates aren't elligible to vote.");
		return false;
	}

	CItemStone * pStone = GetParentStone();
	if ( !pStone )
		return false;

	CStoneMember * pNewLoyalTo = pStone->GetMember(pCharLoyal);
	if ( pNewLoyalTo == NULL || ! pNewLoyalTo->IsPrivMember())
	{
		// you can't vote for candidates
		pCharMe->SysMessage( "Can only vote for full members.");
		return false;
	}

	m_uidLoyalTo = pCharLoyal->GetUID();

	// new vote tally
	pStone->ElectMaster();
	return( true );
}

//////////
// -CItemStone

CItemStone::CItemStone( ITEMID_TYPE id, CItemBase * pItemDef ) : CItem( id, pItemDef )
{
	m_itStone.m_iAlign = STONEALIGN_STANDARD;
	g_World.m_Stones.Add( this );
}

CItemStone::~CItemStone()
{
	SetAmount(0);	// Tell everyone we are deleting.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.

	// Remove this stone from the links of guilds in the world
	g_World.m_Stones.RemovePtr( this );

	// all members are deleted automatically.
	Empty();	// do this manually to preserve the parents type cast
}

MEMORY_TYPE CItemStone::GetMemoryType() const
{
	ADDTOCALLSTACK("CItemStone::GetMemoryType");
	switch ( GetType() )
	{
		case IT_STONE_TOWN: return( MEMORY_TOWN );
		case IT_STONE_GUILD: return( MEMORY_GUILD );
		default: return( MEMORY_NONE ); // Houses have no memories.
	}
}

LPCTSTR CItemStone::GetCharter(unsigned int iLine) const
{
	ASSERT(iLine<COUNTOF(m_sCharter));
	return(  m_sCharter[iLine] );
}

void CItemStone::SetCharter( unsigned int iLine, LPCTSTR pCharter )
{
	ASSERT(iLine<COUNTOF(m_sCharter));
	m_sCharter[iLine] = pCharter;
}

LPCTSTR CItemStone::GetWebPageURL() const
{
	return( m_sWebPageURL );
}

void CItemStone::SetWebPage( LPCTSTR pWebPage )
{
	m_sWebPageURL = pWebPage;
}

STONEALIGN_TYPE CItemStone::GetAlignType() const
{
	return m_itStone.m_iAlign;
}

void CItemStone::SetAlignType(STONEALIGN_TYPE iAlign)
{
	m_itStone.m_iAlign = iAlign;
}

LPCTSTR CItemStone::GetAbbrev() const
{
	return( m_sAbbrev );
}

void CItemStone::SetAbbrev( LPCTSTR pAbbrev )
{
	m_sAbbrev = pAbbrev;
}

LPCTSTR CItemStone::GetTypeName() const
{
	ADDTOCALLSTACK("CItemStone::GetTypeName");

#ifndef _NEWGUILDSYSTEM
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			return "Guild";
		case IT_STONE_TOWN:
			return "Town";
		default:
			return "Unk";
	}

#else
	CVarDefCont * pResult = NULL;
	
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_GUILD");
			break;
		case IT_STONE_TOWN:
			pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_TOWN");
			break;
		default:
			break;
	}

	if ( pResult == NULL )
		pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_TYPENAME_UNK");

	return ( pResult == NULL ) ? "" : pResult->GetValStr();
#endif
}

void CItemStone::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemStone::r_Write");
	CItem::r_Write( s );
	s.WriteKeyVal( "ALIGN", GetAlignType());
	if ( ! m_sAbbrev.IsEmpty())
	{
		s.WriteKey( "ABBREV", m_sAbbrev );
	}

	TemporaryString pszTemp;
	for ( unsigned int i = 0; i < COUNTOF(m_sCharter); i++ )
	{
		if ( ! m_sCharter[i].IsEmpty())
		{
			sprintf(pszTemp, "CHARTER%u", i);
			s.WriteKey(pszTemp, m_sCharter[i] );
		}
	}

	if ( ! m_sWebPageURL.IsEmpty())
	{
		s.WriteKey( "WEBPAGE", GetWebPageURL() );
	}

	// s.WriteKey( "//", "uid,title,priv,loyaluid,abbr&theydecl,wedecl");

	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if (pMember->GetLinkUID().IsValidUID()) // To protect against characters that were deleted!
		{
			s.WriteKeyFormat( "MEMBER",
				"0%lx,%s,%i,0%lx,%i,%i,%i",
				(DWORD) pMember->GetLinkUID() | (pMember->GetLinkUID().IsItem() ? UID_F_ITEM : 0),
				static_cast<LPCTSTR>(pMember->GetTitle()),
				pMember->GetPriv(),
				static_cast<DWORD>(pMember->GetLoyalToUID()),
				pMember->m_UnDef.m_Val1,
				pMember->m_UnDef.m_Val2,
				pMember->GetAccountGold());
		}
	}
}

LPCTSTR CItemStone::GetAlignName() const
{
	ADDTOCALLSTACK("CItemStone::GetAlignName");
#ifndef _NEWGUILDSYSTEM
	static LPCTSTR const sm_AlignName[] = // STONEALIGN_TYPE
	{
		"standard",	// STONEALIGN_STANDARD
		"Order",	// STONEALIGN_ORDER
		"Chaos"		// STONEALIGN_CHAOS
	};
	int iAlign = GetAlignType();
	if ( iAlign >= COUNTOF( sm_AlignName ))
		iAlign = 0;
	return( sm_AlignName[ iAlign ] );
#else
	int iAlign = GetAlignType();

	TemporaryString sDefname;
	if ( GetType() == IT_STONE_GUILD )
	{
		sprintf(sDefname, "GUILDCONFIG_ALIGN_%d", iAlign);
	}
	else if ( GetType() == IT_STONE_TOWN )
	{
		sprintf(sDefname, "TOWNSCONFIG_ALIGN_%d", iAlign);
	}
	else
	{
		return "";
	}

	LPCTSTR sRes = g_Exp.m_VarDefs.GetKeyStr(sDefname);
	return ( sRes == NULL ) ? "" : sRes;
#endif
}

enum STC_TYPE
{
#define ADD(a,b) STC_##a,
#include "../tables/CItemStone_props.tbl"
#undef ADD
	STC_QTY
};

LPCTSTR const CItemStone::sm_szLoadKeys[STC_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CItemStone_props.tbl"
#undef ADD
	NULL
};

enum ISV_TYPE
{
#define ADD(a,b) ISV_##a,
#include "../tables/CItemStone_functions.tbl"
#undef ADD
	ISV_QTY
};

LPCTSTR const CItemStone::sm_szVerbKeys[ISV_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CItemStone_functions.tbl"
#undef ADD
	NULL
};

bool CItemStone::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemStone::r_GetRef");
	if ( !strnicmp("member.", pszKey, 7) )
	{
		pszKey = pszKey + 7;
		if ( !pszKey[0] )
			return false;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());

		for ( int i = 0; pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( !pMember->GetLinkUID().IsChar() ) 
				continue;

			if ( nNumber == i )
			{
				pRef = pMember; 
				return true;
			}

			i++;
		}
	}
	else if ( !strnicmp("memberfromuid.", pszKey, 14) )
	{
		pszKey = pszKey + 14;
		if ( !pszKey[0] )
			return false;

		CGrayUID pMemberUid = static_cast<DWORD>(Exp_GetVal(pszKey));
		SKIP_SEPARATORS(pszKey);

		CChar * pMemberChar = pMemberUid.CharFind();
		if ( pMemberChar )
		{
			CStoneMember * pMemberGuild = GetMember( pMemberChar );
			if ( pMemberGuild )
			{
				pRef = pMemberGuild; 
				return true;
			}
		}
	}
	else if ( !strnicmp("guild.", pszKey, 6) )
	{
		pszKey = pszKey + 6;
		if ( !pszKey[0] )
			return false;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());

		for ( int i = 0; pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( pMember->GetLinkUID().IsChar() ) 
				continue;

			if ( nNumber == i )
			{
				pRef = pMember;
				return true;
			}

			i++;
		}
	}
	else if ( !strnicmp("guildfromuid.", pszKey, 13) )
	{
		pszKey = pszKey + 13;
		if ( !pszKey[0] )
			return false;

		CGrayUID pGuildUid = static_cast<DWORD>(Exp_GetVal(pszKey));
		SKIP_SEPARATORS(pszKey);

		CItem * pMemberGuild = pGuildUid.ItemFind();
		if ( pMemberGuild )
		{
			CStoneMember * pGuild = GetMember( pMemberGuild );
			if ( pGuild )
			{
				pRef = pGuild; 
				return true;
			}
		}
	}

	return CItem::r_GetRef( pszKey, pRef );
}

bool CItemStone::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CItemStone::r_LoadVal");
	EXC_TRY("LoadVal");

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case STC_ABBREV: // "ABBREV"
			m_sAbbrev = s.GetArgStr();
			return true;
		case STC_ALIGN: // "ALIGN"
			SetAlignType(static_cast<STONEALIGN_TYPE>(s.GetArgVal()));
			return true;
		case STC_MasterUid:
			{
				if ( s.HasArgs() )
				{
					CGrayUID pNewMasterUid = (DWORD) s.GetArgVal();
					CChar * pChar = pNewMasterUid.CharFind();
					if ( !pChar )
					{
						DEBUG_ERR(( "MASTERUID called on non char 0%lx uid.\n", (DWORD)pNewMasterUid ));
						return( false );
					}

					CStoneMember * pNewMaster = GetMember( pChar );
					if ( !pNewMaster )
					{
						DEBUG_ERR(( "MASTERUID called on char 0%lx (%s) that is not a valid member of stone with 0x%lx uid.\n", (DWORD)pNewMasterUid, pChar->GetName(), (DWORD)GetUID() ));
						return( false );
					}

					CStoneMember * pMaster = GetMasterMember();
					if ( pMaster )
					{
						if ( pMaster->GetLinkUID() == pNewMasterUid )
							return( true );

						pMaster->SetPriv(STONEPRIV_MEMBER);
						//pMaster->SetLoyalTo(pChar);
					}

					//pNewMaster->SetLoyalTo(pChar);
					pNewMaster->SetPriv(STONEPRIV_MASTER);
				}
				else
				{
					DEBUG_ERR(( "MASTERUID called without arguments.\n" ));
					return( false );
				}
			}
			return( true );
		case STC_MEMBER: // "MEMBER"
			{
			TCHAR *Arg_ppCmd[8];		// Maximum parameters in one line
			size_t Arg_Qty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ), "," );
			if (Arg_Qty < 1) // must at least provide the member uid
				return false;

			new CStoneMember(
				this,
				ahextoi(Arg_ppCmd[0]),													// Member's UID
				Arg_Qty > 2 ? static_cast<STONEPRIV_TYPE>(ATOI(Arg_ppCmd[2])) : STONEPRIV_CANDIDATE,	// Members priv level (use as a type)
				Arg_Qty > 1 ? Arg_ppCmd[1] : "",										// Title
				ahextoi(Arg_ppCmd[3]),													// Member is loyal to this
				Arg_Qty > 4 ? (ATOI( Arg_ppCmd[4] ) != 0) : 0,							// Paperdoll stone abbreviation (also if they declared war)
				Arg_Qty > 5 ? (ATOI( Arg_ppCmd[5] ) != 0) : 0,							// If we declared war
				Arg_Qty > 6 ? ATOI( Arg_ppCmd[6] ) : 0);								// AccountGold
			}
			return true;
		case STC_WEBPAGE: // "WEBPAGE"
			m_sWebPageURL = s.GetArgStr();
			return true;
	}

	if ( s.IsKeyHead( sm_szLoadKeys[STC_CHARTER], 7 ))
	{
		unsigned int i = ATOI(s.GetKey() + 7);
		if ( i >= COUNTOF(m_sCharter))
			return( false );
		m_sCharter[i] = s.GetArgStr();
		return( true );
	}
	
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItemStone::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemStone::r_WriteVal");
	EXC_TRY("WriteVal");
	CChar * pCharSrc = pSrc->GetChar();

	if ( !strnicmp("member.",pszKey,7) )
	{
		LPCTSTR pszCmd = pszKey + 7;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				STONEPRIV_TYPE iPriv = static_cast<STONEPRIV_TYPE>(Exp_GetVal(pszCmd));

				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if ( !pMember->GetLinkUID().IsChar() )
						continue;

					if ( pMember->GetPriv() != iPriv )
						continue;

					i++;
				}
			}
			else
			{
				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if (!pMember->GetLinkUID().IsChar()) 
						continue;

					i++;
				}
			}

			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != NULL; pMember = pMember->GetNext() )
		{
			if (!pMember->GetLinkUID().IsChar()) 
				continue;
				
			if ( nNumber == i )
			{
				if (!pszCmd[0]) 
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}

			i++;
		}

		return true;
	}
	else if ( !strnicmp("memberfromuid.", pszKey, 14) )
	{
		LPCTSTR pszCmd = pszKey + 14;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CGrayUID pMemberUid = static_cast<DWORD>(Exp_GetVal(pszCmd));
		SKIP_SEPARATORS(pszCmd);

		CChar * pMemberChar = pMemberUid.CharFind();
		if ( pMemberChar )
		{
			CStoneMember * pMemberGuild = GetMember( pMemberChar );
			if ( pMemberGuild )
			{
				return pMemberGuild->r_WriteVal(pszCmd, sVal, pSrc);
			}
		}

		return true;
	}
	else if ( !strnicmp("guild.",pszKey,6) )
	{
		LPCTSTR pszCmd = pszKey + 6;

		if ( !strnicmp("COUNT",pszCmd,5) )
		{
			pszCmd = pszCmd + 5;

			int i = 0;
			CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());

			if ( *pszCmd )
			{
				SKIP_ARGSEP(pszCmd);
				int iToCheck = Exp_GetVal(pszCmd);

				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if ( pMember->GetLinkUID().IsChar() )
						continue;

					if ( ( iToCheck == 1 ) && ( pMember->GetWeDeclared() && !pMember->GetTheyDeclared() ) )
						i++;
					else if ( ( iToCheck == 2 ) && ( !pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
						i++;
					else if ( ( iToCheck == 3 ) && ( pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
						i++;
				}
			}
			else
			{
				for (; pMember != NULL; pMember = pMember->GetNext())
				{
					if (pMember->GetLinkUID().IsChar()) 
						continue;

					i++;
				}
			}

			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for ( int i = 0 ; pMember != NULL; pMember = pMember->GetNext() )
		{
			if (pMember->GetLinkUID().IsChar()) 
				continue;
				
			if ( nNumber == i )
			{
				if (!pszCmd[0]) 
					return true;

				return pMember->r_WriteVal(pszCmd, sVal, pSrc);
			}

			i++;
		}

		return true;
	}
	else if ( !strnicmp("guildfromuid.", pszKey, 13) )
	{
		LPCTSTR pszCmd = pszKey + 13;
		sVal.FormatVal(0);

		if ( !pszCmd[0] )
			return true;

		CGrayUID pGuildUid = static_cast<DWORD>(Exp_GetVal(pszCmd));
		SKIP_SEPARATORS(pszCmd);

		CItem * pMemberGuild = pGuildUid.ItemFind();
		if ( pMemberGuild )
		{
			CStoneMember * pGuild = GetMember( pMemberGuild );
			if ( pGuild )
			{
				return pGuild->r_WriteVal(pszCmd, sVal, pSrc);
			}
		}

		return true;
	}
	else if ( !strnicmp(sm_szLoadKeys[STC_CHARTER], pszKey, 7) )
	{
		LPCTSTR pszCmd = pszKey + 7;
		unsigned int i = ATOI(pszCmd);
		if ( i >= COUNTOF(m_sCharter))
			sVal = "";
		else
			sVal = m_sCharter[i];

		return( true );
	}


	STC_TYPE iIndex = (STC_TYPE) FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );

	switch ( iIndex )
	{
		case STC_ABBREV: // "ABBREV"
			sVal = m_sAbbrev;
			return true;
		case STC_ALIGN:
			sVal.FormatVal( GetAlignType());
			return true;
		case STC_WEBPAGE: // "WEBPAGE"
			sVal = GetWebPageURL();
			return true;
		case STC_AbbreviationToggle:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
#ifndef _NEWGUILDSYSTEM
				if ( pMember == NULL )
				{
					sVal = "nonmember";
				}
				else
				{
					sVal = pMember->IsAbbrevOn() ? "On" : "Off";
				}
#else
				CVarDefCont * pResult = NULL;

				if ( pMember == NULL )
				{
					pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");
				}
				else
				{
					pResult = pMember->IsAbbrevOn() ? g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVON") :
								g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_ABBREVOFF");
				}

				sVal = pResult ? pResult->GetValStr() : "";
#endif
			}
			return true;
		case STC_AlignType:
			sVal = GetAlignName();
			return true;

		case STC_LoyalTo:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
#ifndef _NEWGUILDSYSTEM

				if ( pMember == NULL )
				{
					sVal = "nonmember";
				}
				else
				{
					CChar * pLoyalTo = pMember->GetLoyalToUID().CharFind();
					if ((pLoyalTo == NULL) || (pLoyalTo == pCharSrc ))
					{
						sVal = "yourself";
					}
					else
					{
						sVal = pLoyalTo->GetName();
					}
				}
#else
				CVarDefCont * pResult = NULL;

				if ( pMember == NULL )
				{
					pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_NONMEMBER");
				}
				else
				{
					CChar * pLoyalTo = pMember->GetLoyalToUID().CharFind();
					if ((pLoyalTo == NULL) || (pLoyalTo == pCharSrc ))
					{
						pResult = g_Exp.m_VarDefs.GetKey("STONECONFIG_VARIOUSNAME_YOURSELF");
					}
					else
					{
						sVal = pLoyalTo->GetName();
						return true;
					}
				}

				sVal = pResult ? pResult->GetValStr() : "";
#endif
			}
			return( true );
	
		case STC_Master:
			{
				CChar * pMaster = GetMaster();
#ifdef _NEWGUILDSYSTEM
				sVal = (pMaster) ? pMaster->GetName() : g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_PENDVOTE");
#else
				sVal = (pMaster) ? pMaster->GetName() : "vote pending";
#endif
			}
			return( true );
	
		case STC_MasterGenderTitle:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster == NULL )
					sVal = ""; // If no master (vote pending)
				else if ( pMaster->Char_GetDef()->IsFemale())
#ifndef _NEWGUILDSYSTEM
					sVal = "Mistress";
#else
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERFEMALE");
#endif
				else
#ifndef _NEWGUILDSYSTEM
					sVal = "Master";
#else
					sVal = g_Exp.m_VarDefs.GetKeyStr("STONECONFIG_VARIOUSNAME_MASTERGENDERMALE");
#endif
			}
			return( true );
	
		case STC_MasterTitle:
			{
				CStoneMember * pMember = GetMasterMember();
				sVal = (pMember) ? pMember->GetTitle() : "";
			}
			return( true );
	
		case STC_MasterUid:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster )
					sVal.FormatHex( (DWORD) pMaster->GetUID() );
				else
					sVal.FormatHex( (DWORD) 0 );
			}
			return( true );
			
		default:
			return( CItem::r_WriteVal( pszKey, sVal, pSrc ));
	}

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemStone::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItemStone::r_Verb");
	EXC_TRY("Verb");
	// NOTE:: ONLY CALL this from CChar::r_Verb !!!
	// Little to no security checking here !!!

	ASSERT(pSrc);

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		return( CItem::r_Verb( s, pSrc ));
	}

	CChar * pCharSrc = pSrc->GetChar();
#ifndef _NEWGUILDSYSTEM
	CClient * pClient = pCharSrc != NULL? pCharSrc->GetClient() : NULL;
#endif
	CStoneMember * pMember = GetMember(pCharSrc);

	switch ( index )
	{
#ifndef _NEWGUILDSYSTEM
		case ISV_ACCEPTCANDIDATE:
			addStoneDialog(pClient,STONEDISP_ACCEPTCANDIDATE);
			break;
#endif

#ifdef _NEWGUILDSYSTEM
		case ISV_ALLGUILDS:
			{
				if ( s.HasArgs() )
				{
					TCHAR * pszArgs = s.GetArgRaw();
					int iFlags = Exp_GetVal(pszArgs);
					SKIP_ARGSEP(pszArgs);

					if ( iFlags < 0 )
					{
						g_Log.EventError("ItemStone::AllGuilds invalid parameter '%i'.\n", iFlags);
						return false;
					}
					else
					{
						if ( pszArgs[0] != '\0' )
						{
							CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
							CScript scriptVerb( pszArgs );

							for (; pMember != NULL; pMember = pMember->GetNext())
							{
								if ( pMember->GetLinkUID().IsChar() )
									continue;

								if ( !iFlags )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
								else if ( ( iFlags == 1 ) && ( pMember->GetWeDeclared() && !pMember->GetTheyDeclared() ) )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
								else if ( ( iFlags == 2 ) && ( !pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
								else if ( ( iFlags == 3 ) && ( pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
							}
						}
						else
						{
							g_Log.EventError("ItemStone::AllGuilds empty args.\n");
							return false;
						}
					}
				}
			} break;

		case ISV_ALLMEMBERS:
			{
				if ( s.HasArgs() )
				{
					TCHAR * pszArgs = s.GetArgRaw();
					int iFlags = Exp_GetVal(pszArgs);
					SKIP_ARGSEP(pszArgs);

					if (( iFlags < -1 ) || ( iFlags > STONEPRIV_ACCEPTED ))
					{
						g_Log.EventError("ItemStone::AllMembers invalid parameter '%i'.\n", iFlags);
						return false;
					}
					else
					{
						if ( pszArgs[0] != '\0' )
						{
							CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
							CScript scriptVerb( pszArgs );

							for (; pMember != NULL; pMember = pMember->GetNext())
							{
								if ( !pMember->GetLinkUID().IsChar() )
									continue;

								if ( iFlags == -1 )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
								else if ( pMember->GetPriv() == static_cast<STONEPRIV_TYPE>(iFlags) )
								{
									pMember->r_Verb(scriptVerb, pSrc);
								}
							}
						}
						else
						{
							g_Log.EventError("ItemStone::AllMembers empty args.\n");
							return false;
						}
					}
				}
			} break;
#endif

		case ISV_APPLYTOJOIN:
			if ( s.HasArgs())
			{
				CGrayUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					AddRecruit( pMemberChar, STONEPRIV_CANDIDATE );
				}
			}
#ifndef _NEWGUILDSYSTEM
			else if ( pClient != NULL )
			{
				AddRecruit( pClient->GetChar(), STONEPRIV_CANDIDATE );
			}
#endif
			break;
		case ISV_CHANGEALIGN:
			if ( s.HasArgs())
			{
				SetAlignType(static_cast<STONEALIGN_TYPE>(s.GetArgVal()));
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "%s is now a %s %s\n", static_cast<LPCTSTR>(GetName()), static_cast<LPCTSTR>(GetAlignName()), static_cast<LPCTSTR>(GetTypeName()));
				Speak(pszMsg);
			}
#ifndef _NEWGUILDSYSTEM
			else if ( pClient != NULL )
			{
				pClient->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_GUILD_ALIGN"), this );
			}
#endif
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_DECLAREFEALTY:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_FEALTY);
			break;
#endif
		case ISV_DECLAREPEACE:
			if ( s.HasArgs())
			{
				CGrayUID pMemberUid = s.GetArgVal();
				WeDeclarePeace(pMemberUid);
			}
#ifndef _NEWGUILDSYSTEM
			else if ( pClient != NULL )
			{
				addStoneDialog(pClient,STONEDISP_DECLAREPEACE);
			}
#endif
			break;
		case ISV_DECLAREWAR:
			if ( s.HasArgs())
			{
				CGrayUID pMemberUid = s.GetArgVal();
				CItem * pEnemyItem = pMemberUid.ItemFind();
				if ( pEnemyItem )
				{
					CItemStone * pNewEnemy = dynamic_cast<CItemStone*>(pEnemyItem);
					if ( pNewEnemy )
					{
						WeDeclareWar(pNewEnemy);
					}
				}
			}
#ifndef _NEWGUILDSYSTEM
			else if ( pClient != NULL )
			{
				addStoneDialog(pClient,STONEDISP_DECLAREWAR);
			}
#endif
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_DISMISSMEMBER:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_DISMISSMEMBER);
			break;
#endif
		case ISV_ELECTMASTER:
			ElectMaster();
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_GRANTTITLE:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_GRANTTITLE);
			break;
#endif
		// Need to change FixWeirdness or rewrite how cstonemember guilds are handled.
		case ISV_INVITEWAR:
			{
				if ( s.HasArgs() )
				{
					INT64 piCmd[2];
					size_t iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
					if ( iArgQty == 2 )
					{
						CGrayUID pGuildUid = static_cast<unsigned long>(piCmd[0]);
						bool bWeDeclared = (piCmd[1] != 0);
						CItem * pEnemyItem = pGuildUid.ItemFind();
						if ( pEnemyItem && (pEnemyItem->IsType(IT_STONE_GUILD) || pEnemyItem->IsType(IT_STONE_TOWN)) )
						{
							CStoneMember * pMemberGuild = GetMember( pEnemyItem );
							if ( !pMemberGuild )
								pMemberGuild = new CStoneMember(this, pGuildUid, STONEPRIV_ENEMY);

							if ( bWeDeclared )
								pMemberGuild->SetWeDeclared(true);
							else
								pMemberGuild->SetTheyDeclared(true);
						}
					}
				}
			} break;
		case ISV_JOINASMEMBER:
			if ( s.HasArgs())
			{
				CGrayUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					AddRecruit( pMemberChar, STONEPRIV_MEMBER );
				}
			}
#ifndef _NEWGUILDSYSTEM
			else if ( pClient != NULL )
			{
				AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
			}
#endif
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_MASTERMENU:
			if ( pClient == NULL )
				return( false );
			SetupMenu( pClient, true );
			break;
		case ISV_RECRUIT:
			if ( pClient == NULL )
				return( false );
			if ( pClient->IsPriv(PRIV_GM ) || 
				( pMember != NULL && pMember->IsPrivMember()))
			{
				pClient->addTarget(
					s.GetArgVal() ? CLIMODE_TARG_STONE_RECRUITFULL : CLIMODE_TARG_STONE_RECRUIT,
					"Who do you want to recruit into the guild?"
					);
			}
			else
				Speak("Only members can recruit.");
			break;
		case ISV_REFUSECANDIDATE:
			addStoneDialog(pClient,STONEDISP_REFUSECANDIDATE);
			break;
#endif
		case ISV_RESIGN:
			if ( s.HasArgs())
			{
				CGrayUID pMemberUid = s.GetArgVal();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
					{
						delete pMemberGuild;
					}
				}
			}
			else
			{
				if ( pMember != NULL )
					delete pMember;
			}
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_RETURNMAINMENU:
			if ( pClient == NULL )
				return( false );
			SetupMenu( pClient );
			break;
		case ISV_SETABBREVIATION:
			if ( pClient == NULL )
				return( false );
			pClient->addPromptConsole( CLIMODE_PROMPT_STONE_SET_ABBREV, "What shall the abbreviation be?", GetUID() );
			break;
		case ISV_SETCHARTER:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_SETCHARTER);
			break;
		case ISV_SETGMTITLE:
			if ( pClient == NULL )
				return( false );
			pClient->addPromptConsole( CLIMODE_PROMPT_STONE_SET_TITLE, "What shall thy title be?", GetUID() );
			break;
		case ISV_SETNAME:
			{
				if ( pClient == NULL )
					return( false );
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "What would you like to rename the %s to?", static_cast<LPCTSTR>(GetTypeName()));
				pClient->addPromptConsole(CLIMODE_PROMPT_STONE_NAME, pszMsg, GetUID());
			}
			break;
#endif
		case ISV_TOGGLEABBREVIATION:
			{
				CGrayUID pMemberUid = ( s.HasArgs() ) ? s.GetArgVal() : pMember->GetLinkUID();
				CChar * pMemberChar = pMemberUid.CharFind();
				if ( pMemberChar )
				{
					CStoneMember * pMemberGuild = GetMember( pMemberChar );
					if ( pMemberGuild )
					{
						pMemberGuild->ToggleAbbrev();
						pMemberChar->ResendTooltip();
					}
				}
			}
			break;
#ifndef _NEWGUILDSYSTEM
		case ISV_VIEWCANDIDATES:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_CANDIDATES);
			break;
		case ISV_VIEWCHARTER:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_VIEWCHARTER);
			break;
		case ISV_VIEWENEMYS:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_VIEWENEMYS);
			break;
		case ISV_VIEWROSTER:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_ROSTER);
			break;
		case ISV_VIEWTHREATS:
			if ( pClient == NULL )
				return( false );
			addStoneDialog(pClient,STONEDISP_VIEWTHREATS);
			break;
#endif
		default:
			return( false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

CStoneMember * CItemStone::GetMasterMember() const
{
	ADDTOCALLSTACK("CItemStone::GetMasterMember");
	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
			return pMember;
	}
	return NULL;
}

bool CItemStone::IsPrivMember( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CItemStone::IsPrivMember");
	const CStoneMember * pMember = GetMember(pChar);
	if ( pMember == NULL )
		return( false );
	return( pMember->IsPrivMember());
}

CChar * CItemStone::GetMaster() const
{
	ADDTOCALLSTACK("CItemStone::GetMaster");
	CStoneMember * pMember = GetMasterMember();
	if ( pMember == NULL )
		return( NULL );
	return pMember->GetLinkUID().CharFind();
}

CStoneMember * CItemStone::GetMember( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CItemStone::GetMember");
	// Get member info for this char/item (if it has member info)
	if (!pObj)
		return NULL;
	CGrayUID otherUID = pObj->GetUID();
	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetLinkUID() == otherUID )
			return pMember;
	}
	return NULL;
}

bool CItemStone::NoMembers() const
{
	ADDTOCALLSTACK("CItemStone::NoMembers");
	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->IsPrivMember())
			return false;
	}
	return true;
}

CStoneMember * CItemStone::AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull)
{
	ADDTOCALLSTACK("CItemStone::AddRecruit");
	// CLIMODE_TARG_STONE_RECRUIT
	// Set as member or candidate.

	if ( !pChar || !pChar->m_pPlayer )
	{
		Speak( "Only players can be members!");
		return NULL;
	}
#ifndef _NEWGUILDSYSTEM
	else if ( !pChar->IsClient() )
	{
		Speak( "This player must be online to become a member." );
		return NULL;
	}
#endif

	TCHAR * z = Str_GetTemp();
	const CItemStone * pStone = pChar->Guild_Find( GetMemoryType());
	if ( pStone && pStone != this )
	{
		sprintf(z, "%s appears to belong to %s. Must resign previous %s", static_cast<LPCTSTR>(pChar->GetName()), static_cast<LPCTSTR>(pStone->GetName()), static_cast<LPCTSTR>(GetTypeName()));
		Speak(z);
		return NULL;
	}

	if ( IsType(IT_STONE_TOWN) && IsAttr(ATTR_OWNED) && iPriv == STONEPRIV_CANDIDATE )
	{
		// instant member.
		iPriv = STONEPRIV_MEMBER;
	}

	CStoneMember * pMember = GetMember(pChar);
	if ( pMember )
	{
		// I'm already a member of some sort.
		if ( pMember->GetPriv() == iPriv || iPriv == STONEPRIV_CANDIDATE )
		{
			sprintf(z, "%s is already %s %s.", static_cast<LPCTSTR>(pChar->GetName()), static_cast<LPCTSTR>(pMember->GetPrivName()), static_cast<LPCTSTR>(GetName()));
			Speak(z);
			return NULL;
		}

		pMember->SetPriv( iPriv );
	}
	else
	{
		pMember = new CStoneMember(this, pChar->GetUID(), iPriv);

		if ( bFull )	// full join means becoming a full member already
		{
			pMember->SetPriv(STONEPRIV_MEMBER);
			pMember->SetAbbrev(true);
		}
	}

	if ( pMember->IsPrivMember())
	{
		pMember->SetLoyalTo(pChar);
		ElectMaster();	// just in case this is the first.
	}

	sprintf(z, "%s is now %s %s", static_cast<LPCTSTR>(pChar->GetName()), static_cast<LPCTSTR>(pMember->GetPrivName()), static_cast<LPCTSTR>(GetName()));
	Speak(z);
	return pMember;
}

void CItemStone::ElectMaster()
{
	ADDTOCALLSTACK("CItemStone::ElectMaster");
	// Check who is loyal to who and find the new master.
	if ( GetAmount() == 0 )
		return;	// no reason to elect new if the stone is dead.

	int iResultCode = FixWeirdness();	// try to eliminate bad members.
	if ( iResultCode )
	{
		// The stone is bad ?
		// iResultCode
	}

	int iCountMembers = 0;
	CStoneMember * pMaster = NULL;

	// Validate the items and Clear the votes field
	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
		{
			pMaster = pMember;	// find current master.
		}
		else if ( pMember->GetPriv() != STONEPRIV_MEMBER )
		{
			continue;
		}
		pMember->m_Member.m_iVoteTally = 0;
		iCountMembers++;
	}

	// Now tally the votes.
	pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( ! pMember->IsPrivMember())
			continue;

		CChar * pCharVote = pMember->GetLoyalToUID().CharFind();
		if ( pCharVote != NULL )
		{
			CStoneMember * pMemberVote = GetMember( pCharVote );
			if ( pMemberVote != NULL )
			{
				pMemberVote->m_Member.m_iVoteTally ++;
				continue;
			}
		}

		// not valid to vote for. change to self.
		pMember->SetLoyalTo(NULL);
		// Assume I voted for myself.
		pMember->m_Member.m_iVoteTally ++;
	}

	// Find who won.
	bool fTie = false;
	CStoneMember * pMemberHighest = NULL;
	pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( ! pMember->IsPrivMember())
			continue;
		if ( pMemberHighest == NULL )
		{
			pMemberHighest = pMember;
			continue;
		}
		if ( pMember->m_Member.m_iVoteTally == pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = true;
		}
		if ( pMember->m_Member.m_iVoteTally > pMemberHighest->m_Member.m_iVoteTally )
		{
			fTie = false;
			pMemberHighest = pMember;
		}
	}

	// In the event of a tie, leave the current master as is
	if ( ! fTie && pMemberHighest )
	{
		if (pMaster)
			pMaster->SetPriv(STONEPRIV_MEMBER);
		pMemberHighest->SetPriv(STONEPRIV_MASTER);
	}

	if ( ! iCountMembers )
	{
		// No more members, declare peace (by force)
		pMember = STATIC_CAST <CStoneMember *>(GetHead());
		for (; pMember != NULL; pMember = pMember->GetNext())
		{
			WeDeclarePeace(pMember->GetLinkUID(), true);
		}
	}
}

bool CItemStone::IsUniqueName( LPCTSTR pName ) // static
{
	ADDTOCALLSTACK("CItemStone::IsUniqueName");
	for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
	{
		if ( ! strcmpi( pName, g_World.m_Stones[i]->GetName()))
			return false;
	}
	return true;
}

bool CItemStone::CheckValidMember( CStoneMember * pMember )
{
	ADDTOCALLSTACK("CItemStone::CheckValidMember");
	ASSERT(pMember);
	ASSERT( pMember->GetParent() == this );

	if ( GetAmount()==0 || g_Serv.m_iExitFlag )	// no reason to elect new if the stone is dead.
		return( true );	// we are deleting anyhow.

	switch ( pMember->GetPriv())
	{
		case STONEPRIV_MASTER:
		case STONEPRIV_MEMBER:
		case STONEPRIV_CANDIDATE:
		case STONEPRIV_ACCEPTED:
			if ( GetMemoryType())
			{
				// Make sure the member has a memory that links them back here.
				CChar * pChar = pMember->GetLinkUID().CharFind();
				if ( pChar == NULL )
					break;
				if ( pChar->Guild_Find( GetMemoryType()) != this )
					break;
			}
			return( true );
		case STONEPRIV_ENEMY:
			{
				CItemStone * pEnemyStone = dynamic_cast <CItemStone *>( pMember->GetLinkUID().ItemFind());
				if ( pEnemyStone == NULL )
					break;
				CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
// #ifndef _NEWGUILDSYSTEM
				if ( pEnemyMember == NULL )
					break;
				if ( pMember->GetWeDeclared() && ! pEnemyMember->GetTheyDeclared())
					break;
				if ( pMember->GetTheyDeclared() && ! pEnemyMember->GetWeDeclared())
					break;
/*#else
				if ( pEnemyMember == NULL )
				{
					if ( !pMember->GetWeDeclared() ) // this memory can only exist if we declared
						break;

					if ( pMember->GetTheyDeclared() ) // this memory cannot exist if they declared but we don't have
						break;						  // any memory on them there is an error

					if ( pMember->GetWeDeclared() && pMember->GetTheyDeclared() ) // this has no sense at all
						break;
				}
				else
				{
					if ( pMember->GetWeDeclared() && pEnemyMember->GetWeDeclared() ) // WTF!!!
					{
						// Since we don't know who really declared, set it to false and let sphere take care.
						pEnemyMember->SetWeDeclared(false);
						break;
					}

					if ( !pMember->GetWeDeclared() && pEnemyMember->GetTheyDeclared() ) // We haven't declared
						break;

					if ( pMember->GetTheyDeclared() && pEnemyMember->GetTheyDeclared() ) // WTF!!!
						break;

					if ( pMember->GetTheyDeclared() && !pEnemyMember->GetWeDeclared() ) // If unilateral they should have the memory
						break;

					if ( !pMember->GetTheyDeclared() && !pEnemyMember->GetWeDeclared() ) // If unilateral they should have the memory
						break;
				}
#endif */
			}
			return( true );

		default:
			break;
	}

	// just delete this member. (it is mislinked)
	DEBUG_ERR(( "Stone UID=0%lx has mislinked member uid=0%lx\n", 
		(DWORD) GetUID(), (DWORD) pMember->GetLinkUID()));
	return( false );
}

int CItemStone::FixWeirdness()
{
	ADDTOCALLSTACK("CItemStone::FixWeirdness");
	// Check all my members. Make sure all wars are reciprocated and members are flaged.

	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
	{
		return( iResultCode );
	}

	bool fChanges = false;
	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	while ( pMember != NULL )
	{
		CStoneMember * pMemberNext = pMember->GetNext();
		if ( ! CheckValidMember(pMember))
		{
			IT_TYPE oldtype = GetType();
			SetAmount(0);	// turn off validation for now. we don't want to delete other members.
			delete pMember;
			SetAmount(1);	// turn off validation for now. we don't want to delete other members.
			SetType( oldtype );
			fChanges = true;
		}
		pMember = pMemberNext;
	}

	if ( fChanges )
	{
		ElectMaster();	// May have changed the vote count.
	}
	return( 0 );
}

bool CItemStone::IsAlliedWith( const CItemStone * pStone) const
{
	ADDTOCALLSTACK("CItemStone::IsAlliedWith");
	if ( pStone == NULL )
		return( false );

#ifdef _NEWGUILDSYSTEM

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isalliedwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
		{
			return false;
		}
		else if ( tr == TRIGRET_RET_TRUE )
		{
			return true;
		}
	}
#endif

	return( GetAlignType() != STONEALIGN_STANDARD &&
		GetAlignType() == pStone->GetAlignType());
}

bool CItemStone::IsAtWarWith( const CItemStone * pEnemyStone ) const
{
	ADDTOCALLSTACK("CItemStone::IsAtWarWith");
	// Boths guild shave declared war on each other.

	if ( pEnemyStone == NULL )
		return( false );

#ifdef _NEWGUILDSYSTEM

	CScriptTriggerArgs Args;
	Args.m_pO1 = const_cast<CItemStone *>(pEnemyStone);
	enum TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

	if ( const_cast<CItemStone *>(this)->r_Call("f_stonesys_internal_isatwarwith", &g_Serv, &Args, NULL, &tr) )
	{
		if ( tr == TRIGRET_RET_FALSE )
		{
			return false;
		}
		else if ( tr == TRIGRET_RET_TRUE )
		{
			return true;
		}
	}

#endif


	// Just based on align type.
	if ( IsType(IT_STONE_GUILD) &&
		GetAlignType() != STONEALIGN_STANDARD &&
		pEnemyStone->GetAlignType() != STONEALIGN_STANDARD )
	{
		return( GetAlignType() != pEnemyStone->GetAlignType());
	}

	// we have declared or they declared.
	CStoneMember * pEnemyMember = GetMember(pEnemyStone);
	if (pEnemyMember) // Ok, we might be at war
	{
		if ( pEnemyMember->GetTheyDeclared() && pEnemyMember->GetWeDeclared())
			return true;
	}

	return false;

}

void CItemStone::AnnounceWar( const CItemStone * pEnemyStone, bool fWeDeclare, bool fWar )
{
	ADDTOCALLSTACK("CItemStone::AnnounceWar");
	// Announce we are at war or peace.

	ASSERT(pEnemyStone);

	bool fAtWar = IsAtWarWith(pEnemyStone);

	TCHAR *pszTemp = Str_GetTemp();
	int len = sprintf( pszTemp, (fWar) ? "%s %s declared war on %s." : "%s %s requested peace with %s.",
		(fWeDeclare) ? "You" : pEnemyStone->GetName(),
		(fWeDeclare) ? "have" : "has",
		(fWeDeclare) ? pEnemyStone->GetName() : "You" );

	if ( fAtWar )
	{
		sprintf( pszTemp+len, " War is ON!" );
	}
	else if ( fWar )
	{
		sprintf( pszTemp+len, " War is NOT yet on." );
	}
	else
	{
		sprintf( pszTemp+len, " War is OFF." );
	}

	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		CChar * pChar = pMember->GetLinkUID().CharFind();
		if ( pChar == NULL )
			continue;
		if ( ! pChar->IsClient())
			continue;
		pChar->SysMessage( pszTemp );
	}
}

bool CItemStone::WeDeclareWar(CItemStone * pEnemyStone)
{
	ADDTOCALLSTACK("CItemStone::WeDeclareWar");
	if (!pEnemyStone)
		return false;

#ifndef _NEWGUILDSYSTEM
	// Make sure they have actual members first
	if ( pEnemyStone->NoMembers())
	{
		//Speak( "Enemy guild has no members!" );
		return false;
	}
	// Order cannot declare on Order.
	if ( GetAlignType() == STONEALIGN_ORDER &&
		pEnemyStone->GetAlignType() == STONEALIGN_ORDER )
	{
		//Speak( "Order cannot declare on Order!" );
		return( false );
	}
#endif

	// See if they've already declared war on us
	CStoneMember * pMember = GetMember(pEnemyStone);
	if ( pMember )
	{
		if ( pMember->GetWeDeclared())
			return true;
	}
	else // They haven't, make a record of this
	{
		pMember = new CStoneMember( this, pEnemyStone->GetUID(), STONEPRIV_ENEMY );
	}
	pMember->SetWeDeclared(true);

	// Now inform the other stone
	// See if they have already declared war on us
	CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
	if (!pEnemyMember) // Not yet it seems
		pEnemyMember = new CStoneMember( pEnemyStone, GetUID(), STONEPRIV_ENEMY );

	pEnemyMember->SetTheyDeclared(true);

#ifndef _NEWGUILDSYSTEM
	// announce to both sides.
	AnnounceWar( pEnemyStone, true, true );
	pEnemyStone->AnnounceWar( this, false, true );
#endif
	return( true );
}

void CItemStone::TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace )
{
	ADDTOCALLSTACK("CItemStone::TheyDeclarePeace");
	// Now inform the other stone
	// Make sure we declared war on them
	CStoneMember * pEnemyMember = GetMember(pEnemyStone);
	if ( ! pEnemyMember )
		return;

	bool fPrevTheyDeclared = pEnemyMember->GetTheyDeclared();

	if (!pEnemyMember->GetWeDeclared() || fForcePeace) // If we're not at war with them, delete this record
		delete pEnemyMember;
	else
		pEnemyMember->SetTheyDeclared(false);

	if ( ! fPrevTheyDeclared )
		return;

#ifndef _NEWGUILDSYSTEM
	// announce to both sides.
	pEnemyStone->AnnounceWar( this, true, false );
	AnnounceWar( pEnemyStone, false, false );
#endif
}

void CItemStone::WeDeclarePeace(CGrayUID uid, bool fForcePeace)
{
	ADDTOCALLSTACK("CItemStone::WeDeclarePeace");
	CItemStone * pEnemyStone = dynamic_cast <CItemStone*>( uid.ItemFind());
	if (!pEnemyStone)
		return;

	CStoneMember * pMember = GetMember(pEnemyStone);
	if ( ! pMember ) // No such member
		return;

	// Set my flags on the subject.
	if (!pMember->GetTheyDeclared() || fForcePeace) // If they're not at war with us, delete this record
		delete pMember;
	else
		pMember->SetWeDeclared(false);

	pEnemyStone->TheyDeclarePeace( this, fForcePeace );
}

void CItemStone::SetTownName()
{
	ADDTOCALLSTACK("CItemStone::SetTownName");
	// For town stones.
	if ( ! IsTopLevel())
		return;
	CRegionBase * pArea = GetTopPoint().GetRegion(( IsType(IT_STONE_TOWN)) ? REGION_TYPE_AREA : REGION_TYPE_ROOM );
	if ( pArea )
	{
		pArea->SetName( GetIndividualName());
	}
}

bool CItemStone::MoveTo( CPointMap pt, bool bForceFix )
{
	ADDTOCALLSTACK("CItemStone::MoveTo");
	// Put item on the ground here.
	if ( IsType(IT_STONE_TOWN) )
	{
		SetTownName();
	}
	return CItem::MoveTo(pt);
}

bool CItemStone::SetName( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CItemStone::SetName");
	// If this is a town then set the whole regions name.
	if ( !CItem::SetName(pszName) )
		return false;
	if ( IsTopLevel() && IsType(IT_STONE_TOWN) )
		SetTownName();

	return true;
}

#ifndef _NEWGUILDSYSTEM
void CItemStone::SetupMenu( CClient * pClient, bool fMasterFunc )
{
	ADDTOCALLSTACK("CItemStone::SetupMenu");
	if ( pClient == NULL )
		return;

	CStoneMember * pMember = GetMember(pClient->GetChar());
	bool fMaster = ( pClient->IsPriv(PRIV_GM) || ( pMember && pMember->IsPrivMaster() ));

	if ( pMember && pMember->GetPriv() == STONEPRIV_ACCEPTED )
	{
		// Am i an STONEPRIV_ACCEPTED member ? make me a full member.
		AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
	}

	LPCTSTR pszResourceName;
	if ( IsType(IT_STONE_TOWN))
	{
		if ( fMaster )
		{
			// Non GM's shouldn't get here, but in case they do (exploit), give them the non GM menu
			if ( fMasterFunc )
				pszResourceName = "MENU_TOWN_MAYORFUNC";
			else
				pszResourceName = "MENU_TOWN_MAYOR";
		}
		else if ( ! pMember )		// non-member view.
		{
			pszResourceName = "MENU_TOWN_NON";
		}
		else
		{
			pszResourceName = "MENU_TOWN_MEMBER";
		}
	}
	else
	{
		if ( fMaster )
		{
			if ( fMasterFunc )
				pszResourceName = "MENU_GUILD_MASTERFUNC";
			else
				pszResourceName = "MENU_GUILD_MASTER";
		}
		else if ( ! pMember )		// non-member view.
		{
			pszResourceName = "MENU_GUILD_NON";
		}
		else
		{
			pszResourceName = "MENU_GUILD_MEMBER";
		}
	}

	pClient->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, pszResourceName ), this );
}

void CItemStone::addStoneSetViewCharter( CClient * pClient, STONEDISP_TYPE iStoneMenu )
{
	ADDTOCALLSTACK("CItemStone::addStoneSetViewCharter");

	static LPCTSTR const sm_szDefControls[] =
	{
		"page 0",							// Default page
		"resizepic 0 0 2520 350 400",	// Background pic
		"tilepic 30 50 %d",			// Picture of a stone
		"tilepic 275 50 %d",			// Picture of a stone
		"gumppic 76 126 95",				// Decorative separator
		"gumppic 85 135 96",				// Decorative separator
		"gumppic 264 126 97",			// Decorative separator
		"text 65 35 2301 0",				// Stone name at top
		"text 110 70 0 1",				// Page description
		"text 120 85 0 2",				// Page description
		"text 140 115 0 3",				// Section description
		"text 125 290 0 10",				// Section description
		"gumppic 76 301 95",				// Decorative separator
		"gumppic 85 310 96",				// Decorative separator
		"gumppic 264 301 97",			// Decorative separator
		"text 40 370 0 12",				// Directions
		"button 195 370 2130 2129 1 0 %i",	// Save button
		"button 255 370 2121 2120 1 0 0"		// Cancel button
	};

	CGString sControls[COUNTOF(sm_szDefControls)+10+COUNTOF(m_sCharter)];
	size_t iControls = 0;
	while ( iControls < COUNTOF(sm_szDefControls))
	{
		// Fix the button ID so we can trap it later
		sControls[iControls].Format( sm_szDefControls[iControls], ( iControls > 4 ) ? (int) iStoneMenu : (int) GetDispID());
		iControls ++;
	}

	bool fView = (iStoneMenu == STONEDISP_VIEWCHARTER);

	CGString sText[10+COUNTOF(m_sCharter)];
	size_t iTexts = 0;
	sText[iTexts++] = GetName();
	sText[iTexts++].Format( "%s %s Charter", (fView) ? "View": "Set", (LPCTSTR) GetTypeName());
	sText[iTexts++] = "and Web Link";
	sText[iTexts++] = "Charter";

	for ( unsigned int iLoop = 0; iLoop < COUNTOF(m_sCharter); iLoop++)
	{
		if ( fView )
		{
			sControls[iControls++].Format( "text 50 %u 0 %u", 155 + (iLoop*22), iLoop + 4);
		}
		else
		{
			sControls[iControls++].Format( "gumppic 40 %u 1143", 152 + (iLoop*22));
			sControls[iControls++].Format( "textentry 50 %u 250 32 0 %u %u", 155 + (iLoop*22), iLoop + 1000, iLoop + 4 );
		}
		if ( fView && iLoop == 0 && m_sCharter[0].IsEmpty())
		{
			sText[iTexts++] = "No charter has been specified.";
		}
		else
		{
			sText[iTexts++] = GetCharter(iLoop);
		}
	}

	if ( fView )
	{
		sControls[iControls++] = "text 50 331 0 11";
	}
	else
	{
		sControls[iControls++] = "gumppic 40 328 1143";
		sControls[iControls++] = "textentry 50 331 250 32 0 1006 11";
	}

	sText[iTexts++] = "Web Link";
	sText[iTexts++] = GetWebPageURL();
	sText[iTexts++] = (fView) ? "Go to the web page": "Save this information";

	ASSERT( iTexts <= COUNTOF(sText));
	ASSERT( iControls <= COUNTOF(sControls));

	pClient->addGumpDialog( CLIMODE_DIALOG_GUILD, sControls, iControls, sText, iTexts, 0x6e, 0x46 );
}

bool CItemStone::IsInMenu( STONEDISP_TYPE iStoneMenu, const CStoneMember * pMember ) const
{
	ADDTOCALLSTACK("CItemStone::IsInMenu");
	ASSERT( pMember );

	switch ( iStoneMenu )
	{
		case STONEDISP_ROSTER:
		case STONEDISP_FEALTY:
		case STONEDISP_GRANTTITLE:
			if ( ! pMember->IsPrivMember())
				return( false );
			break;
		case STONEDISP_DISMISSMEMBER:
			if ( ! pMember->IsPrivMember() && pMember->GetPriv() != STONEPRIV_ACCEPTED )
				return( false );
			break;
		case STONEDISP_ACCEPTCANDIDATE:
		case STONEDISP_REFUSECANDIDATE:
		case STONEDISP_CANDIDATES:
			if ( pMember->GetPriv() != STONEPRIV_CANDIDATE )
				return( false );
			break;
		case STONEDISP_DECLAREPEACE:
		case STONEDISP_VIEWENEMYS:
			if ( pMember->GetPriv() != STONEPRIV_ENEMY)
				return( false );
			if ( !pMember->GetWeDeclared())
				return( false );
			break;
		case STONEDISP_VIEWTHREATS:
			if ( pMember->GetPriv() != STONEPRIV_ENEMY)
				return( false );
			if ( !pMember->GetTheyDeclared())
				return( false );
			break;
		default:
			return( false );
	}

	return( true );
}

bool CItemStone::IsInMenu( STONEDISP_TYPE iStoneMenu, const CItemStone * pOtherStone ) const
{
	ADDTOCALLSTACK("CItemStone::IsInMenu");
	if ( iStoneMenu != STONEDISP_DECLAREWAR )
		return( false );

	if ( pOtherStone == this )
		return( false );

	CStoneMember * pMember = GetMember( pOtherStone );
	if (pMember)
	{
		if ( pMember->GetWeDeclared())	// already declared.
			return( false );
	}
	else
	{
		if ( pOtherStone->GetCount() <= 0 )	// Only stones with members can have war declared against them
			return( false );
	}

	return( true );
}

size_t CItemStone::addStoneListSetup( STONEDISP_TYPE iStoneMenu, CGString * psText, size_t iTexts )
{
	ADDTOCALLSTACK("CItemStone::addStoneListSetup");
	// ARGS: psText = NULL if i just want to count.

	if ( iStoneMenu == STONEDISP_DECLAREWAR )
	{
		// This list is special.
		for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
		{
			CItemStone * pOtherStone = g_World.m_Stones[i];
			if ( ! IsInMenu( STONEDISP_DECLAREWAR, pOtherStone ))
				continue;
			if ( psText )
			{
				psText[iTexts] = pOtherStone->GetName();
			}
			iTexts ++;
		}
		return( iTexts );
	}

	CStoneMember * pMember = STATIC_CAST <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( ! IsInMenu( iStoneMenu, pMember ))
			continue;

		if ( psText )
		{
			CChar * pChar = pMember->GetLinkUID().CharFind();
			if ( pChar )
			{
				TCHAR szTmp[256];
				strcpy( szTmp, pChar->GetName());
				if (strlen( pMember->GetTitle()) > 0)
				{
					strcat( szTmp, ", ");
					strcat( szTmp, pMember->GetTitle());
				}
				psText[iTexts] = szTmp;
			}
			else
			{
				CItem * pItem = pMember->GetLinkUID().ItemFind();
				if (pItem)
				{
					CItemStone * pStoneItem = dynamic_cast <CItemStone*> ( pItem );
					if (pStoneItem)
						psText[iTexts] = pStoneItem->GetName();
					else
						psText[iTexts] = "Bad stone";
				}
				else
				{
					psText[iTexts] = "Bad member";
				}
			}
		}
		iTexts ++;
	}

	return( iTexts );
}

void CItemStone::addStoneList( CClient * pClient, STONEDISP_TYPE iStoneMenu )
{
	ADDTOCALLSTACK("CItemStone::addStoneList");

	// Add a list of members of a type.
	// Estimate the size first.
	static LPCTSTR const sm_szDefControls[] =
	{
		// "nomove",
		"page 0",
		"resizepic 0 0 5100 400 350",
		"text 15 10 0 0",
		"button 13 290 5050 5051 1 0 %i",
		"text 45 292 0 3",
		"button 307 290 5200 5201 1 0 0"
	};

	CGString sControls[512];
	size_t iControls = 0;

	size_t iControlLimit = COUNTOF(sm_szDefControls);
	switch ( iStoneMenu )
	{
		case STONEDISP_FEALTY:
		case STONEDISP_ACCEPTCANDIDATE:
		case STONEDISP_REFUSECANDIDATE:
		case STONEDISP_DISMISSMEMBER:
		case STONEDISP_DECLAREWAR:
		case STONEDISP_DECLAREPEACE:
		case STONEDISP_GRANTTITLE:
			break;
		case STONEDISP_ROSTER:
		case STONEDISP_CANDIDATES:
		case STONEDISP_VIEWENEMYS:
		case STONEDISP_VIEWTHREATS:
			iControlLimit --;
			break;
		default:
			break;
	}

	while ( iControls < iControlLimit )
	{
		// Fix the button's number so we know what screen this is later
		sControls[iControls].Format( sm_szDefControls[iControls], iStoneMenu );
		iControls++;
	}

	static LPCTSTR const sm_szDefText[] =
	{
		"%s %s",
		"Previous page",
		"Next page",
		"%s"
	};

	CGString sText[512];
	size_t iTexts = 1;
	for ( ; iTexts < COUNTOF(sm_szDefText) - 1; iTexts++ )
	{
		ASSERT( iTexts < COUNTOF(sText));
		sText[iTexts] = sm_szDefText[iTexts];
	}

	switch ( iStoneMenu )
	{
		case STONEDISP_ROSTER:
			sText[0].Format(sm_szDefText[0], (LPCTSTR) GetName(), "Roster");
			break;
		case STONEDISP_CANDIDATES:
			sText[0].Format(sm_szDefText[0], (LPCTSTR) GetName(), "Candidates");
			break;
		case STONEDISP_FEALTY:
			sText[0].Format(sm_szDefText[0], "Declare your fealty", "" );
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
				"I have selected my new lord");
			break;
		case STONEDISP_ACCEPTCANDIDATE:
			sText[0].Format(sm_szDefText[0], "Accept candidate for", (LPCTSTR) GetName());
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
				"Accept this candidate for membership");
			break;
		case STONEDISP_REFUSECANDIDATE:
			sText[0].Format(sm_szDefText[0], "Refuse candidate for", (LPCTSTR) GetName());
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
				"Refuse this candidate membership");
			break;
		case STONEDISP_DISMISSMEMBER:
			sText[0].Format(sm_szDefText[0], "Dismiss member from", (LPCTSTR) GetName());
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
				"Dismiss this member");
			break;
		case STONEDISP_DECLAREWAR:
			sText[0].Format(sm_szDefText[0], "Declaration of war by", (LPCTSTR) GetName());
			sText[COUNTOF(sm_szDefText)-1].Format(	"Declare war on this %s", (LPCTSTR) GetTypeName());
			break;
		case STONEDISP_DECLAREPEACE:
			sText[0].Format(sm_szDefText[0], "Declaration of peace by", (LPCTSTR) GetName());
			sText[COUNTOF(sm_szDefText)-1].Format( "Declare peace with this %s", (LPCTSTR) GetTypeName());
			break;
		case STONEDISP_GRANTTITLE:
			sText[0] = "To whom do you wish to grant a title?";
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
				"Grant this member a title");
			break;
		case STONEDISP_VIEWENEMYS:
			sText[0].Format( "%ss we have declared war on", (LPCTSTR) GetTypeName());
			break;
		case STONEDISP_VIEWTHREATS:
			sText[0].Format( "%ss which have declared war on us", (LPCTSTR) GetTypeName());
			break;
		default:
			break;
	}

	switch ( iStoneMenu )
	{
		case STONEDISP_ROSTER:
		case STONEDISP_CANDIDATES:
		case STONEDISP_VIEWTHREATS:
		case STONEDISP_VIEWENEMYS:
			sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1], "Done");
			break;
		default:
			break;
	}
	iTexts++;

	// First count the appropriate members
	size_t iMemberCount = addStoneListSetup( iStoneMenu, NULL, 0 );

	if ( (iMemberCount + iTexts) > COUNTOF(sText))
	{
		DEBUG_ERR(( "Too Many Guilds !!!\n" ));
		iMemberCount = COUNTOF(sText) - iTexts - 1;
	}

	size_t iPages = 0;
	for ( size_t iLoop = 0; iLoop < iMemberCount; iLoop++)
	{
		if (iLoop % 10 == 0)
		{
			iPages += 1;
			sControls[iControls++].Format("page %i", iPages);
			if (iPages > 1)
			{
				sControls[iControls++].Format("button 15 320 5223 5223 0 %" FMTSIZE_T, iPages - 1);
				sControls[iControls++] = "text 40 317 0 1";
			}
			if ( iMemberCount > (iPages * 10))
			{
				sControls[iControls++].Format("button 366 320 5224 5224 0 %" FMTSIZE_T, iPages + 1);
				sControls[iControls++] = "text 288 317 0 2";
			}
		}
		switch ( iStoneMenu )
		{
			case STONEDISP_FEALTY:
			case STONEDISP_DISMISSMEMBER:
			case STONEDISP_ACCEPTCANDIDATE:
			case STONEDISP_REFUSECANDIDATE:
			case STONEDISP_DECLAREWAR:
			case STONEDISP_DECLAREPEACE:
			case STONEDISP_GRANTTITLE:
				{
					sControls[iControls++].Format("radio 20 %i 5002 5003 0 %i", ((iLoop % 10) * 25) + 35, iLoop + 1000);
				}
			case STONEDISP_ROSTER:
			case STONEDISP_CANDIDATES:
			case STONEDISP_VIEWENEMYS:
			case STONEDISP_VIEWTHREATS:
				{
					sControls[iControls++].Format("text 55 %i 0 %i", ((iLoop % 10) * 25) + 32, iLoop + 4);
				}
				break;
			default:
				break;
		}
		ASSERT( iControls < COUNTOF(sControls));
	}

	iTexts = addStoneListSetup( iStoneMenu, sText, iTexts );

	pClient->addGumpDialog( CLIMODE_DIALOG_GUILD, sControls, iControls, sText, iTexts, 0x6e, 0x46 );
}

void CItemStone::addStoneDialog( CClient * pClient, STONEDISP_TYPE menuid )
{
	ADDTOCALLSTACK("CItemStone::addStoneDialog");
	ASSERT( pClient );

	// Use this for a stone dispatch routine....
	switch (menuid)
	{
		case STONEDISP_ROSTER:
		case STONEDISP_CANDIDATES:
		case STONEDISP_FEALTY:
		case STONEDISP_ACCEPTCANDIDATE:
		case STONEDISP_REFUSECANDIDATE:
		case STONEDISP_DISMISSMEMBER:
		case STONEDISP_DECLAREWAR:
		case STONEDISP_DECLAREPEACE:
		case STONEDISP_VIEWENEMYS:
		case STONEDISP_VIEWTHREATS:
		case STONEDISP_GRANTTITLE:
			addStoneList(pClient,menuid);
			break;
		case STONEDISP_VIEWCHARTER:
		case STONEDISP_SETCHARTER:
			addStoneSetViewCharter(pClient,menuid);
			break;
		default:
			break;
	}
}

bool CItemStone::OnDialogButton( CClient * pClient, STONEDISP_TYPE type, CDialogResponseArgs & resp )
{
	ADDTOCALLSTACK("CItemStone::OnDialogButton");
	// Button presses come here
	ASSERT( pClient );

	switch ( type )
	{
		case STONEDISP_NONE: // They right clicked out, or hit the cancel button, no more stone functions
			return true;

		case STONEDISP_VIEWCHARTER:
			// The only button is the web button, so just go there
			pClient->addWebLaunch( GetWebPageURL());
			return true;

		case STONEDISP_SETCHARTER:
			{
				for (size_t i = 0; i < resp.m_TextArray.GetCount(); i++)
				{
					unsigned int id = resp.m_TextArray[i]->m_ID - 1000;
					switch ( id )
					{
						case 0:	// Charter[0]
						case 1:	// Charter[1]
						case 2:	// Charter[2]
						case 3:	// Charter[3]
						case 4:	// Charter[4]
						case 5:	// Charter[5]
							SetCharter(id, resp.m_TextArray[i]->m_sText);
							break;
						case 6:	// Weblink
							SetWebPage( resp.m_TextArray[i]->m_sText );
							break;
					}
				}
			}
			return true;

		case STONEDISP_DISMISSMEMBER:
		case STONEDISP_ACCEPTCANDIDATE:
		case STONEDISP_REFUSECANDIDATE:
		case STONEDISP_FEALTY:
		case STONEDISP_DECLAREWAR:
		case STONEDISP_DECLAREPEACE:
		case STONEDISP_GRANTTITLE:
			break;

		case STONEDISP_ROSTER:
		case STONEDISP_VIEWTHREATS:
		case STONEDISP_VIEWENEMYS:
		case STONEDISP_CANDIDATES:
			SetupMenu( pClient );
			return( true );

		default:
			return( false );
	}

	if ( resp.m_CheckArray.GetCount() == 0 )	 // If they hit ok but didn't pick one, treat it like a cancel
		return true;

	int iMember = resp.m_CheckArray[0] - 1000;

	CStoneMember * pMember = NULL;
	bool fFound = false;
	int iLoop = 0;
	size_t iStoneIndex = 0;

	if ( type == STONEDISP_DECLAREWAR )
	{
		for ( ; iStoneIndex < g_World.m_Stones.GetCount(); iStoneIndex ++ )
		{
			CItemStone * pOtherStone = g_World.m_Stones[iStoneIndex];
			if ( ! IsInMenu( STONEDISP_DECLAREWAR, pOtherStone ))
				continue;
			if (iLoop == iMember)
			{
				fFound = true;
				break;
			}
			iLoop ++;
		}
	}
	else
	{
		pMember = STATIC_CAST <CStoneMember *>(GetHead());
		for (; pMember != NULL; pMember = pMember->GetNext())
		{
			if ( ! IsInMenu( type, pMember ))
				continue;
			if (iLoop == iMember)
			{
				fFound = true;
				break;
			}
			iLoop ++;
		}
	}

	if (fFound)
	{
		switch ( type ) // Button presses come here
		{
			case STONEDISP_DECLAREWAR:
				if ( ! WeDeclareWar(g_World.m_Stones[iStoneIndex]))
				{
					pClient->SysMessage( "Cannot declare war" );
				}
				break;
			case STONEDISP_ACCEPTCANDIDATE:
				ASSERT( pMember );
				AddRecruit( pMember->GetLinkUID().CharFind(), STONEPRIV_ACCEPTED );
				break;
			case STONEDISP_REFUSECANDIDATE:
				ASSERT( pMember );
				delete pMember;
				break;
			case STONEDISP_DISMISSMEMBER:
				ASSERT( pMember );
				delete pMember;
				break;
			case STONEDISP_FEALTY:
				ASSERT( pMember );
				{
					CStoneMember * pMe = GetMember(pClient->GetChar());
					if ( pMe == NULL ) return( false );
					pMe->SetLoyalTo( pMember->GetLinkUID().CharFind());
				}
				break;
			case STONEDISP_DECLAREPEACE:
				ASSERT( pMember );
				WeDeclarePeace(pMember->GetLinkUID());
				break;
			case STONEDISP_GRANTTITLE:
				ASSERT( pMember );
				pClient->addPromptConsole( CLIMODE_PROMPT_STONE_GRANT_TITLE, "What title dost thou grant?", GetUID(), pMember->GetLinkUID() );
				return( true );
			default:
				break;
		}
	}
	else
	{
		pClient->SysMessage("Who is that?");
	}

	// Now send them back to the screen they came from

	switch ( type )
	{
		case STONEDISP_ACCEPTCANDIDATE:
		case STONEDISP_REFUSECANDIDATE:
		case STONEDISP_DISMISSMEMBER:
		case STONEDISP_DECLAREPEACE:
		case STONEDISP_DECLAREWAR:
			SetupMenu( pClient, true );
			break;
		default:
			SetupMenu( pClient, false );
			break;
	}

	return true;
}

bool CItemStone::OnPromptResp( CClient * pClient, CLIMODE_TYPE TargMode, LPCTSTR pszText, CGString & sMsg, CGrayUID context )
{
	ADDTOCALLSTACK("CItemStone::OnPromptResp");
	ASSERT( pClient );

	switch ( TargMode )
	{
		case CLIMODE_PROMPT_STONE_NAME:
			// Set the stone or town name !
			if ( ! CItemStone::IsUniqueName( pszText ))
			{
				if (!strcmpi( pszText, GetName()))
				{
					pClient->SysMessage( "Name is unchanged." );
					return false;
				}
				pClient->SysMessage( "That name is already taken." );
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "What would you like to rename the %s to?", (LPCTSTR) GetTypeName());
				pClient->addPromptConsole(CLIMODE_PROMPT_STONE_NAME, pszMsg, GetUID());
				return false;
			}

			SetName( pszText );
			if ( NoMembers()) // No members? It must be a brand new stone then, fix it up
			{
				AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
			}
			sMsg.Format( "%s renamed: %s", (LPCTSTR) GetTypeName(), (LPCTSTR) pszText );
			break;

		case CLIMODE_PROMPT_STONE_SET_ABBREV:
			SetAbbrev(pszText);
			sMsg.Format( "Abbreviation set: %s", pszText );
			break;

		case CLIMODE_PROMPT_STONE_GRANT_TITLE:
			{
				CStoneMember * pMember = GetMember( context.CharFind());
				if (pMember)
				{
					pMember->SetTitle(pszText);
					sMsg.Format( "Title set: %s", pszText);
				}
			}
			break;
		case CLIMODE_PROMPT_STONE_SET_TITLE:
			{
				CStoneMember * pMaster = GetMasterMember();
				pMaster->SetTitle(pszText);
				sMsg.Format( "Title set: %s", pszText);
			}
			break;

		default:
			break;
	}

	return( true );
}

void CItemStone::Use_Item( CClient * pClient )
{
	ADDTOCALLSTACK("CItemStone::Use_Item");
	if ( NoMembers() && IsType(IT_STONE_GUILD)) // Everyone resigned...new master
	{
		AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
	}
	SetupMenu( pClient );
}

#endif

