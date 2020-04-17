#ifndef _INC_CSERVREF_H
#define _INC_CSERVREF_H
#pragma once

enum SERV_STAT_TYPE
{
	SERV_STAT_CLIENTS,	// How many clients connected (use as % full)
	SERV_STAT_CHARS,
	SERV_STAT_ITEMS,
	SERV_STAT_ACCOUNTS,
	SERV_STAT_MEM,		// Virtual
	SERV_STAT_QTY
};

class CServerDef : public CScriptObj
{
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];

	CServerDef(LPCTSTR pszName, CSocketAddressIP dwIP);

private:
	DWORD m_dwStat[SERV_STAT_QTY];	// Statistic counters
	CGString m_sName;				// Server name
	CServTime m_timeCreate;			// Server creation time

public:
	CSocketAddress m_ip;			// Server IP/port
	CCrypt m_ClientVersion;			// Server client version
	signed char m_TimeZone;			// Server timezone (GMT based)
	CGString m_sEMail;				// Server admin email address
	CGString m_sURL;				// Server URL
	CGString m_sLang;				// Server language
	CGString m_sRestAPIPublicIP;	// REST API URL used to set server public IP on ServIP automatically at server startup

public:
	DWORD StatGet(SERV_STAT_TYPE i) const;
	void StatInc(SERV_STAT_TYPE i)
	{
		ASSERT((i >= SERV_STAT_CLIENTS) && (i < SERV_STAT_QTY));
		++m_dwStat[i];
	}
	void StatDec(SERV_STAT_TYPE i)
	{
		ASSERT((i >= SERV_STAT_CLIENTS) && (i < SERV_STAT_QTY));
		--m_dwStat[i];
	}

	void SetName(LPCTSTR pszName);
	LPCTSTR GetName() const
	{
		return m_sName;
	}

	virtual INT64 GetAge() const;

	bool IsSame(const CServerDef *pServ) const
	{
		UNREFERENCED_PARAMETER(pServ);
		return true;
	}

	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pKey, CGString &sVal, CTextConsole *pSrc = NULL);

private:
	CServerDef(const CServerDef &copy);
	CServerDef &operator=(const CServerDef &other);
};

#endif // _INC_CSERVREF_H
