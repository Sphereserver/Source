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
	SERV_STAT_MEM,		// virtual
	SERV_STAT_QTY
};

enum ACCAPP_TYPE	// types of new account applications.
{
	ACCAPP_Closed = 0,	// 0=Closed. Not accepting more.
	ACCAPP_Unused1,
	ACCAPP_Free,		// 2=Anyone can just log in and create a full account.
	ACCAPP_GuestAuto,	// You get to be a guest and are automatically sent email with u're new password.
	ACCAPP_GuestTrial,	// You get to be a guest til u're accepted for full by an Admin.
	ACCAPP_Unused5,
	ACCAPP_Unspecified,	// Not specified.
	ACCAPP_Unused7,
	ACCAPP_Unused8,
	ACCAPP_QTY
};

class CServerDef : public CScriptObj
{
	static LPCTSTR const sm_szLoadKeys[];

private:
	CGString m_sName;	// What the name should be. Fill in from ping.
	CServTime  m_timeLastValid;	// Last valid poll time in CServTime::GetCurrentTime()
	CGTime	m_dateLastValid;

	CServTime  m_timeCreate;	// When added to the list ? 0 = at start up.

	// Status read from returned string.
	CGString m_sStatus;	// last returned status string.

	// statistics
	DWORD m_dwStat[ SERV_STAT_QTY ];

public:
	static const char *m_sClassName;
	CSocketAddress m_ip;	// socket and port.
	CCrypt m_ClientVersion;

	// Breakdown the string. or filled in locally.
	signed char m_TimeZone;	// Hours from GMT. +5=EST
	CGString m_sEMail;		// Admin email address.
	CGString m_sURL;			// URL for the server.
	CGString m_sRestAPIPublicIP;	// REST API URL used to set server public IP on ServIP automatically at server startup
	CGString m_sLang;
	ACCAPP_TYPE m_eAccApp;	// types of new account applications.

public:
	CServerDef( LPCTSTR pszName, CSocketAddressIP dwIP );

private:
	CServerDef(const CServerDef& copy);
	CServerDef& operator=(const CServerDef& other);

public:
	LPCTSTR GetStatus() const
	{
		return(m_sStatus);
	}

	DWORD StatGet( SERV_STAT_TYPE i ) const;

	void StatInc( SERV_STAT_TYPE i )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_dwStat[i]++;
	}
	void StatDec( SERV_STAT_TYPE i )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_dwStat[i]--;
	}
	void SetStat( SERV_STAT_TYPE i, DWORD dwVal )
	{
		ASSERT( i>=0 && i<SERV_STAT_QTY );
		m_dwStat[i] = dwVal;
	}

	LPCTSTR GetName() const { return( m_sName ); }
	void SetName( LPCTSTR pszName );

	virtual INT64 GetAgeHours() const;

	bool IsSame( const CServerDef * pServNew ) const
	{
		UNREFERENCED_PARAMETER(pServNew);
		return true;
	}

	void SetValidTime();
	INT64 GetTimeSinceLastValid() const;

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString &sVal, CTextConsole * pSrc = NULL );

	bool IsConnected() const
	{
		return( m_timeLastValid.IsTimeValid() );
	}
};

#endif // _INC_CSERVERDEF_H
