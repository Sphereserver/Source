#ifndef _INC_CPARTY_H
#define _INC_CPARTY_H
#pragma once

class CPartyDef : public CGObListRec, public CScriptObj
{
	// List of characters in party
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];

	CPartyDef(CChar *pCharInviter, CChar *pCharAccept);

private:
	CCharRefArray m_Chars;
	CGString m_sName;
	CGString m_pSpeechFunction;
	CVarDefMap m_TagDefs;

private:
	bool SendMemberMsg(CChar *pCharDest, PacketSend *pPacket);
	void SendAll(PacketSend *pPacket);

	bool SetMaster(CChar *pChar);
	CChar *GetMaster() const
	{
		return m_Chars.GetChar(0).CharFind();
	}

	// List manipulation
	size_t AttachChar(CChar *pChar);
	size_t DetachChar(CChar *pChar);

public:
	static bool AcceptEvent(CChar *pCharAccept, CChar *pCharInviter, bool fForced = false);
	static bool DeclineEvent(CChar *pCharDecline, CChar *pCharInviter);

	// Functions sent to all party members
	void StatsUpdateAll(CChar *pCharSrc, PacketSend *pPacket);
	void SysMessageAll(LPCTSTR pszMsg);
	void UpdateWaypointAll(CChar *pCharSrc, MAPWAYPOINT_TYPE type);

	// List sending wrappers
	bool SendRemoveList(CChar *pCharRemove);
	bool SendAddList(CChar *pCharDest);

	// Party message sending wrappers
	bool MessageEvent(CChar *pCharDest, CChar *pCharSrc, const NCHAR *pszMsg);

	// Commands
	bool Disband();
	bool RemoveMember(CChar *pChar, CChar *pCharSrc = NULL);
	void AcceptMember(CChar *pChar);
	void SetLootFlag(CChar *pChar, bool fSet);
	bool GetLootFlag(const CChar *pChar);

	bool IsPartyFull() const
	{
		return (m_Chars.GetCharCount() >= 10);
	}
	bool IsInParty(const CChar *pChar) const
	{
		return m_Chars.IsCharIn(pChar);
	}
	bool IsPartyMaster(const CChar *pChar) const
	{
		return (m_Chars.FindChar(pChar) == 0);
	}

	LPCTSTR GetName() const
	{
		return static_cast<LPCTSTR>(m_sName);
	}

	bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script
	bool r_LoadVal(CScript &s);
	bool r_Load(CScript &s);

private:
	CPartyDef(const CPartyDef &copy);
	CPartyDef &operator=(const CPartyDef &other);
};

#endif	// _INC_CPARTY_H
