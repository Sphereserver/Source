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
	CV_QTY
};

enum CC_TYPE
{
	#define ADD(a,b) CC_##a,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	CC_QTY
};

class CPartyDef : public CGObListRec, public CScriptObj
{
	// a list of characters in the party.
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];
private:
	CCharRefArray m_Chars;
	CGString m_sName;
	CVarDefMap m_TagDefs;
	CVarDefMap m_BaseDefs;		// New Variable storage system
	CGString m_pSpeechFunction;

public:
	LPCTSTR GetDefStr(LPCTSTR pszKey, bool fZero = false) const
	{
		return m_BaseDefs.GetKeyStr(pszKey, fZero);
	}

	INT64 GetDefNum(LPCTSTR pszKey) const
	{
		return m_BaseDefs.GetKeyNum(pszKey);
	}

	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

private:
	bool SendMemberMsg( CChar *pCharDest, PacketSend *pPacket );
	void SendAll( PacketSend *pPacket );
	// List manipulation
	size_t AttachChar( CChar *pChar );
	size_t DetachChar( CChar *pChar );

public:
	CPartyDef( CChar *pCharInvite, CChar *pCharAccept );

private:
	CPartyDef(const CPartyDef &copy);
	CPartyDef &operator=(const CPartyDef &other);

public:
	static bool AcceptEvent( CChar *pCharAccept, CGrayUID uidInviter, bool bForced = false );
	static bool DeclineEvent( CChar *pCharDecline, CGrayUID uidInviter );

	bool IsPartyFull() const
	{
		return (m_Chars.GetCharCount() >= 10);
	}
	bool IsInParty( const CChar *pChar ) const
	{
		return m_Chars.IsCharIn(pChar);
	}
	bool IsPartyMaster( const CChar *pChar ) const
	{
		return (m_Chars.FindChar(pChar) == 0);
	}

	CGrayUID GetMaster() 
	{ 
		return m_Chars.GetChar(0); 
	}

	// Functions sent to all party members
	void StatsUpdateAll( CChar *pCharSrc, PacketSend *pPacket );
	void SysMessageAll( LPCTSTR pText );
	void UpdateWaypointAll( CChar *pCharSrc, MAPWAYPOINT_TYPE type );
	// List sending wrappers
	bool SendRemoveList( CChar *pCharRemove, bool bFor );
	bool SendAddList( CChar *pCharDest );
	// Party message sending wrappers
	bool MessageEvent( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR *pText, int ilenmsg );

	// Commands
	bool Disband( CGrayUID uidMaster );
	bool RemoveMember( CGrayUID uidRemove, CGrayUID uidCommand );
	void AcceptMember( CChar *pChar );
	void SetLootFlag( CChar *pChar, bool fSet );
	bool GetLootFlag( const CChar *pChar );
	bool SetMaster( CChar *pChar );
	
	// -------------------------------

	LPCTSTR GetName() const { return static_cast<LPCTSTR>(m_sName); }
	bool r_GetRef( LPCTSTR &pszKey, CScriptObj *&pRef );
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc );
	bool r_Verb( CScript &s, CTextConsole *pSrc ); // Execute command from script
	bool r_LoadVal( CScript &s );
	bool r_Load( CScript &s );
};

#endif	// _INC_CCLIENT_H
