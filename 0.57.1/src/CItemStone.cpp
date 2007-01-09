#include "graysvr.h"

//////////
// -CStoneMember

CStoneMember::CStoneMember( CItemStone * pStone, UID uid, STONEPRIV_TYPE iType, LPCTSTR pTitle, UID loyaluid, bool fVal1, bool fVal2, int nAccountGold)
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
	pStone->ElectMaster();	// May have changed the vote count.

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
		CChar * pChar = GetLinkUID().CharFind();
		if ( pChar )
		{
			pChar->Memory_ClearTypes( pStone->GetMemoryType()); 	// Make them forget they were ever in this guild
		}
	}
}

CItemStone * CStoneMember::GetParentStone()
{
	return dynamic_cast <CItemStone *>( GetParent());
}

LPCTSTR CStoneMember::GetPrivName() const
{
	switch ( GetPriv())
	{
	case STONEPRIV_CANDIDATE: return "a candidate of";
	case STONEPRIV_MEMBER: return "a member of";
	case STONEPRIV_MASTER: return "the master of";
	case STONEPRIV_ACCEPTED: return "accepted in";
	case STONEPRIV_ENEMY: return "the enemy of";
	}
	return( "unknown" );
}

bool CStoneMember::SetLoyalTo( const CChar * pCharLoyal )
{
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
	return true;
}

//////////
// -CItemStone

CItemStone::CItemStone( ITEMID_TYPE id, CItemBase * pItemDef ) : CItem( id, pItemDef )
{
	m_itStone.m_iAlign = STONEALIGN_STANDARD;
	g_World.m_Stones.Add( this );
	m_pts = 0;
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

LPCTSTR CItemStone::GetTypeName() const
{
	switch ( GetType() )
	{
		case IT_STONE_GUILD:
			return "Guild";
		case IT_STONE_TOWN:
			return "Town";
	}
	return "Unk";
}

void CItemStone::r_Write( CScript & s )
{
	CItem::r_Write( s );
	s.WriteKeyVal( "ALIGN", GetAlignType());
	if ( ! m_sAbbrev.IsEmpty())
	{
		s.WriteKey( "ABBREV", m_sAbbrev );
	}
	TEMPSTRING(pszTemp);
	for ( int i = 0; i<COUNTOF(m_sCharter); i++ )
	{
		if ( ! m_sCharter[i].IsEmpty())
		{
			sprintf(pszTemp, "CHARTER%d", i);
			s.WriteKey(pszTemp, m_sCharter[i] );
		}
	}

	if ( ! m_sWebPageURL.IsEmpty())
	{
		s.WriteKey( "WEBPAGE", GetWebPageURL() );
	}
	if ( m_pts )
		s.WriteKeyVal("PTS", m_pts);

	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if (pMember->GetLinkUID().IsValidUID()) // To protect against characters that were deleted!
		{
			s.WriteKeyFormat( "MEMBER",
				"0%x,%s,%d,0%x,%d,%d,%d",
				pMember->GetLinkUID() | (pMember->GetLinkUID().IsItem() ? UID_F_ITEM : 0),
				(LPCTSTR) pMember->GetTitle(),
				pMember->GetPriv(),
				(DWORD) pMember->GetLoyalToUID(),
				pMember->m_UnDef.m_Val1,
				pMember->m_UnDef.m_Val2,
				pMember->GetAccountGold());
		}
	}
}

LPCTSTR CItemStone::GetAlignName() const
{
	static LPCTSTR const sm_AlignName[] = // STONEALIGN_TYPE
	{
		"standard",	// STONEALIGN_STANDARD
		"Order",	// STONEALIGN_ORDER
		"Chaos",	// STONEALIGN_CHAOS
	};
	int iAlign = GetAlignType();
	if ( iAlign >= COUNTOF( sm_AlignName ))
		iAlign = 0;
	return( sm_AlignName[ iAlign ] );
}

enum STC_TYPE
{
	STC_ABBREV,
	STC_AbbreviationToggle,
	STC_ALIGN,
	STC_AlignType,
	STC_CHARTER,
	STC_LoyalTo,
	STC_Master,
	STC_MasterGenderTitle,
	STC_MasterTitle,
	STC_MasterUid,
	STC_MEMBER,
	STC_PTS,
	STC_WEBPAGE,
	STC_QTY,
};

LPCTSTR const CItemStone::sm_szLoadKeys[STC_QTY+1] =
{
	"ABBREV",				// (R/W)
	"AbbreviationToggle",	// (W)
	"ALIGN",				// (R/W)
	"AlignType",			// (W)
	"CHARTER",				// (R/W)
	"LoyalTo",				// (W)
	"Master",				// (W)
	"MasterGenderTitle",	// (W)
	"MasterTitle",			// (W)
	"MasterUid",
	"MEMBER",				// (R/W)
	"PTS",
	"WEBPAGE",				// (R/W)
	NULL,
};

enum STCM_TYPE
{
	STCM_ACCOUNTGOLD,
	STCM_TITLE,
	STCM_ISCANDIDATE,
	STCM_ISMASTER,
	STCM_ISMEMBER,
	STCM_LOYALTO,
	STCM_PRIV,
	STCM_PRIVNAME,
	STCM_SHOWABBREV,
	STCM_UID,
	STCM_QTY,
};

LPCTSTR const CItemStone::sm_szLoadKeysM[STCM_QTY+1] =
{
	"ACCOUNTGOLD",
	"GUILDTITLE",
	"IsCandidate",
	"ISMASTER",
	"IsMember",
	"LOYALTO",
	"PRIV",
	"PRIVNAME",
	"ShowAbbrev",
	"UID",
	NULL,
};

enum STCG_TYPE
{
	STCG_THEYALLIANCE,
	STCG_THEYWAR,
	STCG_WEALLIANCE,
	STCG_WEWAR,
	STCG_QTY,
};

LPCTSTR const CItemStone::sm_szLoadKeysG[STCG_QTY+1] =
{
	"THEYALLIANCE",
	"THEYWAR",
	"WEALLIANCE",
	"WEWAR",
	NULL,
};

bool CItemStone::r_LoadVal( CScript & s ) // Load an item Script
{
	EXC_TRY("LoadVal");
	if ( !strnicmp("member.", s.GetKey(), 7) )
	{
		LPCTSTR pszCmd = s.GetKey()+7;
		if ( !pszCmd[0] )
			return false;

		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());

		for ( int i = 0 ; pMember != NULL; pMember = pMember->GetNext() )
		{
			if ( !pMember->GetLinkUID().IsChar() ) 
				continue;
				
			if ( nNumber == i )
			{
				switch ( FindTableSorted(pszCmd, sm_szLoadKeysM, COUNTOF(sm_szLoadKeysM)-1) )
				{
					case STCM_ACCOUNTGOLD:
						pMember->SetAccountGold(s.GetArgVal());
						return true;
					case STCM_LOYALTO:
						{
							UID uid = s.GetArgVal();
							pMember->SetLoyalTo(uid.CharFind());
						}
						return true;
					case STCM_PRIV:
						pMember->SetPriv((STONEPRIV_TYPE)s.GetArgVal());
						return true;
					case STCM_TITLE:
						pMember->SetTitle(s.GetArgStr());
						return true;
					case STCM_SHOWABBREV:
						pMember->SetAbbrev(s.GetArgVal()?1:0);
						return true;
					default:
						{
							CScriptObj *pRef = pMember->GetLinkUID().CharFind();
							if (pRef) return pRef->r_SetVal(pszCmd,s.GetArgStr());
							else return 0;
						}
				}
			}
			i++;
		}
		return false;
	}

	if ( !strnicmp("guild.", s.GetKey(), 6) )
	{
		LPCTSTR pszCmd = s.GetKey()+6;

		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	}

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case STC_ABBREV: // "ABBREV"
			m_sAbbrev = s.GetArgStr();
			return true;
		case STC_ALIGN: // "ALIGN"
			SetAlignType( (STONEALIGN_TYPE) s.GetArgVal());
			return true;
		case STC_MasterUid:
			{
				UID pNewMasterUid = (DWORD) s.GetArgVal();
				CChar * pChar = pNewMasterUid.CharFind();
				if ( !pChar )
					return false;
					
				CStoneMember * pNewMaster = GetMember( pChar );
				if ( !pNewMaster )
					return false;
					
				CStoneMember * pMaster = GetMasterMember();
				if ( pMaster )
				{
					if ( pMaster->GetLinkUID() == pNewMasterUid )
						return true;

					pMaster->SetPriv(STONEPRIV_MEMBER);
					//pMaster->SetLoyalTo(pChar);
				}
				
				//pNewMaster->SetLoyalTo(pChar);
				pNewMaster->SetPriv(STONEPRIV_MASTER);
			}
			return true;
		case STC_MEMBER: // "MEMBER"
			{
			TCHAR *Arg_ppCmd[8];		// Maximum parameters in one line
			int Arg_Qty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ), "," );
			if (Arg_Qty < 1) // must at least provide the member uid
				return false;

			CStoneMember * pNew = new CStoneMember(
				this,
				ATOI(Arg_ppCmd[0]),													// Member's UID
				Arg_Qty>2?(STONEPRIV_TYPE)ATOI(Arg_ppCmd[2]):STONEPRIV_CANDIDATE,	// Members priv level (use as a type)
				Arg_Qty>1?Arg_ppCmd[1]:"",											// Title
				Arg_Qty>3?ATOI( Arg_ppCmd[3] ):0,									// Member is loyal to this
				Arg_Qty>4?ATOI( Arg_ppCmd[4] ):0,									// Paperdoll stone abbreviation (also if they declared war)
				Arg_Qty>5?ATOI( Arg_ppCmd[5] ):0,									// If we declared war
				Arg_Qty>6?ATOI( Arg_ppCmd[6] ):0);									// AccountGold
			return true;
			}
		case STC_PTS:
			m_pts = s.GetArgVal();
			return true;

		case STC_WEBPAGE: // "WEBPAGE"
			m_sWebPageURL = s.GetArgStr();
			return true;
	}

	if ( s.IsKeyHead( sm_szLoadKeys[STC_CHARTER], 7 ))
	{
		int i = ATOI(s.GetKey()+7);
		if ( i >= COUNTOF(m_sCharter))
			return false;
		m_sCharter[i] = s.GetArgStr();
		return true;
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
	EXC_TRY("WriteVal");
	CChar * pCharSrc = pSrc->GetChar();

	if (!strnicmp("member.",pszKey,7))
	{
		LPCTSTR pszCmd = pszKey+7;

		if (!strcmpi(pszCmd,"COUNT"))
		{
			int i=0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
			for (; pMember != NULL; pMember = pMember->GetNext())
			{
				if (!pMember->GetLinkUID().IsChar()) 
					continue;
					
				i++;
			}
			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for (int i=0 ; pMember != NULL; pMember = pMember->GetNext())
		{
			if (!pMember->GetLinkUID().IsChar()) 
				continue;
				
			if (nNumber==i)
			{
				sVal.FormatHex(pMember->GetLinkUID());
				
				if (!pszCmd[0]) 
					return true;

				switch ( FindTableSorted( pszCmd, sm_szLoadKeysM, COUNTOF( sm_szLoadKeysM )-1 ))
				{
					case STCM_ACCOUNTGOLD:
						sVal.FormatVal(pMember->GetAccountGold());
						return true;
					case STCM_ISMASTER:
						sVal.FormatVal(pMember->IsPrivMaster());
						return true;
					case STCM_ISMEMBER:
						sVal.FormatVal(pMember->IsPrivMember());
						return true;
					case STCM_LOYALTO:
						sVal.FormatHex(pMember->GetLoyalToUID());
						return true;
					case STCM_PRIV:
						sVal.FormatVal(pMember->GetPriv());
						return true;
					case STCM_PRIVNAME:
						sVal=pMember->GetPrivName();
						return true;
					case STCM_TITLE:
						sVal=pMember->GetTitle();
						return true;
					case STCM_SHOWABBREV:
						sVal.FormatVal(pMember->IsAbbrevOn());
						return true;
					case STCM_ISCANDIDATE:
						sVal.FormatVal((pMember->GetPriv() == STONEPRIV_CANDIDATE) ? 1 : 0);
						return true;
					default:
						{
							CScriptObj *pRef = pMember->GetLinkUID().CharFind();
							return pRef->r_WriteVal(pszCmd,sVal,pSrc);
						}
				}
			}
			i++;
		}
		return true;
	}

	// GUILD.X.
	if (!strnicmp("guild.",pszKey,6))
	{
		LPCTSTR pszCmd = pszKey+6;

		if (!strcmpi(pszCmd,"COUNT"))
		{
			int i=0;
			CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
			for (; pMember != NULL; pMember = pMember->GetNext())
			{
				if (pMember->GetLinkUID().IsChar()) continue;
				i++;
			}
			sVal.FormatVal(i);
			return true;
		}
		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPARATORS(pszCmd);

		CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
		sVal.FormatVal(0);

		for (int i=0 ; pMember != NULL; pMember = pMember->GetNext())
		{
			if (pMember->GetLinkUID().IsChar()) 
				continue;
				
			if (nNumber==i)
			{
				sVal.FormatHex(pMember->GetLinkUID());
				
				if (!pszCmd[0]) 
					return true;

				switch ( FindTableSorted( pszCmd, sm_szLoadKeysG, COUNTOF( sm_szLoadKeysG )-1 ))
				{
					case STCG_THEYWAR:
						sVal.FormatVal(pMember->GetTheyDeclared());
						return true;
					case STCG_WEWAR:
						sVal.FormatVal(pMember->GetWeDeclared());
						return true;
					case STCG_WEALLIANCE:
						sVal.FormatVal(0);
						return true;
					case STCG_THEYALLIANCE:
						sVal.FormatVal(0);
						return true;
					default:
						{
							CScriptObj *pRef = pMember->GetLinkUID().ItemFind();
							return pRef->r_WriteVal(pszCmd,sVal,pSrc);
						}
				}
			}
		}
		return true;
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
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
				if ( pMember == NULL )
				{
					sVal = "nonmember";
					return true;
				}
				sVal = pMember->IsAbbrevOn() ? "On" : "Off";
			}
			return true;
		case STC_AlignType:
			sVal = GetAlignName();
			return true;
		case STC_LoyalTo:
			{
				CStoneMember * pMember = GetMember(pCharSrc);
				if ( pMember == NULL )
				{
					sVal = "nonmember";
					return true;
				}
				CChar * pLoyalTo = pMember->GetLoyalToUID().CharFind();
				if (pLoyalTo == NULL || pLoyalTo == pCharSrc )
					sVal = "yourself";
				else
					sVal = pLoyalTo->GetName();
			}
			return true;
	
		case STC_Master:
			{
				CChar * pMaster = GetMaster();
				sVal = (pMaster) ? pMaster->GetName() : "vote pending";
			}
			return true;
	
		case STC_MasterGenderTitle:
			{
				CChar * pMaster = GetMaster();
				if ( pMaster == NULL )
					sVal = ""; // If no master (vote pending)
				else if ( pMaster->Char_GetDef()->IsFemale())
					sVal = "Mistress";
				else
					sVal = "Master";
			}
			return true;
	
		case STC_MasterTitle:
			{
				CStoneMember * pMember = GetMasterMember();
				sVal = (pMember) ? pMember->GetTitle() : "";
			}
			return true;
	
		case STC_MasterUid:
			{
				CChar *pMaster = GetMaster();
				sVal.FormatHex( pMaster ? (int)pMaster->GetUID() : 0 );
			}
			break;

		case STC_PTS:
			sVal.FormatVal(m_pts);
			break;

		default:
			return CItem::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

CStoneMember * CItemStone::GetMasterMember() const
{
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetPriv() == STONEPRIV_MASTER )
			return pMember;
	}
	return NULL;
}

bool CItemStone::IsPrivMember( const CChar * pChar ) const
{
	const CStoneMember * pMember = GetMember(pChar);
	if ( pMember == NULL )
		return false;
	return( pMember->IsPrivMember());
}

CChar * CItemStone::GetMaster() const
{
	CStoneMember * pMember = GetMasterMember();
	if ( pMember == NULL )
		return NULL;
	return pMember->GetLinkUID().CharFind();
}

CStoneMember * CItemStone::GetMember( const CObjBase * pObj ) const
{
	// Get member info for this char/item (if it has member info)
	if (!pObj)
		return NULL;
	UID otherUID = pObj->GetUID();
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->GetLinkUID() == otherUID )
			return pMember;
	}
	return NULL;
}

bool CItemStone::NoMembers() const
{
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
	for ( ; pMember != NULL; pMember = pMember->GetNext())
	{
		if ( pMember->IsPrivMember())
			return false;
	}
	return true;
}

CStoneMember * CItemStone::AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull)
{
	// CLIMODE_TARG_STONE_RECRUIT
	// Set as member or candidate.

	if ( !pChar || !pChar->m_pPlayer )
	{
		Speak( "Only players can be members!");
		return NULL;
	}
	else if ( !pChar->IsClient() )
	{
		Speak( "This player must be online to become a member." );
		return NULL;
	}

	TEMPSTRING(z);
	CItemStone * pStone = pChar->Guild_Find( GetMemoryType());
	if ( pStone && pStone != this )
	{
		sprintf(z, "%s appears to belong to %s. Must resign previous %s", pChar->GetName(), pStone->GetName(), GetTypeName());
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
			sprintf(z, "%s is already %s %s.", pChar->GetName(), pMember->GetPrivName(), GetName());
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

	sprintf(z, "%s is now %s %s", pChar->GetName(), pMember->GetPrivName(), GetName());
	Speak(z);
	return pMember;
}

void CItemStone::ElectMaster()
{
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
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
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
	pMember = static_cast <CStoneMember *>(GetHead());
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
	pMember = static_cast <CStoneMember *>(GetHead());
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
		pMember = static_cast <CStoneMember *>(GetHead());
		for (; pMember != NULL; pMember = pMember->GetNext())
		{
			WeDeclarePeace(pMember->GetLinkUID(), true);
		}
	}
}

bool CItemStone::IsUniqueName( LPCTSTR pName ) // static
{
	for ( int i=0; i<g_World.m_Stones.GetCount(); i++ )
	{
		if ( ! strcmpi( pName, g_World.m_Stones[i]->GetName()))
			return false;
	}
	return true;
}

bool CItemStone::CheckValidMember( CStoneMember * pMember )
{
	if ( GetAmount()==0 || g_Serv.m_iExitFlag )	// no reason to elect new if the stone is dead.
		return true;	// we are deleting anyhow.

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
			return true;
		case STONEPRIV_ENEMY:
			{
				CItemStone * pEnemyStone = dynamic_cast <CItemStone *>( pMember->GetLinkUID().ItemFind());
				if ( pEnemyStone == NULL )
					break;
				CStoneMember * pEnemyMember = pEnemyStone->GetMember(this);
				if ( pEnemyMember == NULL )
					break;
				if ( pMember->GetWeDeclared() && ! pEnemyMember->GetTheyDeclared())
					break;
				if ( pMember->GetTheyDeclared() && ! pEnemyMember->GetWeDeclared())
					break;
			}
			return true;
	}

	// just delete this member. (it is mislinked)
	g_Log.Error("Stone UID=0%x has mislinked member uid=0%x\n", uid(), (DWORD)pMember->GetLinkUID());
	return false;
}

int CItemStone::FixWeirdness()
{
	// Check all my members. Make sure all wars are reciprocated and members are flaged.

	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
	{
		return( iResultCode );
	}

	bool fChanges = false;
	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
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
	return 0;
}

bool CItemStone::IsAtWarWith( const CItemStone * pEnemyStone ) const
{
	// Boths guild shave declared war on each other.

	if ( pEnemyStone == NULL )
		return false;

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
	// Announce we are at war or peace.
	bool fAtWar = IsAtWarWith(pEnemyStone);

	TEMPSTRING(pszTemp);
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

	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
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
	if (!pEnemyStone)
		return false;
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
		return false;
	}

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

	// announce to both sides.
	AnnounceWar( pEnemyStone, true, true );
	pEnemyStone->AnnounceWar( this, false, true );
	return true;
}

void CItemStone::TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace )
{
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

	// announce to both sides.
	pEnemyStone->AnnounceWar( this, true, false );
	AnnounceWar( pEnemyStone, false, false );
}

void CItemStone::WeDeclarePeace(UID uid, bool fForcePeace)
{
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

void CItemStone::SetupMenu( CClient * pClient, bool fMasterFunc )
{
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

enum ISV_TYPE
{
	ISV_ACCEPTCANDIDATE,
	ISV_APPLYTOJOIN,
	ISV_CHANGEALIGN,
	ISV_DECLAREFEALTY,
	ISV_DECLAREPEACE,
	ISV_DECLAREWAR,
	ISV_DISMISSMEMBER,
	ISV_GRANTTITLE,
	ISV_JOINASMEMBER,
	ISV_MASTERMENU,
	ISV_RECRUIT,
	ISV_REFUSECANDIDATE,
	ISV_RESIGN,
	ISV_RETURNMAINMENU,
	ISV_SETABBREVIATION,
	ISV_SETCHARTER,
	ISV_SETGMTITLE,
	ISV_SETNAME,
	ISV_TOGGLEABBREVIATION,
	ISV_VIEWCANDIDATES,
	ISV_VIEWCHARTER,
	ISV_VIEWENEMYS,
	ISV_VIEWROSTER,
	ISV_VIEWTHREATS,
	ISV_QTY,
};

LPCTSTR const CItemStone::sm_szVerbKeys[ISV_QTY+1] =
{
	"ACCEPTCANDIDATE",
	"APPLYTOJOIN",
	"CHANGEALIGN",
	"DECLAREFEALTY",
	"DECLAREPEACE",
	"DECLAREWAR",
	"DISMISSMEMBER",
	"GRANTTITLE",
	"JOINASMEMBER",
	"MASTERMENU",
	"RECRUIT",
	"REFUSECANDIDATE",
	"RESIGN",
	"RETURNMAINMENU",
	"SETABBREVIATION",
	"SETCHARTER",
	"SETGMTITLE",
	"SETNAME",
	// "TELEPORT",
	"TOGGLEABBREVIATION",
	"VIEWCANDIDATES",
	"VIEWCHARTER",
	"VIEWENEMYS",
	"VIEWROSTER",
	"VIEWTHREATS",
	NULL,
};

bool CItemStone::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	EXC_TRY("Verb");
	// NOTE:: ONLY CALL this from CChar::r_Verb !!!
	// Little to no security checking here !!!
	CChar * pCharSrc = pSrc->GetChar();
	if ( pCharSrc == NULL || ! pCharSrc->IsClient())
	{
		return( CItem::r_Verb( s, pSrc ));
	}

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	if ( index < 0 )
	{
		return( CItem::r_Verb( s, pSrc ));
	}

	CClient * pClient = pCharSrc->GetClient();
	CStoneMember * pMember = GetMember(pCharSrc);

	switch ( index )
	{
	case ISV_ACCEPTCANDIDATE:
		addStoneDialog(pClient,STONEDISP_ACCEPTCANDIDATE);
		break;
	case ISV_APPLYTOJOIN:
		AddRecruit( pClient->GetChar(), STONEPRIV_CANDIDATE );
		break;
	case ISV_CHANGEALIGN:
		if ( s.HasArgs())
		{
			SetAlignType( (STONEALIGN_TYPE) s.GetArgVal());
			TEMPSTRING(pszMsg);
			sprintf(pszMsg, "%s is now a %s %s\n", GetName(), GetAlignName(), GetTypeName());
			Speak(pszMsg);
		}
		else
		{
			pClient->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_GUILD_ALIGN"), this );
		}
		break;
	case ISV_DECLAREFEALTY:
		addStoneDialog(pClient,STONEDISP_FEALTY);
		break;
	case ISV_DECLAREPEACE:
		addStoneDialog(pClient,STONEDISP_DECLAREPEACE);
		break;
	case ISV_DECLAREWAR:
		addStoneDialog(pClient,STONEDISP_DECLAREWAR);
		break;
	case ISV_DISMISSMEMBER:
		addStoneDialog(pClient,STONEDISP_DISMISSMEMBER);
		break;
	case ISV_GRANTTITLE:
		addStoneDialog(pClient,STONEDISP_GRANTTITLE);
		break;
	case ISV_JOINASMEMBER:
		AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
		break;
	case ISV_MASTERMENU:
		SetupMenu( pClient, true );
		break;
	case ISV_RECRUIT:
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
	case ISV_RESIGN:
		delete pMember;
		break;
	case ISV_RETURNMAINMENU:
		SetupMenu( pClient );
		break;
	case ISV_SETABBREVIATION:
		pClient->addPromptConsole( CLIMODE_PROMPT_STONE_SET_ABBREV, "What shall the abbreviation be?" );
		break;
	case ISV_SETCHARTER:
		addStoneDialog(pClient,STONEDISP_SETCHARTER);
		break;
	case ISV_SETGMTITLE:
		pClient->addPromptConsole( CLIMODE_PROMPT_STONE_SET_TITLE, "What shall thy title be?" );
		break;
	case ISV_SETNAME:
		{
			TEMPSTRING(pszMsg);
			sprintf(pszMsg, "What would you like to rename the %s to?", (LPCTSTR) GetTypeName());
			pClient->addPromptConsole(CLIMODE_PROMPT_STONE_NAME, pszMsg);
		}
		break;
	//case ISV_TELEPORT:
	//	break;
	case ISV_TOGGLEABBREVIATION:
		if ( pMember == NULL )
			break;
		pMember->ToggleAbbrev();
		SetupMenu( pClient );
		break;
	case ISV_VIEWCANDIDATES:
		addStoneDialog(pClient,STONEDISP_CANDIDATES);
		break;
	case ISV_VIEWCHARTER:
		addStoneDialog(pClient,STONEDISP_VIEWCHARTER);
		break;
	case ISV_VIEWENEMYS:
		addStoneDialog(pClient,STONEDISP_VIEWENEMYS);
		break;
	case ISV_VIEWROSTER:
		addStoneDialog(pClient,STONEDISP_ROSTER);
		break;
	case ISV_VIEWTHREATS:
		addStoneDialog(pClient,STONEDISP_VIEWTHREATS);
		break;
	default:
		return false;
	}
	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

//	hardcoded gump operations
#define ADD2GUMPHC(_a_) { \
	CDialogDef::Element storage; \
	storage.m_index = iControls; \
	storage.m_contents.Format _a_ ; \
	m_controls.push_back(storage); \
	iControls++; }
#define ADDT2GUMPHC(_a_) { \
	CDialogDef::Element storage; \
	storage.m_index = iTexts; \
	storage.m_contents.Format _a_ ; \
	m_text.push_back(storage); \
	iTexts++; }

void CItemStone::addStoneSetViewCharter( CClient * pClient, STONEDISP_TYPE iStoneMenu )
{
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
		"button 195 370 2130 2129 1 0 %d",	// Save button
		"button 255 370 2121 2120 1 0 0"		// Cancel button
	};

	vector<CDialogDef::Element> m_controls;
	vector<CDialogDef::Element> m_text;

	int iControls = 0;
	while ( iControls < COUNTOF(sm_szDefControls))
	{
		// Fix the button ID so we can trap it later
		ADD2GUMPHC((sm_szDefControls[iControls], ( iControls > 4 ) ? (int) iStoneMenu : (int) GetDispID()));
	}

	bool fView = (iStoneMenu == STONEDISP_VIEWCHARTER);

	int iTexts=0;
	ADDT2GUMPHC((GetName()));
	ADDT2GUMPHC(("%s %s Charter", (fView) ? "View": "Set", (LPCTSTR)GetTypeName()))
	ADDT2GUMPHC(("and Web Link"));
	ADDT2GUMPHC(("Charter"));

	for ( int iLoop = 0; iLoop < COUNTOF(m_sCharter); iLoop++)
	{
		if ( fView )
		{
			ADD2GUMPHC(("text 50 %d 0 %d", 155 + (iLoop*22), iLoop + 4));
		}
		else
		{
			ADD2GUMPHC(("gumppic 40 %d 1143", 152 + (iLoop*22)));
			ADD2GUMPHC(("textentry 50 %d 250 32 0 %d %d", 155 + (iLoop*22), iLoop + 1000, iLoop + 4));
		}
		if ( fView && iLoop == 0 && m_sCharter[0].IsEmpty())
		{
			ADDT2GUMPHC(("No charter has been specified."));
		}
		else
		{
			ADDT2GUMPHC((GetCharter(iLoop)));
		}
	}

	if ( fView )
	{
		ADD2GUMPHC(("text 50 331 0 11"));
	}
	else
	{
		ADD2GUMPHC(("gumppic 40 328 1143"));
		ADD2GUMPHC(("textentry 50 331 250 32 0 1006 11"));
	}

	ADDT2GUMPHC(("Web Link"));
	ADDT2GUMPHC((GetWebPageURL()));
	ADDT2GUMPHC(((fView) ? "Go to the web page": "Save this information"));

	//	TODO:GUMP:VJAKA
	//pClient->addGumpDialog( CLIMODE_DIALOG_GUILD, sControls, iControls, sText, iTexts, 0x6e, 0x46 );
}

bool CItemStone::IsInMenu( STONEDISP_TYPE iStoneMenu, const CStoneMember * pMember ) const
{
	switch ( iStoneMenu )
	{
	case STONEDISP_ROSTER:
	case STONEDISP_FEALTY:
	case STONEDISP_GRANTTITLE:
		if ( ! pMember->IsPrivMember())
			return false;
		break;
	case STONEDISP_DISMISSMEMBER:
		if ( ! pMember->IsPrivMember() && pMember->GetPriv() != STONEPRIV_ACCEPTED )
			return false;
		break;
	case STONEDISP_ACCEPTCANDIDATE:
	case STONEDISP_REFUSECANDIDATE:
	case STONEDISP_CANDIDATES:
		if ( pMember->GetPriv() != STONEPRIV_CANDIDATE )
			return false;
		break;
	case STONEDISP_DECLAREPEACE:
	case STONEDISP_VIEWENEMYS:
		if ( pMember->GetPriv() != STONEPRIV_ENEMY)
			return false;
		if ( !pMember->GetWeDeclared())
			return false;
		break;
	case STONEDISP_VIEWTHREATS:
		if ( pMember->GetPriv() != STONEPRIV_ENEMY)
			return false;
		if ( !pMember->GetTheyDeclared())
			return false;
		break;
	default:
		return false;
	}
	return true;
}

bool CItemStone::IsInMenu( STONEDISP_TYPE iStoneMenu, const CItemStone * pOtherStone ) const
{
	if ( iStoneMenu != STONEDISP_DECLAREWAR )
		return false;

	if ( pOtherStone == this )
		return false;

	CStoneMember * pMember = GetMember( pOtherStone );
	if (pMember)
	{
		if ( pMember->GetWeDeclared())	// already declared.
			return false;
	}
	else
	{
		if ( pOtherStone->GetCount() <= 0 )	// Only stones with members can have war declared against them
			return false;
	}

	return true;
}

int CItemStone::addStoneListSetup( STONEDISP_TYPE iStoneMenu, CGString * psText, int iTexts )
{
	// ARGS: psText = NULL if i just want to count.

	if ( iStoneMenu == STONEDISP_DECLAREWAR )
	{
		// This list is special.
		for ( int i=0; i<g_World.m_Stones.GetCount(); i++ )
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

	CStoneMember * pMember = static_cast <CStoneMember *>(GetHead());
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
	// Add a list of members of a type.
	// Estimate the size first.
	static LPCTSTR const sm_szDefControls[] =
	{
		// "nomove",
		"page 0",
		"resizepic 0 0 5100 400 350",
		"text 15 10 0 0",
		"button 13 290 5050 5051 1 0 %d",
		"text 45 292 0 3",
		"button 307 290 5200 5201 1 0 0",
	};

	CGString sControls[512];
	int iControls = 0;

	int iControlLimit = COUNTOF(sm_szDefControls);
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
		"%s",
	};

	CGString sText[512];
	int iTexts=1;
	for ( ; iTexts<COUNTOF(sm_szDefText) - 1; iTexts++ )
	{
		sText[iTexts] = sm_szDefText[iTexts];
	}

	switch ( iStoneMenu )
	{
	case STONEDISP_ROSTER:
		sText[0].Format(sm_szDefText[0], GetName(), "Roster");
		break;
	case STONEDISP_CANDIDATES:
		sText[0].Format(sm_szDefText[0], GetName(), "Candidates");
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
		sText[0].Format(sm_szDefText[0], "Refuse candidate for", GetName());
		sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
			"Refuse this candidate membership");
		break;
	case STONEDISP_DISMISSMEMBER:
		sText[0].Format(sm_szDefText[0], "Dismiss member from", GetName());
		sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1],
			"Dismiss this member");
		break;
	case STONEDISP_DECLAREWAR:
		sText[0].Format(sm_szDefText[0], "Declaration of war by", GetName());
		sText[COUNTOF(sm_szDefText)-1].Format(	"Declare war on this %s", GetTypeName());
		break;
	case STONEDISP_DECLAREPEACE:
		sText[0].Format(sm_szDefText[0], "Declaration of peace by", GetName());
		sText[COUNTOF(sm_szDefText)-1].Format( "Declare peace with this %s", GetTypeName());
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
	}

	switch ( iStoneMenu )
	{
	case STONEDISP_ROSTER:
	case STONEDISP_CANDIDATES:
	case STONEDISP_VIEWTHREATS:
	case STONEDISP_VIEWENEMYS:
		sText[COUNTOF(sm_szDefText)-1].Format(sm_szDefText[COUNTOF(sm_szDefText)-1], "Done");
		break;
	}
	iTexts++;

	// First count the appropriate members
	int iMemberCount = addStoneListSetup( iStoneMenu, NULL, 0 );

	if ( iMemberCount+iTexts > COUNTOF(sText))
	{
		g_Log.Warn("Too Many Guilds\n");
		iMemberCount = COUNTOF(sText) - iTexts - 1;
	}

	int iPages = 0;
	for ( int iLoop = 0; iLoop < iMemberCount; iLoop++)
	{
		if (iLoop % 10 == 0)
		{
			iPages += 1;
			sControls[iControls++].Format("page %d", iPages);
			if (iPages > 1)
			{
				sControls[iControls++].Format("button 15 320 5223 5223 0 %d", iPages - 1);
				sControls[iControls++] = "text 40 317 0 1";
			}
			if ( iMemberCount > iPages * 10)
			{
				sControls[iControls++].Format("button 366 320 5224 5224 0 %d", iPages + 1);
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
				sControls[iControls++].Format("radio 20 %d 5002 5003 0 %d", ((iLoop % 10) * 25) + 35, iLoop + 1000);
			}
		case STONEDISP_ROSTER:
		case STONEDISP_CANDIDATES:
		case STONEDISP_VIEWENEMYS:
		case STONEDISP_VIEWTHREATS:
			{
				sControls[iControls++].Format("text 55 %d 0 %d", ((iLoop % 10) * 25) + 32, iLoop + 4);
			}
			break;
		}
	}

	iTexts = addStoneListSetup( iStoneMenu, sText, iTexts );
	//	TODO:GUMP:VJAKA
	//pClient->addGumpDialog( CLIMODE_DIALOG_GUILD, sControls, iControls, sText, iTexts, 0x6e, 0x46 );
}

void CItemStone::addStoneDialog( CClient * pClient, STONEDISP_TYPE menuid )
{
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
	}
}

bool CItemStone::OnDialogButton( CClient * pClient, STONEDISP_TYPE type, CDialogResponseArgs & resp )
{
	// Button presses come here
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
			for (int i = 0; i < resp.m_TextArray.GetCount(); i++)
			{
				int id = resp.m_TextArray[i]->m_ID - 1000;
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
		return true;

	default:
		return false;
	}

	if ( resp.m_CheckArray.GetCount() == 0 )	 // If they hit ok but didn't pick one, treat it like a cancel
		return true;

	int iMember = resp.m_CheckArray[0] - 1000;

	CStoneMember * pMember = NULL;
	bool fFound = false;
	int iLoop = 0;
	int iStoneIndex = 0;

	if ( type == STONEDISP_DECLAREWAR )
	{
		for ( ; iStoneIndex<g_World.m_Stones.GetCount(); iStoneIndex ++ )
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
		pMember = static_cast <CStoneMember *>(GetHead());
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
			AddRecruit( pMember->GetLinkUID().CharFind(), STONEPRIV_ACCEPTED );
			break;
		case STONEDISP_REFUSECANDIDATE:
			delete pMember;
			break;
		case STONEDISP_DISMISSMEMBER:
			delete pMember;
			break;
		case STONEDISP_FEALTY:
			{
				CStoneMember * pMe = GetMember(pClient->GetChar());
				if ( pMe == NULL ) return false;
				pMe->SetLoyalTo( pMember->GetLinkUID().CharFind());
			}
			break;
		case STONEDISP_DECLAREPEACE:
			WeDeclarePeace(pMember->GetLinkUID());
			break;
		case STONEDISP_GRANTTITLE:
			pClient->m_Targ_PrvUID = pMember->GetLinkUID();
			pClient->addPromptConsole( CLIMODE_PROMPT_STONE_GRANT_TITLE, "What title dost thou grant?" );
			return true;
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

void CItemStone::SetTownName()
{
	// For town stones.
	if ( ! IsTopLevel())
		return;
	CRegionBase * pArea = GetTopPoint().GetRegion(( IsType(IT_STONE_TOWN)) ? REGION_TYPE_AREA : REGION_TYPE_ROOM );
	if ( pArea )
	{
		pArea->SetName( GetIndividualName());
	}
}

bool CItemStone::MoveTo( CPointMap pt )
{
	// Put item on the ground here.
	if ( IsType(IT_STONE_TOWN) )
	{
		SetTownName();
	}
	return CItem::MoveTo(pt);
}

bool CItemStone::SetName( LPCTSTR pszName )
{
	// If this is a town then set the whole regions name.
	if ( !CItem::SetName(pszName) )
		return false;
	if ( IsTopLevel() && IsType(IT_STONE_TOWN) )
		SetTownName();

	return true;
}

bool CItemStone::OnPromptResp( CClient * pClient, CLIMODE_TYPE TargMode, LPCTSTR pszText, CGString & sMsg )
{
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
			TEMPSTRING(pszMsg);
			sprintf(pszMsg, "What would you like to rename the %s to?", (LPCTSTR) GetTypeName());
			pClient->addPromptConsole(CLIMODE_PROMPT_STONE_NAME,pszMsg);
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
			CStoneMember * pMember = GetMember( pClient->m_Targ_PrvUID.CharFind());
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
	}
	return true;
}

void CItemStone::Use_Item( CClient * pClient )
{
	if ( NoMembers() && IsType(IT_STONE_GUILD)) // Everyone resigned...new master
	{
		AddRecruit( pClient->GetChar(), STONEPRIV_MEMBER );
	}
	SetupMenu( pClient );
}
