//
// CClient.h
//

#ifndef _INC_CCLIENT_H
#define _INC_CCLIENT_H
#pragma once

enum CV_TYPE
{
	#define ADD(a,b) CV_##a,
	#include "../tables/CClient_functions.tbl"
	#undef ADD
	CV_QTY,
};

enum CC_TYPE
{
	#define ADD(a,b) CC_##a,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	CC_QTY,
};

class CPartyDef : public CGObListRec, public CScriptObj
{
	// a list of characters in the party.
	#define MAX_CHAR_IN_PARTY 10

public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];
private:
	CCharRefArray m_Chars;
	CGString m_sName;
	CVarDefMap m_TagDefs;
	CGString m_pSpeechFunction;

private:
	bool SendMemberMsg( CChar * pCharDest, PacketSend * pPacket );
	void SendAll( PacketSend * pPacket );
	// List manipulation
	int AttachChar( CChar * pChar );
	int DetachChar( CChar * pChar );
	bool FixWeirdness( CChar * pChar ); // true = weirdness found

public:
	CPartyDef( CChar * pCharInvite, CChar * pCharAccept );

private:
	CPartyDef(const CPartyDef& copy);
	CPartyDef& operator=(const CPartyDef& other);

public:
	static bool AcceptEvent( CChar * pCharAccept, CGrayUID uidInviter, bool bForced = false );
	static bool DeclineEvent( CChar * pCharDecline, CGrayUID uidInviter );
	static void SysMessageStatic( CChar * pChar, LPCTSTR pText );

	bool IsPartyFull() const
	{
		return( m_Chars.GetCharCount() >= MAX_CHAR_IN_PARTY );
	}
	bool IsInParty( const CChar * pChar ) const
	{
		int i = m_Chars.FindChar( pChar );
		return( i >= 0 );
	}
	bool IsPartyMaster( const CChar * pChar ) const
	{
		int i = m_Chars.FindChar( pChar );
		return( i == 0 );
	}

	CGrayUID GetMaster() 
	{ 
		return( m_Chars.GetChar(0) ); 
	}

	
	// Refresh status for party members
	void AddStatsUpdate( CChar * pChar, PacketSend * pPacket );
	// List sending wrappers
	bool SendRemoveList( CChar * pCharRemove, bool bFor );
	bool SendAddList( CChar * pCharDest );
	// Party message sending wrappers
	bool MessageEvent( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// void MessageAll( CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// bool MessageMember( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// Sysmessage sending wrappers
	void SysMessageAll( LPCTSTR pText );
	void SysMessageChar( CChar * pChar, LPCTSTR pText ) { CPartyDef::SysMessageStatic( pChar, pText); };

	// Commands
	bool Disband( CGrayUID uidMaster );
	bool RemoveMember( CGrayUID uidRemove, CGrayUID uidCommand );
	void AcceptMember( CChar * pChar );
	void SetLootFlag( CChar * pChar, bool fSet );
	bool GetLootFlag( const CChar * pChar );
	bool SetMaster( CChar * pChar );
	
	// -------------------------------

	LPCTSTR GetName() const { return (LPCTSTR)m_sName; }
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	bool r_LoadVal( CScript & s );
	bool r_Load( CScript & s );
};

#endif	// _INC_CCLIENT_H
