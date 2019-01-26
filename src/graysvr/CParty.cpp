#include "graysvr.h"	// predef header.
#include "../network/send.h"

//*****************************************************************
// -CCharRefArray

size_t CCharRefArray::FindChar( const CChar *pChar ) const
{
	ADDTOCALLSTACK("CCharRefArray::FindChar");
	if ( !pChar )
		return m_uidCharArray.BadIndex();

	CGrayUID uid(pChar->GetUID());
	size_t iQty = m_uidCharArray.GetCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		if ( uid == m_uidCharArray[i] )
			return i;
	}
	return m_uidCharArray.BadIndex();
}

size_t CCharRefArray::AttachChar( const CChar *pChar )
{
	ADDTOCALLSTACK("CCharRefArray::AttachChar");
	size_t i = FindChar(pChar);
	if ( i != m_uidCharArray.BadIndex() )
		return i;
	return m_uidCharArray.Add(pChar->GetUID());
}

size_t CCharRefArray::InsertChar( const CChar *pChar, size_t i )
{
	ADDTOCALLSTACK("CCharRefArray::InsertChar");
	size_t currentIndex = FindChar(pChar);
	if ( currentIndex != m_uidCharArray.BadIndex() )
	{
		if ( currentIndex == i )	// already there
			return i;
		DetachChar(currentIndex);	// remove from list
	}

	if ( !IsValidIndex(i) )		// prevent from being inserted too high
		i = GetCharCount();

	m_uidCharArray.InsertAt(i, pChar->GetUID() );
	return i;
}

void CCharRefArray::DetachChar( size_t i )
{
	ADDTOCALLSTACK("CCharRefArray::DetachChar");
	m_uidCharArray.RemoveAt(i);
}

size_t CCharRefArray::DetachChar( const CChar *pChar )
{
	ADDTOCALLSTACK("CCharRefArray::DetachChar");
	size_t i = FindChar(pChar);
	if ( i != m_uidCharArray.BadIndex() )
		DetachChar(i);
	return i;
}

void CCharRefArray::DeleteChars()
{
	ADDTOCALLSTACK("CCharRefArray::DeleteChars");
	size_t iQty = m_uidCharArray.GetCount();
	while ( iQty > 0 )
	{
		CChar *pChar = m_uidCharArray[--iQty].CharFind();
		if ( pChar )
			pChar->Delete();
	}
	m_uidCharArray.RemoveAll();
}


void CCharRefArray::WritePartyChars( CScript &s )
{
	ADDTOCALLSTACK("CCharRefArray::WritePartyChars");
	size_t iQty = m_uidCharArray.GetCount();
	for ( size_t j = 0; j < iQty; j++ )		// write out links to all my chars
		s.WriteKeyHex("CHARUID", m_uidCharArray[j]);
}

//*****************************************************************
// -CPartyDef

CPartyDef::CPartyDef( CChar *pCharInvite, CChar *pCharAccept )
{
	// pCharInviter = the master.
	AcceptMember(pCharInvite);
	AcceptMember(pCharAccept);
	SendAddList(NULL);		// send full list to all
	UpdateWaypointAll(pCharInvite, PartyMember);
	m_sName.Format("Party_0%lx", static_cast<DWORD>(pCharInvite->GetUID()));
}

// ---------------------------------------------------------
size_t CPartyDef::AttachChar( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::AttachChar");
	// RETURN:
	//  index of the char in the group. BadIndex = not in group.
	size_t i = m_Chars.AttachChar(pChar);
	pChar->NotoSave_Update();
	UpdateWaypointAll(pChar, PartyMember);
	return i;
}

size_t CPartyDef::DetachChar( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::DetachChar");
	// RETURN:
	//  index of the char in the group. BadIndex = not in group.
	size_t i = m_Chars.DetachChar(pChar);
	if ( i != m_Chars.BadIndex() )
	{
		// Remove map waypoint of party members
		if ( pChar->m_pClient )
		{
			size_t iQty = m_Chars.GetCharCount();
			CChar *pPartyMember = NULL;
			for ( size_t n = 0; n < iQty; n++ )
			{
				pPartyMember = m_Chars.GetChar(n).CharFind();
				if ( !pPartyMember )
					continue;
				pChar->m_pClient->addMapWaypoint(pPartyMember, Remove);
			}
		}
		UpdateWaypointAll(pChar, Remove);

		pChar->m_pParty = NULL;
		pChar->DeleteKey("PARTY_LASTINVITE");
		pChar->DeleteKey("PARTY_LASTINVITETIME");
		pChar->NotoSave_Update();
	}
	return i;
}

bool CPartyDef::SetMaster( CChar *pNewMaster )
{
	if ( !pNewMaster )
		return false;
	else if ( !IsInParty(pNewMaster) || IsPartyMaster(pNewMaster) )
		return false;

	size_t i = m_Chars.InsertChar(pNewMaster, 0);
	SendAddList(NULL);
	return (i == 0);
}

void CPartyDef::SetLootFlag( CChar *pChar, bool fSet )
{
	ADDTOCALLSTACK("CPartyDef::SetLootFlag");
	ASSERT(pChar);
	if ( IsInParty(pChar) )
	{
		pChar->SetKeyNum("PARTY_CANLOOTME", fSet);
		pChar->SysMessageDefault(fSet ? DEFMSG_PARTY_LOOT_ALLOW : DEFMSG_PARTY_LOOT_BLOCK);
	}
}

bool CPartyDef::GetLootFlag( const CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::GetLootFlag");
	ASSERT(pChar);
	if ( IsInParty(pChar) )
		return (pChar->GetKeyNum("PARTY_CANLOOTME") != 0);

	return false;
}

// ---------------------------------------------------------
void CPartyDef::StatsUpdateAll( CChar *pCharSrc, PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::StatsUpdateAll");
	size_t iQty = m_Chars.GetCharCount();
	if ( iQty <= 0 )
		return;

	CChar *pChar = NULL;
	for ( size_t i = 0; i < iQty; i++ )
	{
		pChar = m_Chars.GetChar(i).CharFind();
		if ( !pChar || !pChar->m_pClient || (pChar == pCharSrc) || !pChar->CanSee(pCharSrc) )
			continue;
		pPacket->send(pChar->m_pClient);
	}
}

void CPartyDef::SysMessageAll( LPCTSTR pText )
{
	ADDTOCALLSTACK("CPartyDef::SysMessageAll");
	// SysMessage to all party members.
	size_t iQty = m_Chars.GetCharCount();
	if ( iQty <= 0 )
		return;

	CChar *pChar = NULL;
	for ( size_t i = 0; i < iQty; i++ )
	{
		pChar = m_Chars.GetChar(i).CharFind();
		if ( !pChar || !pChar->m_pClient )
			continue;
		pChar->m_pClient->SysMessage(pText);
	}
}

void CPartyDef::UpdateWaypointAll( CChar *pCharSrc, MAPWAYPOINT_TYPE type )
{
	ADDTOCALLSTACK("CPartyDef::UpdateWaypointAll");
	// Send pCharSrc map waypoint location to all party members (enhanced client only)
	size_t iQty = m_Chars.GetCharCount();
	if ( iQty <= 0 )
		return;

	CChar *pChar = NULL;
	for ( size_t i = 0; i < iQty; i++ )
	{
		pChar = m_Chars.GetChar(i).CharFind();
		if ( !pChar || !pChar->m_pClient || (pChar == pCharSrc) )
			continue;
		pChar->m_pClient->addMapWaypoint(pCharSrc, type);
	}
}

// ---------------------------------------------------------
bool CPartyDef::SendMemberMsg( CChar *pCharDest, PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::SendMemberMsg");
	if ( !pCharDest )
	{
		SendAll(pPacket);
		return true;
	}

	// Weirdness check.
	if ( pCharDest->m_pParty != this )
	{
		if ( DetachChar(pCharDest) != m_Chars.BadIndex() )	// this is bad!
			return false;
		return true;
	}
	if ( !m_Chars.IsCharIn(pCharDest) )
	{
		pCharDest->m_pParty = NULL;
		return true;
	}

	if ( pCharDest->m_pClient )
		pPacket->send(pCharDest->m_pClient);
	return true;
}

void CPartyDef::SendAll( PacketSend *pPacket )
{
	ADDTOCALLSTACK("CPartyDef::SendAll");
	// Send this to all party members.
	size_t iQty = m_Chars.GetCharCount();
	if ( iQty <= 0 )
		return;

	CChar *pChar = NULL;
	for ( size_t i = 0; i < iQty; i++ )
	{
		pChar = m_Chars.GetChar(i).CharFind();
		if ( !pChar )
			continue;
		if ( !SendMemberMsg(pChar, pPacket) )
		{
			iQty--;
			i--;
		}
	}
}

// ---------------------------------------------------------
bool CPartyDef::SendAddList( CChar *pCharDest )
{
	ADDTOCALLSTACK("CPartyDef::SendAddList");

	if ( m_Chars.GetCharCount() <= 0 )
		return false;

	PacketPartyList cmd(&m_Chars);
	if (pCharDest)
		SendMemberMsg(pCharDest, &cmd);
	else
		SendAll(&cmd);

	return true;
}

bool CPartyDef::SendRemoveList( CChar *pCharRemove, bool bFor )
{
	ADDTOCALLSTACK("CPartyDef::SendRemoveList");

	if ( bFor )
	{
		PacketPartyRemoveMember cmd(pCharRemove, NULL);
		SendMemberMsg(pCharRemove, &cmd);
	}
	else
	{
		if ( m_Chars.GetCharCount() <= 0 )
			return false;

		PacketPartyRemoveMember cmd(pCharRemove, &m_Chars);
		SendAll(&cmd);
	}

	return true;
}

// ---------------------------------------------------------
bool CPartyDef::MessageEvent( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR *pText, int ilenmsg )
{
	ADDTOCALLSTACK("CPartyDef::MessageEvent");
	UNREFERENCED_PARAMETER(ilenmsg);
	if ( pText == NULL )
		return false;
	if ( uidDst && !IsInParty(uidDst.CharFind()) )
		return false;

	CChar *pFrom = uidSrc.CharFind();
	CChar *pTo = NULL;
	if ( uidDst )
		pTo = uidDst.CharFind();

	TCHAR *szText = Str_GetTemp();
	CvtNUNICODEToSystem(szText, MAX_TALK_BUFFER, pText, MAX_TALK_BUFFER);

	if ( !m_pSpeechFunction.IsEmpty() )
	{
		TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
		CScriptTriggerArgs Args;
		Args.m_iN1 = uidSrc;
		Args.m_iN2 = uidDst;
		Args.m_s1 = szText;
		Args.m_s1_raw = szText;

		if ( r_Call(m_pSpeechFunction, &g_Serv, &Args, NULL, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return false;
		}
	}

	if ( g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) )
		g_Log.Event(LOGM_PLAYER_SPEAK, "%lx:'%s' Says '%s' in party to '%s'\n", pFrom->m_pClient->GetSocketID(), pFrom->GetName(), szText, pTo ? pTo->GetName() : "all");

	sprintf(szText, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_MSG), pText);
	PacketPartyChat cmd(pFrom, pText);

	if ( pTo )
		SendMemberMsg(pTo, &cmd);
	else
		SendAll(&cmd);

	return true;
}

// ---------------------------------------------------------

void CPartyDef::AcceptMember( CChar *pChar )
{
	ADDTOCALLSTACK("CPartyDef::AcceptMember");
	// This person has accepted to be part of the party.
	ASSERT(pChar);

	pChar->m_pParty = this;
	AttachChar(pChar);
	SendAddList(NULL);
}

bool CPartyDef::RemoveMember( CGrayUID uidRemove, CGrayUID uidCommand )
{
	ADDTOCALLSTACK("CPartyDef::RemoveMember");
	// ARGS:
	//  uidRemove = Who is being removed.
	//  uidCommand = who removed this person (only the master or self can remove)
	//
	// NOTE: remove of the master will cause the party to disband.

	if ( m_Chars.GetCharCount() <= 0 )
		return false;

	CGrayUID uidMaster = GetMaster();
	if ( (uidRemove != uidCommand) && (uidCommand != uidMaster) )
		return false;

	CChar *pCharRemove = uidRemove.CharFind();
	if ( !pCharRemove )
		return false;
	if ( !IsInParty(pCharRemove) )
		return false;
	if ( uidRemove == uidMaster )
		return Disband(uidMaster);

	CChar *pSrc = uidCommand.CharFind();
	if ( pSrc && IsTrigUsed(TRIGGER_PARTYREMOVE) )
	{
		CScriptTriggerArgs args;
		if ( pCharRemove->OnTrigger(CTRIG_PartyRemove, pSrc, &args) == TRIGRET_RET_TRUE )
			return false;
	}
	if ( IsTrigUsed(TRIGGER_PARTYLEAVE) )
	{
		if ( pCharRemove->OnTrigger(CTRIG_PartyLeave, pCharRemove, 0) == TRIGRET_RET_TRUE )
			return false;
	}

	// Remove it from the party
	SendRemoveList(pCharRemove, true);
	DetachChar(pCharRemove);
	pCharRemove->SysMessageDefault(DEFMSG_PARTY_LEAVE_2);

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_LEAVE_1), pCharRemove->GetName());
	SysMessageAll(pszMsg);

	if ( m_Chars.GetCharCount() <= 1 )
	{
		// Disband the party
		SysMessageAll(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_LEAVE_LAST_PERSON));
		return Disband(uidMaster);
	}

	return true;
}

bool CPartyDef::Disband( CGrayUID uidMaster )
{
	ADDTOCALLSTACK("CPartyDef::Disband");
	// Make sure i am the master.
	if ( m_Chars.GetCharCount() <= 0 )
		return false;
	if ( GetMaster() != uidMaster )
		return false;

	CChar *pMaster = GetMaster().CharFind();
	if ( pMaster && IsTrigUsed(TRIGGER_PARTYDISBAND) )
	{
		CScriptTriggerArgs args;
		if ( pMaster->OnTrigger(CTRIG_PartyDisband, pMaster, &args) == TRIGRET_RET_TRUE )
			return false;
	}

	SysMessageAll(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DISBANDED));

	CChar *pSrc = uidMaster.CharFind();
	size_t iQty = m_Chars.GetCharCount();
	ASSERT(iQty > 0);
	CChar *pChar = NULL;
	for ( size_t i = 0; i < iQty; i++ )
	{
		pChar = m_Chars.GetChar(i).CharFind();
		if ( !pChar )
			continue;

		if ( IsTrigUsed(TRIGGER_PARTYREMOVE) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = 1;
			pChar->OnTrigger(CTRIG_PartyRemove, pSrc, &args);
		}

		SendRemoveList(pChar, true);
		DetachChar(pChar);
	}

	delete this;	// should remove itself from the world list.
	return true;
}

// ---------------------------------------------------------
bool CPartyDef::DeclineEvent( CChar *pCharDecline, CGrayUID uidInviter )	// static
{
	ADDTOCALLSTACK("CPartyDef::DeclineEvent");
	// This should happen after a timeout as well.
	// "You notify %s that you do not wish to join the party"

	CChar *pCharInviter = uidInviter.CharFind();
	if ( !pCharInviter || !pCharDecline || uidInviter == pCharDecline->GetUID() )
		return false;

	CVarDefCont *pVar = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
	if ( !pVar || (static_cast<CGrayUID>(pVar->GetValNum()) != pCharDecline->GetUID()) )
		return false;

	pCharInviter->DeleteKey("PARTY_LASTINVITE");
	pCharInviter->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_1), pCharDecline->GetName());
	pCharDecline->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_2), pCharInviter->GetName());
	return true;
}

bool CPartyDef::AcceptEvent( CChar *pCharAccept, CGrayUID uidInviter, bool bForced )	// static
{
	ADDTOCALLSTACK("CPartyDef::AcceptEvent");
	// We are accepting the invite to join a party
	// No security checks if bForced -> true !!!
	// Party master is only one that can add ! GetChar(0)

	CChar *pCharInviter = uidInviter.CharFind();
	if ( !pCharInviter || !pCharInviter->m_pClient || !pCharAccept || !pCharAccept->m_pClient || (pCharInviter == pCharAccept) )
		return false;

	CPartyDef *pParty = pCharInviter->m_pParty;
	if ( !bForced )
	{
		CVarDefCont *pVar = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
		if ( !pVar || (static_cast<CGrayUID>(pVar->GetValNum()) != pCharAccept->GetUID()) )
			return false;

		pCharInviter->DeleteKey("PARTY_LASTINVITE");
		if ( !pCharInviter->CanSee(pCharAccept) )
			return false;
	}

	if ( pCharAccept->m_pParty )	// already in a party
	{
		if ( pParty == pCharAccept->m_pParty )	// already in this party
			return true;

		if ( bForced )
		{
			pCharAccept->m_pParty->RemoveMember(pCharAccept->GetUID(), pCharAccept->GetUID());
			pCharAccept->m_pParty = NULL;
		}
		else
			return false;
	}

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_JOINED), pCharAccept->GetName());

	if ( !pParty )
	{
		// Create the party now.
		pParty = new CPartyDef(pCharInviter, pCharAccept);
		ASSERT(pParty);
		g_World.m_Parties.InsertHead(pParty);
		pCharInviter->SysMessage(pszMsg);
	}
	else
	{
		// Add to existing party
		if ( pParty->IsPartyFull() || (!bForced && !pParty->IsPartyMaster(pCharInviter)) )
			return false;

		pParty->SysMessageAll(pszMsg);	// tell everyone already in the party about this
		pParty->AcceptMember(pCharAccept);
	}

	pCharAccept->SysMessageDefault(DEFMSG_PARTY_ADDED);
	return true;
}

// ---------------------------------------------------------

enum PDV_TYPE
{
	#define ADD(a,b) PDV_##a,
	#include "../tables/CParty_functions.tbl"
	#undef ADD
	PDV_QTY
};

LPCTSTR const CPartyDef::sm_szVerbKeys[PDV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CParty_functions.tbl"
	#undef ADD
	NULL
};

enum PDC_TYPE
{
	#define ADD(a,b) PDC_##a,
	#include "../tables/CParty_props.tbl"
	#undef ADD
	PDC_QTY
};

LPCTSTR const CPartyDef::sm_szLoadKeys[PDC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CParty_props.tbl"
	#undef ADD
	NULL
};

bool CPartyDef::r_GetRef( LPCTSTR &pszKey, CScriptObj *&pRef )
{
	ADDTOCALLSTACK("CPartyDef::r_GetRef");
	if ( !strnicmp("MASTER.", pszKey, 7) )
	{
		pszKey += 7;
		CChar *pMaster = GetMaster().CharFind();
		if ( pMaster )
		{
			pRef = pMaster;
			return true;
		}
	}
	else if ( !strnicmp("MEMBER.", pszKey, 7) )
	{
		pszKey += 7;
		size_t nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);
		if ( !m_Chars.IsValidIndex(nNumber) )
			return false;

		CChar *pMember = m_Chars.GetChar(nNumber).CharFind();
		if ( pMember )
		{
			pRef = pMember;
			return true;
		}
	}
	return false;
}

bool CPartyDef::r_LoadVal( CScript &s )
{ 
	ADDTOCALLSTACK("CPartyDef::r_LoadVal");
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	switch ( index )
	{
		case PDC_SPEECHFILTER:
		{
			if ( !s.HasArgs() )
				m_pSpeechFunction.Empty();
			else
			{
				LPCTSTR pszArg = s.GetArgStr();
				CResourceLink *m_pTestEvent = dynamic_cast<CResourceLink *>(g_Cfg.ResourceGetDefByName(RES_FUNCTION, pszArg));

				if ( !m_pTestEvent )
					return false;

				m_pSpeechFunction.Format("%s", pszArg);
			}

		} break;

		case PDC_TAG0:
		case PDC_TAG:
		{
			bool fQuoted = false;
			pszKey = pszKey + ((index == PDC_TAG0) ? 5 : 4);
			m_TagDefs.SetStr(pszKey, fQuoted, s.GetArgStr(&fQuoted), (index == PDC_TAG0));
		} break;
		
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CPartyDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CPartyDef::r_WriteVal");
	EXC_TRY("WriteVal");

	CScriptObj *pRef;
	if ( r_GetRef(pszKey, pRef) )
	{
		if ( pRef == NULL )		// good command but bad link.
		{
			sVal = "0";
			return true;
		}
		if ( pszKey[0] == '\0' )	// we where just testing the ref.
		{
			CObjBase *pObj = dynamic_cast<CObjBase *>(pRef);
			if ( pObj )
				sVal.FormatHex(static_cast<DWORD>(pObj->GetUID()));
			else
				sVal.FormatVal(1);
			return true;
		}
		return pRef->r_WriteVal(pszKey, sVal, pSrc);
	}

	bool fZero = false;
	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case PDC_ISSAMEPARTYOF:
		{
			pszKey += 13;
			GETNONWHITESPACE(pszKey);
			if ( pszKey[0] != '\0' )
			{
				CGrayUID uidToCheck = static_cast<CGrayUID>(Exp_GetVal(pszKey));
				CChar *pCharToCheck = uidToCheck.CharFind();

				sVal.FormatVal(pCharToCheck && (pCharToCheck->m_pParty == this));
			}
			else
				return false;
		} break;

		case PDC_MEMBERS:
			sVal.FormatVal(m_Chars.GetCharCount());
			break;

		case PDC_SPEECHFILTER:
			sVal = m_pSpeechFunction.IsEmpty() ? "" : m_pSpeechFunction;
			break;

		case PDC_TAG0:
			fZero = true;
			pszKey++;
		case PDC_TAG:
		{
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, fZero);
		} break;

		case PDC_TAGAT:
		{
			pszKey += 5;	// eat the 'TAGAT'
			if ( *pszKey == '.' )	// do we have an argument?
			{
				SKIP_SEPARATORS(pszKey);
				size_t iQty = static_cast<size_t>(Exp_GetVal(pszKey));
				if ( iQty >= m_TagDefs.GetCount() )
					return false;	// trying to get non-existant tag

				CVarDefCont *pVar = m_TagDefs.GetAt(iQty);
				if ( !pVar )
					return false;	// trying to get non-existant tag

				SKIP_SEPARATORS(pszKey);
				if ( !*pszKey )
				{
					sVal.Format("%s=%s", pVar->GetKey(), pVar->GetValStr());
					return true;
				}
				else if ( !strnicmp(pszKey, "KEY", 3) )
				{
					sVal = pVar->GetKey();
					return true;
				}
				else if ( !strnicmp(pszKey, "VAL", 3) )
				{
					sVal = pVar->GetValStr();
					return true;
				}
			}
			return false;
		}

		case PDC_TAGCOUNT:
			sVal.FormatVal(m_TagDefs.GetCount());
			break;

		default:
			return false;
	}

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CPartyDef::r_Verb( CScript &s, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CPartyDef::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	LPCTSTR pszKey = s.GetKey();
	CScriptObj *pRef;
	if ( r_GetRef(pszKey, pRef) )
	{
		if ( pszKey[0] )
		{
			if ( !pRef )
				return true;
			CScript script(pszKey, s.GetArgStr());
			return pRef->r_Verb(script, pSrc);
		}
	}

	int iIndex = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	switch ( iIndex )
	{
		case PDV_ADDMEMBER:
		case PDV_ADDMEMBERFORCED:
		{
			bool bForced = (iIndex == PDV_ADDMEMBERFORCED);
			CGrayUID toAdd = static_cast<CGrayUID>(s.GetArgVal());
			CChar *pCharAdd = toAdd.CharFind();
			CChar *pCharMaster = GetMaster().CharFind();
			if ( !pCharAdd || IsInParty(pCharAdd) )
				return false;

			if ( pCharMaster && !bForced )
				pCharMaster->SetKeyNum("PARTY_LASTINVITE", (long long)toAdd);
			
			return CPartyDef::AcceptEvent(pCharAdd, GetMaster(), bForced);
		} break;

		case PDV_CLEARTAGS:
		{
			LPCTSTR pszArg = s.GetArgStr();
			SKIP_SEPARATORS(pszArg);
			m_TagDefs.ClearKeys(pszArg);
		} break;

		case PDV_CREATE:
			return true;

		case PDV_DISBAND:
			return Disband(GetMaster());

		case PDV_MESSAGE:
			break;

		case PDV_REMOVEMEMBER:
		{
			CGrayUID toRemove;
			LPCTSTR pszArg = s.GetArgStr();
			if ( *pszArg == '@' )
			{
				pszArg++;
				size_t nMember = Exp_GetVal(pszArg);
				if ( !m_Chars.IsValidIndex(nMember) )
					return false;

				toRemove = m_Chars.GetChar(nMember);
			}
			else
				toRemove = static_cast<CGrayUID>(s.GetArgVal());

			if ( toRemove )
				return RemoveMember(toRemove, GetMaster());

			return false;
		} break;

		case PDV_SETMASTER:
		{
			CGrayUID newMaster;
			LPCTSTR pszArg = s.GetArgStr();
			if ( *pszArg == '@' )
			{
				pszArg++;
				size_t nMember = Exp_GetVal(pszArg);
				if ( nMember == 0 || !m_Chars.IsValidIndex(nMember) )
					return false;

				newMaster = m_Chars.GetChar(nMember);
			}
			else
				newMaster = static_cast<CGrayUID>(s.GetArgVal());

			if ( newMaster )
				return SetMaster(newMaster.CharFind());

			return false;
		} break;

		case PDV_SYSMESSAGE:
		{
			CGrayUID toSysmessage;
			LPCTSTR pszArg = s.GetArgStr();
			TCHAR *pUid = Str_GetTemp();
			size_t x = 0;

			if ( *pszArg == '@' )
			{
				pszArg++;
				if ( *pszArg != '@' )
				{
					LPCTSTR __pszArg = pszArg;
					while ( *pszArg != ' ' )
					{
						pszArg++;
						x++;
					}
					strcpylen(pUid, __pszArg, ++x);

					size_t nMember = Exp_GetVal(pUid);
					if ( !m_Chars.IsValidIndex(nMember) )
						return false;

					toSysmessage = m_Chars.GetChar(nMember);
				}
			}
			else
			{
				LPCTSTR __pszArg = pszArg;
				while ( *pszArg != ' ' )
				{
					pszArg++;
					x++;
				}
				strcpylen(pUid, __pszArg, ++x);

				toSysmessage = static_cast<CGrayUID>(Exp_GetVal(pUid));
			}

			SKIP_SEPARATORS(pszArg);

			if ( toSysmessage )
			{
				CChar *pSend = toSysmessage.CharFind();
				pSend->SysMessage(pszArg);
			}
			else
				SysMessageAll(pszArg);
		} break;

		case PDV_TAGLIST:
			m_TagDefs.DumpKeys(pSrc, "TAG.");
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

bool CPartyDef::r_Load( CScript &s )
{ 
	ADDTOCALLSTACK("CPartyDef::r_Load");
	UNREFERENCED_PARAMETER(s);
	return false; 
}
