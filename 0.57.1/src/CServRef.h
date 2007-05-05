//
// CServerDef.h
//

#ifndef _INC_CSERVERDEF_H
#define _INC_CSERVERDEF_H
#pragma once

enum SERV_STAT_TYPE
{
	SERV_STAT_CLIENTS,	// How many clients does it say it has ? (use as % full)
	SERV_STAT_CHARS,
	SERV_STAT_ITEMS,
	SERV_STAT_ACCOUNTS,
	SERV_STAT_QTY,
	SERV_STAT_MEM = SERV_STAT_QTY,	// virtual
};

class CServerDef : public CScriptObj
{
	static LPCTSTR const sm_szLoadKeys[];

private:
	CGString	m_sName;					// What the name should be. Fill in from ping.
	CServTime	m_timeCreate;				// When added to the list ? 0 = at start up.
	DWORD		m_dwStat[SERV_STAT_QTY];	// statistics

public:
	static const char *m_sClassName;
	CSocketAddress m_ip;	// socket and port.

	// Breakdown the string. or filled in locally.
	signed char m_TimeZone;	// Hours from GMT. +5=EST
	CGString m_sEMail;		// Admin email address.
	CGString m_sURL;			// URL for the server.
	CGString m_sLang;

public:
	CServerDef( LPCTSTR pszName, CSocketAddressIP dwIP );

	DWORD StatGet( SERV_STAT_TYPE i ) const;

	void StatInc( SERV_STAT_TYPE i )
	{
		m_dwStat[i]++;
	}
	void StatDec( SERV_STAT_TYPE i )
	{
		m_dwStat[i]--;
	}
	void SetStat( SERV_STAT_TYPE i, DWORD dwVal )
	{
		m_dwStat[i] = dwVal;
	}

	LPCTSTR GetName() const { return( m_sName ); }
	void SetName( LPCTSTR pszName );

	virtual int GetAgeHours() const;

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString &sVal, CTextConsole * pSrc = NULL );

	void addToServersList( CCommand & cmd, int iThis, int jArray, bool bReverse ) const;
};

#endif // _INC_CSERVERDEF_H