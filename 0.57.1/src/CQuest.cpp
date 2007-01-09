#include "graysvr.h"
#include "CClient.h"

//*****************************************************************
// -CCharRefArray

int CCharRefArray::FindChar( const CChar * pChar ) const
{
	if ( pChar == NULL )
	{
		return -1;
	}
	UID uid( pChar->GetUID());
	int iQty = m_uidCharArray.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		if ( uid == m_uidCharArray[i] )
			return i;
	}
	return -1;
}

int CCharRefArray::AttachChar( const CChar * pChar )
{
	int i = FindChar( pChar );
	if ( i >= 0 )
		return i;
	return m_uidCharArray.Add( pChar->GetUID() );
}

void CCharRefArray::DetachChar( int i )
{
	m_uidCharArray.RemoveAt(i);
}

int CCharRefArray::DetachChar( const CChar * pChar )
{
	int i = FindChar( pChar );
	if ( i < 0 )
		return -1;
	DetachChar(i);
	return i;
}

void CCharRefArray::DeleteChars()
{
	int iQty = m_uidCharArray.GetCount();
	for ( int k=iQty-1; k >= 0; --k )
	{
		delete m_uidCharArray[k].CharFind();
	}
	m_uidCharArray.RemoveAll();
}


void CCharRefArray::WritePartyChars( CScript & s )
{
	int iQty = m_uidCharArray.GetCount();
	for ( int j=0; j<iQty; j++ )	// write out links to all my chars
	{
		s.WriteKeyHex( "CHARUID", m_uidCharArray[j] );
	}
}

//*****************************************************************
// -CPartyDef

CPartyDef::CPartyDef( CChar * pChar1, CChar *pChar2 )
{
	// pChar1 = the master.
	pChar1->m_pParty = this;
	pChar2->m_pParty = this;
	AttachChar(pChar1);
	AttachChar(pChar2);
	SendAddList( NULL );	// send full list to all.
	m_sName.Format("Party_0%x", pChar1->uid());
}

// ---------------------------------------------------------
int CPartyDef::AttachChar( CChar * pChar )
{
	// RETURN:
	//  index of the char in the group. -1 = not in group.
	int i = m_Chars.AttachChar( pChar );
	SetLootFlag( pChar, false );;
	return i;
}

int CPartyDef::DetachChar( CChar * pChar )
{
	// RETURN:
	//  index of the char in the group. -1 = not in group.
	int i = m_Chars.DetachChar( pChar );
	if ( i != -1 )
	{
		VariableList::Variable *sTempVal = pChar->GetTagDefs()->GetKey("PARTY_CANLOOTME");
		if ( sTempVal )
			pChar->DeleteKey("PARTY_CANLOOTME");

		sTempVal = pChar->GetTagDefs()->GetKey("PARTY_LASTINVITE");
		if ( sTempVal )
			pChar->DeleteKey("PARTY_LASTINVITE");
	}
	return i;
}

void CPartyDef::SetLootFlag( CChar * pChar, bool fSet )
{
	if ( IsInParty(pChar) )
	{
		pChar->SetKeyNum("PARTY_CANLOOTME", fSet);
		SysMessageChar( pChar, fSet ? g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LOOT_ALLOW ) : g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LOOT_BLOCK ));
	}
}

bool CPartyDef::GetLootFlag( const CChar * pChar )
{
	if ( IsInParty(pChar) )
	{
		return pChar->GetKeyNum("PARTY_CANLOOTME", true);
	}

	return false;
}

bool CPartyDef::FixWeirdness( CChar * pChar )
{
	if ( !pChar )
	{
		return false;
	}

	if ( pChar->m_pParty != this )
	{
		return( DetachChar( pChar ) >= 0 );	// this is bad!
	}
	else if ( ! m_Chars.IsCharIn( pChar ))
	{
		pChar->m_pParty = NULL;
		return true;
	}

	return false;
}

// ---------------------------------------------------------
int CPartyDef::CraftAddList( CExtData * pExtData )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Add;

	int iQty;
	iQty = m_Chars.GetCharCount();
	
	if ( !iQty )
		return -1;

	for ( int i = 0; i < iQty; i++ )
	{
		pExtData->Party_Msg_Data.m_uids[i] = m_Chars.GetChar(i);
	}

	pExtData->Party_Msg_Data.m_Qty = iQty;

	return( (iQty*sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftEmptyList( CExtData * pExtData, CChar * pChar )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Remove;
	pExtData->Party_Msg_Data.m_uids[0] = pChar->GetUID();
	pExtData->Party_Msg_Data.m_Qty = 0;

	return( (sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftRemoveList( CExtData * pExtData, CChar * pChar )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Remove;
	pExtData->Party_Msg_Data.m_uids[0] = pChar->GetUID();

	int iQty;
	iQty = m_Chars.GetCharCount();
	
	if ( !iQty )
	{
		return -1;
	}

	int x = 1;
	for ( int i = 0; i < iQty; i++ )
	{
		if ( (m_Chars.GetChar(i)) && ( pChar->GetUID() != m_Chars.GetChar(i) ) )
		{
			pExtData->Party_Msg_Data.m_uids[x] = m_Chars.GetChar(i);
			x++;
		}
	}

	pExtData->Party_Msg_Data.m_Qty = x;

	return( (x*sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftMessage( CExtData * pExtData, CChar * pFrom, const NCHAR * pText, int len )
{
	pExtData->Party_Msg_Rsp.m_code = PARTYMSG_Msg;
	pExtData->Party_Msg_Rsp.m_UID = pFrom->GetUID();

	if ( len > MAX_TALK_BUFFER )
		len = MAX_TALK_BUFFER;
	
	memcpy( pExtData->Party_Msg_Rsp.m_msg, pText, len );

	return( len + 5 );
}

// ---------------------------------------------------------
void CPartyDef::AddStatsUpdate( CChar * pChar, CCommand * cmd, int iLen )
{
	int iQty;
	iQty = m_Chars.GetCharCount();

	if ( !iQty )
		return;

	for ( int i = 0; i < iQty; i++ )
	{
		CChar * pCharNow = m_Chars.GetChar(i).CharFind();
		if ( pCharNow && pCharNow != pChar )
		{
			if ( pCharNow->CanSee( pChar ) && pCharNow->IsClient() )
			{
				pCharNow->GetClient()->xSend(cmd, iLen);
			}
		}
	}
}
// ---------------------------------------------------------
void CPartyDef::SysMessageStatic( CChar * pChar, LPCTSTR pText )
{
	// SysMessage to a member/or not with [PARTY]:
	if ( pChar && pChar->IsClient() )
	{
		CClient * pClient = pChar->GetClient();
		TEMPSTRING(pCharTemp);
		sprintf(pCharTemp, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_MSG), pText);
		pClient->addBark(pCharTemp, NULL, HUE_GREEN_LIGHT, TALKMODE_SYSTEM, FONT_NORMAL);
	}
}

void CPartyDef::SysMessageAll( LPCTSTR pText )
{
	// SysMessage to all members of the party.
	int iQty = m_Chars.GetCharCount();
	for ( int i=0; i<iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		SysMessageChar( pChar, pText );
	}
}


// ---------------------------------------------------------
bool CPartyDef::SendMemberMsg( CChar * pCharDest, const CExtData * pExtData, int iLen )
{
	if ( pCharDest == NULL )
	{
		SendAll( pExtData, iLen );
		return true;
	}

	// Weirdness check.
	if ( pCharDest->m_pParty != this )
	{
		if ( DetachChar( pCharDest ) >= 0 )	// this is bad!
			return false;
		return true;
	}
	else if ( ! m_Chars.IsCharIn( pCharDest ))
	{
		pCharDest->m_pParty = NULL;
		return true;
	}

	if ( pCharDest->IsClient())
	{
		CClient * pClient = pCharDest->GetClient();
		pClient->addExtData( EXTDATA_Party_Msg, pExtData, iLen );
		if ( pExtData->Party_Msg_Data.m_code == PARTYMSG_Remove )
		{
			pClient->addReSync();
		}
	}

	return true;
}

void CPartyDef::SendAll( const CExtData * pExtData, int iLen )
{
	// Send this to all members of the party.
	int iQty = m_Chars.GetCharCount();
	for ( int i = 0; i < iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		if ( ! SendMemberMsg( pChar, pExtData, iLen ))
		{
			iQty--;
			i--;
		}
	}
}

// ---------------------------------------------------------
bool CPartyDef::SendAddList( CChar * pCharDest )
{
	CExtData extdata;
	int iLen = CraftAddList(&extdata);
	
	if (iLen != -1)
	{
		if ( pCharDest )
		{
			SendMemberMsg(pCharDest, &extdata, iLen);
		}
		else
		{
			SendAll(&extdata, iLen);
		}
	}

	return ( iLen != -1 );
}

bool CPartyDef::SendRemoveList( CChar * pCharRemove, bool bFor )
{
	CExtData ExtData;
	int iLen;

	if ( bFor )
	{
		iLen = CraftEmptyList( &ExtData, pCharRemove );
	}
	else
	{
		iLen = CraftRemoveList( &ExtData, pCharRemove );
	}

	if ( iLen == -1 )
		return false;

	if ( bFor ) 
	{
		SendMemberMsg(pCharRemove, &ExtData, iLen);
	}
	else
	{
		SendAll( &ExtData, iLen );
	}

	return true;
}

// ---------------------------------------------------------
bool CPartyDef::MessageEvent( UID uidDst, UID uidSrc, const NCHAR * pText, int ilenmsg )
{
	if ( pText == NULL )
		return false;

	if ( uidDst && !IsInParty( uidDst.CharFind() ) )
		return false;

	CChar * pFrom = uidSrc.CharFind();
	CChar * pTo = NULL;
	if ( uidDst != (DWORD) 0 )
		pTo = uidDst.CharFind();

	TEMPSTRING(szText);
	CvtNUNICODEToSystem( szText, MAX_TALK_BUFFER, pText, MAX_TALK_BUFFER );

	if ( ! m_pSpeechFunction.IsEmpty() )
	{
		TRIGRET_TYPE tr = TRIGRET_RET_FALSE;
		CScriptTriggerArgs Args;
		Args.m_iN1 = uidSrc;
		Args.m_iN2 = uidDst;
		Args.m_VarsLocal.SetStr("SPEECH", false, szText);

		if ( r_Call(m_pSpeechFunction, &g_Serv, &Args, NULL, &tr) )
		{
			if ( tr == TRIGRET_RET_TRUE )
				return false;
		}
	}

	g_Log.Event(LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' in party to '%s'\n",
		pFrom->GetClient()->socketId(), pFrom->GetName(), szText, pTo ? pTo->GetName() : "all");

	CExtData extData;
	int iLen = CraftMessage( &extData, pFrom, pText, ilenmsg );
	if ( iLen == -1 )
		return false;

	if ( pTo )
		SendMemberMsg( pTo, &extData, iLen );
	else
		SendAll( &extData, iLen );

	return true;
}

// ---------------------------------------------------------

void CPartyDef::AcceptMember( CChar * pChar )
{
	// This person has accepted to be part of the party.
	// SendAddList( pChar->GetUID(), NULL );	// tell all that there is a new member.

	pChar->m_pParty = this;
	AttachChar(pChar);

	// "You have been added to the party"
	// NOTE: We don't have to send the full party to everyone. just the new guy.
	// SendAddList( UID_CLEAR, pChar );
	// SendAddList( pChar->GetUID(), NULL );
	// SendAddList( UID_CLEAR, NULL );

	SendAddList( NULL );
	// else
	//	throwerror !!
}


bool CPartyDef::RemoveMember( UID uidRemove, UID uidCommand )
{
	// ARGS:
	//  uid = Who is being removed.
	//  uidAct = who removed this person. (Only the master or self can remove)
	//
	// NOTE: remove of the master will cause the party to disband.
	if ( !m_Chars.GetCharCount() ) return false;

	UID uidMaster = GetMaster();
	if ( uidRemove != uidCommand && uidCommand != uidMaster )
	{
		return false;
	}

	CChar * pCharRemove = uidRemove.CharFind();

	if (( pCharRemove == NULL ) || !IsInParty(pCharRemove) )
	{
		return false;
	}

	if ( uidRemove == uidMaster )
	{
		return( Disband( uidMaster ));
	}

	LPCTSTR pszForceMsg = ( (DWORD) uidCommand != (DWORD) uidRemove ) ? g_Cfg.GetDefaultMsg( DEFMSG_PARTY_PART_1 ) : g_Cfg.GetDefaultMsg( DEFMSG_PARTY_PART_2 );

	{
		// Tell the kicked person they are out
		TEMPSTRING(pszMsg);
		sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LEAVE_2 ), pszForceMsg );
		SysMessageChar(pCharRemove, pszMsg);
		// Remove it
		SendRemoveList( pCharRemove, true );
		DetachChar( pCharRemove );
		pCharRemove->m_pParty = NULL;
	}

	if ( m_Chars.GetCharCount() <= 1 )
	{
		// Disband the party
		// "The last person has left the party"
		return( Disband( uidMaster ) );
	}
	else
	{
		SendRemoveList( pCharRemove, false );
		// Tell the others he is gone
		TEMPSTRING(pszMsg);
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_LEAVE_1), pCharRemove->GetName(), pszForceMsg);
		SysMessageAll(pszMsg);
	}

	return true;
}

bool CPartyDef::Disband( UID uidMaster )
{
	// Make sure i am the master.
	if ( ! m_Chars.GetCharCount())
	{
		return false;
	}

	if ( GetMaster() != uidMaster )
	{
		return false;
	}

	SysMessageAll(g_Cfg.GetDefaultMsg( DEFMSG_PARTY_DISBANDED ));

	int iQty = m_Chars.GetCharCount();
	for ( int i=0; i<iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		if ( pChar == NULL )
			continue;
		SendRemoveList( pChar, true );
		pChar->m_pParty = NULL;
	}

	delete this;	// should remove itself from the world list.
	return true;
}

// ---------------------------------------------------------
bool CPartyDef::DeclineEvent( CChar * pCharDecline, UID uidInviter )	// static
{
	// This should happen after a timeout as well.
	// " You notify %s that you do not wish to join the party"

	CChar * pCharInviter = uidInviter.CharFind();
	if ( !pCharInviter || !pCharDecline )
		return false;

	if ( uidInviter == pCharDecline->GetUID() )
	{
		return false;
	}

	VariableList::Variable *sTempVal = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
	if ( !sTempVal )
		return false;

	if ((DWORD)sTempVal->GetValNum() != pCharDecline->uid())
		return false;

	// Remove the key
	pCharInviter->DeleteKey("PARTY_LASTINVITE");

	TEMPSTRING(sTemp);
	sprintf(sTemp, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_2), pCharInviter->GetName());
	CPartyDef::SysMessageStatic( pCharDecline, sTemp );
	sprintf(sTemp, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE_1), pCharDecline->GetName());
	CPartyDef::SysMessageStatic( pCharInviter, sTemp );

	return true;
}

bool CPartyDef::IsPartyFull() const
{
	return ( m_Chars.GetCharCount() >= config.get("client.party.max") );
}

bool CPartyDef::AcceptEvent( CChar * pCharAccept, UID uidInviter, bool bForced )	// static
{
	// We are accepting the invite to join a party
	// No security Here if bForced -> true !!!

	// Party master is only one that can add ! GetChar(0)
	CChar * pCharInviter = uidInviter.CharFind();
	if ( pCharInviter == NULL )
	{
		return false;
	}
	if ( pCharInviter == pCharAccept )
	{
		return false;
	}
	if ( !pCharInviter->IsClient() || !pCharAccept->IsClient() )
	{
		return false;
	}

	CPartyDef * pParty = pCharInviter->m_pParty;
	if ( !bForced )
	{
		VariableList::Variable *sTempVal = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
		if ( !sTempVal )
			return false;

		if ((DWORD)sTempVal->GetValNum() != pCharAccept->uid())
			return false;

		// Remove the key
		pCharInviter->DeleteKey("PARTY_LASTINVITE");

		if ( !pCharInviter->CanSee( pCharAccept ) )
			return false;
	}

	if ( pCharAccept->m_pParty != NULL )	// Aready in a party !
	{
		if ( pParty == pCharAccept->m_pParty )	// already in this party
			return true;

		if ( bForced )
		{
			pCharAccept->m_pParty->RemoveMember( pCharAccept->GetUID(), pCharAccept->GetUID() );
			pCharAccept->m_pParty = NULL;
		}
		else
			return false;
	}

	TEMPSTRING(pszMsg);
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_JOINED), pCharAccept->GetName());

	if ( pParty == NULL )
	{
		// create the party now.
		pParty = new CPartyDef( pCharInviter, pCharAccept );
		g_World.m_Parties.InsertHead( pParty );
		pParty->SysMessageChar(pCharInviter, pszMsg);
	}
	else
	{
		if ( !bForced )
		{
			if ( !pParty->IsPartyMaster(pCharInviter) )
				return false;

			if ( pParty->IsPartyFull() )
				return false;
		}
		// Just add to existing party. Only the party master can do this !
		pParty->SysMessageAll(pszMsg);	// tell everyone already in the party about this.
		pParty->AcceptMember( pCharAccept );
	}

	pParty->SysMessageChar( pCharAccept, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_ADDED) );
	return true;
}

// ---------------------------------------------------------

enum PDV_TYPE
{
	#define ADD(a,b) PDV_##a,
	#include "tables/CQuest_functions.tbl"
	#undef ADD
	PDV_QTY,
};

LPCTSTR const CPartyDef::sm_szVerbKeys[PDV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CQuest_functions.tbl"
	#undef ADD
	NULL,
};

enum PDC_TYPE
{
	#define ADD(a,b) PDC_##a,
	#include "tables/CQuest_props.tbl"
	#undef ADD
	PDC_QTY,
};

LPCTSTR const CPartyDef::sm_szLoadKeys[PDC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "tables/CQuest_props.tbl"
	#undef ADD
	NULL,
};

bool CPartyDef::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef ) 
{
	if ( !strnicmp("MASTER.",pszKey,7) )
	{
		pszKey += 7; 

		CChar * pMaster = GetMaster().CharFind();

		if ( pMaster != NULL ) 
		{ 
			pRef = pMaster; 
			return true; 
		}
	}
	else if ( !strnicmp("MEMBER.",pszKey,7) )
	{
		pszKey += 7;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPARATORS(pszKey);

		if ( nNumber < 0 || nNumber > m_Chars.GetCharCount() )
			return false;

		CChar * pMember = m_Chars.GetChar(nNumber).CharFind();

		if ( pMember != NULL ) 
		{ 
			pRef = pMember; 
			return true; 
		}
	}

	return false;
}

bool CPartyDef::r_LoadVal( CScript & s )
{ 
	EXC_TRY("LoadVal");
	LPCTSTR pszKey = s.GetKey();

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1);
	
	switch ( index )
	{
		case PDC_SPEECHFILTER:
		{
			if ( !s.HasArgs() )
				this->m_pSpeechFunction.Empty();
			else
			{
				LPCTSTR pszArg = s.GetArgStr();
				CResourceLink * m_pTestEvent = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( RES_FUNCTION, pszArg ) );

				if ( m_pTestEvent == NULL )
					return false;
		
				this->m_pSpeechFunction.Format("%s", pszArg);
			}
		
		} break;

		case PDC_TAG0:
		case PDC_TAG:
		{
			bool fQuoted = false;
			pszKey = pszKey + ((index == PDC_TAG0) ? 5 : 4);
			m_TagDefs.SetStr( pszKey, fQuoted, s.GetArgStr( &fQuoted ), (index == PDC_TAG0) );
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

bool CPartyDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");

	CScriptObj * pRef;
	if ( r_GetRef( pszKey, pRef ))
	{
		if ( pRef == NULL )	// good command but bad link.
		{
			sVal = "0";
			return true;
		}
		if ( pszKey[0] == '\0' )	// we where just testing the ref.
		{
			CObjBase *	pObj	= dynamic_cast <CObjBase *> (pRef);
			if ( pObj )
				sVal.FormatHex(pObj->uid());
			else
				sVal.FormatVal( 1 );
			return true;
		}
		return pRef->r_WriteVal( pszKey, sVal, pSrc );
	}

	bool fZero = false;
	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case PDC_MEMBERS:
		{
			sVal.FormatVal(m_Chars.GetCharCount());
		} break;

		case PDC_SPEECHFILTER:
		{
			sVal = this->m_pSpeechFunction.IsEmpty() ? "" : this->m_pSpeechFunction;
		} break;

		case PDC_TAG0:
			fZero	= true;
			pszKey++;
		case PDC_TAG:
		{
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr( pszKey, fZero );
		} break;

		case PDC_TAGAT:
		{
 			pszKey += 5;	// eat the 'TAGAT'
 			if ( *pszKey == '.' )	// do we have an argument?
 			{
 				SKIP_SEPARATORS( pszKey );
 				int iQty = Exp_GetVal( pszKey );
				if ( iQty < 0 || iQty >= m_TagDefs.GetCount() )
 					return false;	// trying to get non-existent tag

				VariableList::Variable * pTagAt = m_TagDefs.GetAt( iQty );
 				if ( !pTagAt )
 					return false;	// trying to get non-existent tag

 				SKIP_SEPARATORS( pszKey );
 				if ( ! *pszKey )
 				{
 					sVal.Format( "%s=%s", (LPCTSTR) pTagAt->GetKey(), (LPCTSTR) pTagAt->GetValStr() );
 					return true;
 				}
 				if ( strnicmp(pszKey, "KEY", 3) )	// key?
 				{
 					sVal = pTagAt->GetKey();
 					return true;
 				}
 				if ( strnicmp(pszKey, "VAL", 3) )	// val?
 				{
 					sVal = pTagAt->GetValStr();
 					return true;
 				}
 			}
			return false;
		}

		case PDC_TAGCOUNT:
		{
			sVal.FormatVal( m_TagDefs.GetCount() );
		} break;

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

bool CPartyDef::r_Verb( CScript & s, CTextConsole * pSrc )
{
	EXC_TRY("Verb");
	LPCTSTR pszKey = s.GetKey();

	CScriptObj * pRef;
	if ( r_GetRef( pszKey, pRef ))
	{
		if ( pszKey[0] )
		{
			if ( !pRef ) return true;
			CScript script( pszKey, s.GetArgStr());
			return pRef->r_Verb( script, pSrc );
		}
	}

	int iIndex = FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );

	switch ( iIndex )
	{
		case PDV_ADDMEMBER:
		case PDV_ADDMEMBERFORCED:
		{
			bool bForced = ( iIndex == PDV_ADDMEMBERFORCED);
			UID toAdd = s.GetArgVal();
			CChar * pCharAdd = toAdd.CharFind();

			if ( !pCharAdd || IsInParty(pCharAdd) )
				return false;

			if ( !bForced )
				(GetMaster().CharFind())->SetKeyNum("PARTY_LASTINVITE", (DWORD)toAdd);
			
			return CPartyDef::AcceptEvent(pCharAdd, GetMaster(), bForced);
		} break;

		case PDV_CLEARTAGS:
		{
			LPCTSTR pszArg = s.GetArgStr();
			SKIP_SEPARATORS(pszArg);
			m_TagDefs.ClearKeys(pszArg);
		} break;

		case PDV_DISBAND:
		{
			return( Disband( GetMaster() ) );
		} break;

		case PDV_MESSAGE:
		{
			
		} break;

		case PDV_REMOVEMEMBER:
		{
			UID toRemove;
			LPCTSTR pszArg = s.GetArgStr();
			if ( *pszArg == '@' )
			{
				pszArg++;
				int nMember = Exp_GetVal(pszArg);
				if ( nMember < 0 || nMember > m_Chars.GetCharCount() )
					return false;

				toRemove = m_Chars.GetChar(nMember);
			}
			else
			{
				toRemove = (DWORD) s.GetArgVal();
			}

			if ( toRemove != (DWORD) 0 )
				return( RemoveMember( toRemove, GetMaster() ) );

			return false;
		} break;

		case PDV_SYSMESSAGE:
		{
			UID toSysmessage;
			LPCTSTR pszArg = s.GetArgStr();
			int x(0);

			TEMPSTRING(pUid);
			if ( *pszArg == '@' )
			{
				pszArg++;
				if ( *pszArg != '@' )
				{
					LPCTSTR __pszArg = pszArg;
					while ( *pszArg != ' ' ) { pszArg++; x++; }
					strcpylen(pUid, __pszArg, ++x);

					int nMember = Exp_GetVal(pUid);
					if ( nMember < 0 || nMember > m_Chars.GetCharCount() )
						return false;

					toSysmessage = m_Chars.GetChar(nMember);
				}
			}
			else
			{
				LPCTSTR __pszArg = pszArg;
				while ( *pszArg != ' ' ) { pszArg++; x++; }
				strcpylen(pUid, __pszArg, ++x);

				toSysmessage = (DWORD) Exp_GetVal(pUid);
			}

			SKIP_SEPARATORS( pszArg );

			if ( toSysmessage != (DWORD) 0 )
			{
				CChar * pSend = toSysmessage.CharFind();
				SysMessageChar( pSend, pszArg );
			}
			else
			{
				SysMessageAll( pszArg );
			}
		} break;

		case PDV_TAGLIST:
		{
			m_TagDefs.DumpKeys( pSrc, "TAG." );
		} break;

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

bool CPartyDef::r_Load( CScript & s )
{ 
	return false; 
}
