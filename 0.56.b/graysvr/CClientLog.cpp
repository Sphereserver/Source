//
// CClientLog.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Login and low level stuff for the client.
//

#include "graysvr.h"	// predef header.

BYTE CClient::sm_xCompress_Buffer[MAX_BUFFER];	// static
CHuffman CClient::m_Comp;

/////////////////////////////////////////////////////////////////
// -CClient stuff.

int CClient::xCompress( BYTE * pOutput, const BYTE * pInput, int iLen ) // static
{
	ADDTOCALLSTACK("CClient::xCompress");
	// The game server will compress the outgoing data to the clients.
	return m_Comp.Compress( pOutput, pInput, iLen );
}

CLogIP * CClient::GetLogIP() const
{
	ADDTOCALLSTACK("CClient::GetLogIP");
	if ( ! m_PeerName.IsValidAddr())	// can't get ip ? why ?
		return( NULL );
	return g_Cfg.FindLogIP( m_PeerName, true );
}

bool	CClient::IsConnecting()
{
	ADDTOCALLSTACK("CClient::IsConnecting");
	switch ( GetConnectType() )
	{
	case CONNECT_TELNET:
	case CONNECT_HTTP:
	case CONNECT_GAME:
		return false;
	}
	return true;
}


void	CClient::SetConnectType( CONNECT_TYPE iType )
{
	ADDTOCALLSTACK("CClient::SetConnectType");
	m_iConnectType	= iType;
	if ( iType == CONNECT_GAME )
		UpdateLogIPConnecting( false );
}
	

int	CClient::GetLogIPConnecting() const
{
	ADDTOCALLSTACK("CClient::GetLogIPConnecting");
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return 0;
	return pLogIP->m_iConnecting;
}


int	CClient::GetLogIPConnected() const
{
	ADDTOCALLSTACK("CClient::GetLogIPConnected");
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return 0;
	return pLogIP->m_iConnected;
}


void	CClient::UpdateLogIPConnecting( bool fIncrease )
{
	ADDTOCALLSTACK("CClient::UpdateLogIPConnecting");
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return;
	if ( fIncrease )
		pLogIP->m_iConnecting++;
	else if ( pLogIP->m_iConnecting > 0 )
		pLogIP->m_iConnecting--;

}


void CClient::UpdateLogIPConnected( bool fIncrease )
{
	ADDTOCALLSTACK("CClient::UpdateLogIPConnected");
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return;
	if ( fIncrease )
		pLogIP->m_iConnected++;
	else if ( pLogIP->m_iConnected > 0 )
		pLogIP->m_iConnected--;
}


bool CClient::IsBlockedIP() const
{
	ADDTOCALLSTACK("CClient::IsBlockedIP");
	CLogIP * pLogIP;

	for ( int i=0; i < g_Cfg.m_LogIP.GetCount(); i++ )
	{
		pLogIP = g_Cfg.m_LogIP[i];

		if ( pLogIP->IsSameIP( m_PeerName ) )		// checked below
			continue;

		if ( pLogIP->IsMatchIP( m_PeerName ) )
		{
			if ( pLogIP->IsBlocked() )
				return true;
		}
	}

	pLogIP = GetLogIP();
	if ( pLogIP == NULL )
		return( true );
	return( pLogIP->CheckPingBlock( false ));
}

//---------------------------------------------------------------------
// Push world display data to this client only.

bool CClient::addLoginErr( LOGIN_ERR_TYPE code )
{
	ADDTOCALLSTACK("CClient::addLoginErr");
	// code
	// 0 = no account
	// 1 = account used.
	// 2 = blocked.
	// 3 = no password
	// LOGIN_ERR_OTHER

	if ( code == LOGIN_SUCCESS )
		return( true );

	static LPCTSTR const sm_Login_ErrMsg[] =
	{
		"No account",
		"Account already in use",
		"Account blocked",
		"Bad password",
		"Timeout / Wrong encryption"
	};
	
	DEBUG_ERR(( "%x:Bad Login %d (%s)\n", m_Socket.GetSocket(), code, sm_Login_ErrMsg[((int)code)] ));
	CCommand cmd;
	cmd.LogBad.m_Cmd = XCMD_LogBad;
	cmd.LogBad.m_code = code;
	xSendPkt( &cmd, sizeof( cmd.LogBad ));
	xFlush();
	m_fClosed	= true;
	return( false );
}


void CClient::addSysMessage(LPCTSTR pszMsg) // System message (In lower left corner)
{
	ADDTOCALLSTACK("CClient::addSysMessage");
	if ( !pszMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && ( GetPrivLevel() <= PLEVEL_Player )  )
	{
		if ( !strcmpi(pszMsg, m_zLastMessage) )
			return;

		if ( strlen(pszMsg) < SCRIPT_MAX_LINE_LEN )
			strcpy(m_zLastMessage, pszMsg);
	}

	addBarkParse(pszMsg, NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM, FONT_NORMAL);
}


void CClient::addWebLaunch( LPCTSTR pPage )
{
	ADDTOCALLSTACK("CClient::addWebLaunch");
	// Direct client to a web page
	if ( !pPage || !pPage[0] )
		return;

	CCommand cmd;
	cmd.Web.m_Cmd = XCMD_Web;
	int iLen = sizeof(cmd.Web) + strlen(pPage);
	cmd.Web.m_len = iLen;
	strcpy( cmd.Web.m_page, pPage );
	xSendPkt( &cmd, iLen );
}

///////////////////////////////////////////////////////////////
// Login server.

bool CClient::addRelay( const CServerDef * pServ )
{
	ADDTOCALLSTACK("CClient::addRelay");
	EXC_TRY("addRelay");

	// Tell the client to play on this server.
	if ( !pServ )
		return false;

	CSocketAddressIP ipAddr = pServ->m_ip;

	if ( ipAddr.IsLocalAddr())	// local server address not yet filled in.
	{
		ipAddr = m_Socket.GetSockName();
		DEBUG_MSG(( "%x:Login_Relay to %s\n", m_Socket.GetSocket(), ipAddr.GetAddrStr() ));
	}

	if ( m_PeerName.IsLocalAddr() || m_PeerName.IsSameIP( ipAddr ))	// weird problem with client relaying back to self.
	{
		DEBUG_MSG(( "%x:Login_Relay loopback to server %s\n", m_Socket.GetSocket(), ipAddr.GetAddrStr() ));
		ipAddr.SetAddrIP( SOCKET_LOCAL_ADDRESS );
	}

	EXC_SET("customer id");
	DWORD dwAddr = ipAddr.GetAddrIP();
	DWORD dwCustomerId = 0x7f000001;
	if ( g_Cfg.m_fUseAuthID )
	{
		dwCustomerId = Calc_GetRandVal(INT_MAX - 1);
		GetAccount()->m_TagDefs.SetNum("customerid", dwCustomerId);
	}

	DEBUG_MSG(( "%x:Login_Relay to server %s with AuthId %d\n", m_Socket.GetSocket(), ipAddr.GetAddrStr(), dwCustomerId ));

	EXC_SET("server relay packet");
	CCommand cmd;
	cmd.Relay.m_Cmd = XCMD_Relay;
	cmd.Relay.m_ip[3] = ( dwAddr >> 24 ) & 0xFF;
	cmd.Relay.m_ip[2] = ( dwAddr >> 16 ) & 0xFF;
	cmd.Relay.m_ip[1] = ( dwAddr >> 8  ) & 0xFF;
	cmd.Relay.m_ip[0] = ( dwAddr	   ) & 0xFF;
	cmd.Relay.m_port = pServ->m_ip.GetPort();
	cmd.Relay.m_Account = dwCustomerId; // customer account handshake. (it was 0x7f000001)

	xSendPkt( &cmd, sizeof(cmd.Relay));
	xFlush();	// flush b4 we turn into a game server.

	m_Targ_Mode = CLIMODE_SETUP_RELAY;

	EXC_SET("fast init encryption");
	// just in case they are on the same machine, change over to the new game encrypt
	m_Crypt.InitFast( UNPACKDWORD( cmd.Relay.m_ip ), CONNECT_GAME ); // Init decryption table
	SetConnectType( m_Crypt.GetConnectType() );
	
	return( true );
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("account '%s'\n", GetAccount() ? GetAccount()->GetName() : "");
	EXC_DEBUG_END;
	return( false );
}

bool CClient::Login_Relay( int iRelay ) // Relay player to a selected IP
{
	ADDTOCALLSTACK("CClient::Login_Relay");
	// Client wants to be relayed to another server. XCMD_ServerSelect
	// iRelay = 0 = this local server.

	// >= 1.26.00 clients list Gives us a 1 based index for some reason.
	iRelay --;

	CServerRef pServ;
	if ( iRelay <= 0 )
	{
		pServ = &g_Serv;	// we always list ourself first.
	}
	else
	{
		iRelay --;
		pServ = g_Cfg.Server_GetDef(iRelay);
		if ( pServ == NULL )
		{
			DEBUG_ERR(( "%x:Login_Relay BAD index! %d\n", m_Socket.GetSocket(), iRelay ));
			return( false );
		}
	}

	return addRelay( pServ );
}

LOGIN_ERR_TYPE CClient::Login_ServerList( const char * pszAccount, const char * pszPassword )
{
	ADDTOCALLSTACK("CClient::Login_ServerList");
	// XCMD_ServersReq
	// Initial login (Login on "loginserver", new format)
	// If the messages are garbled make sure they are terminated to correct length.

	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	int iLenAccount = Str_GetBare( szAccount, pszAccount, sizeof(szAccount)-1 );
	if ( iLenAccount > MAX_ACCOUNT_NAME_SIZE )
		return( LOGIN_ERR_OTHER );
	if ( iLenAccount != strlen(pszAccount))
		return( LOGIN_ERR_OTHER );

	TCHAR szPassword[MAX_NAME_SIZE+3];
	int iLenPassword = Str_GetBare( szPassword, pszPassword, sizeof( szPassword )-1 );
	if ( iLenPassword > MAX_NAME_SIZE )
		return( LOGIN_ERR_OTHER );
	if ( iLenPassword != strlen(pszPassword))
		return( LOGIN_ERR_OTHER );

	// don't bother logging in yet.
	// Give the server list to everyone.
	// if ( LogIn( pszAccount, pszPassword ) )
	//   return( LOGIN_ERR_BAD_PASS );
	CGString sMsg;
	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;

	lErr = LogIn( pszAccount, pszPassword, sMsg );
	
	if ( lErr != LOGIN_SUCCESS )
	{
		if (  lErr != LOGIN_ERR_OTHER )
		{
			addLoginErr(lErr);
		}
		return( lErr );
	}

	CCommand cmd;
	cmd.ServerList.m_Cmd = XCMD_ServerList;

	int indexoffset = 2; // Client older than 1.26.00 --> 1;

	// clients before 4.0.0 require serverlist ips to be in reverse
	bool bReverse = (m_Crypt.GetClientVer() < 0x400000);

	// always list myself first here.
	g_Serv.addToServersList( cmd, indexoffset-1, 0, bReverse );

	//	too many servers in list can crash the client
#define	MAX_SERVERS_LIST	32

	int j = 1;
	for ( int i=0; j < MAX_SERVERS_LIST; i++ )
	{
		CServerRef pServ = g_Cfg.Server_GetDef(i);
		if ( pServ == NULL )
			break;
		pServ->addToServersList( cmd, i+indexoffset, j, bReverse );
		j++;
	}

	int iLen = sizeof(cmd.ServerList) - sizeof(cmd.ServerList.m_serv) + ( j * sizeof(cmd.ServerList.m_serv[0]));
	cmd.ServerList.m_len = iLen;
	cmd.ServerList.m_count = j;
	cmd.ServerList.m_unk3 = 0xFF;
	xSendPkt( &cmd, iLen );

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	return( LOGIN_SUCCESS );
}

//*****************************************

bool CClient::OnRxConsoleLoginComplete()
{
	ADDTOCALLSTACK("CClient::OnRxConsoleLoginComplete");
	if ( GetConnectType() != CONNECT_TELNET )
		return false;

 	if ( GetPrivLevel() < PLEVEL_Admin )	// this really should not happen.
	{
		SysMessagef("%s\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NO_ADMIN));
		return false;
	}

	if ( ! m_PeerName.IsValidAddr())
		return( false );

	SysMessagef( "%s '%s','%s'\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_2), GetName(), m_PeerName.GetAddrStr());
	return( true );
}

bool CClient::OnRxConsole( const BYTE * pData, int iLen )
{
	ADDTOCALLSTACK("CClient::OnRxConsole");
	// A special console version of the client. (Not game protocol)
	if ( !iLen || ( GetConnectType() != CONNECT_TELNET ))
		return false;

	if ( IsSetEF( EF_AllowTelnetPacketFilter ) )
	{
		bool fFiltered = xPacketFilter( (const CEvent *)pData, iLen );
		if ( fFiltered )
			return fFiltered;
	}

	while ( iLen -- )
	{
		int iRet = OnConsoleKey( m_Targ_Text, *pData++, GetAccount() != NULL );
		if ( ! iRet )
			return( false );
		if ( iRet == 2 )
		{
			if ( GetAccount() == NULL )
			{
				if ( !m_zLogin[0] )
				{
					if ( m_Targ_Text.GetLength() > sizeof(m_zLogin)-1 ) SysMessage("Login?:\n");
					else
					{
						strcpy(m_zLogin, m_Targ_Text);
						SysMessage("Password?:\n");
					}
					m_Targ_Text.Empty();
				}
				else
				{
					CGString sMsg;

					CAccountRef pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == NULL ) || ( pAccount->GetPrivLevel() < PLEVEL_Admin ))
					{
						SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NOT_PRIV));
						m_Targ_Text.Empty();
						return false;
					}
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == LOGIN_SUCCESS )
					{
						m_Targ_Text.Empty();
						return OnRxConsoleLoginComplete();
					}
					else if ( ! sMsg.IsEmpty())
					{
						SysMessage( sMsg );
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
			else
			{
				iRet = g_Serv.OnConsoleCmd( m_Targ_Text, this );
			}
			if ( ! iRet )
				return false;
		}
	}
	return true;
}

bool CClient::OnRxPing( const BYTE * pData, int iLen )
{
	ADDTOCALLSTACK("CClient::OnRxPing");
	// packet iLen < 5
	// UOMon should work like this.
	// RETURN: true = keep the connection open.
	if ( GetConnectType() != CONNECT_UNK )
		return false;

	if ( !iLen || iLen > 4 )
		return false;

	switch ( pData[0] )
	{
		// Remote Admin Console
		case '\x1':
		case ' ':
		{
			if ( iLen > 1 )
				break;

			// enter into remote admin mode. (look for password).
			SetConnectType( CONNECT_TELNET );
			m_zLogin[0] = 0;
			SysMessagef( "%s %s Admin Telnet\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_1), (LPCTSTR) g_Serv.GetName());

			if ( g_Cfg.m_fLocalIPAdmin )
			{
				// don't bother logging in if local.

				if ( m_PeerName.IsLocalAddr() )
				{
					CAccountRef pAccount = g_Accounts.Account_Find("Administrator");
					if ( !pAccount )
						pAccount = g_Accounts.Account_Find("RemoteAdmin");
					if ( pAccount )
					{
						CGString sMsg;
						LOGIN_ERR_TYPE lErr = LogIn( pAccount, sMsg );
						if ( lErr != LOGIN_SUCCESS )
						{
							if ( lErr != LOGIN_ERR_NONE )
								SysMessage( sMsg );
							return( false );
						}
						return OnRxConsoleLoginComplete();
					}
				}
			}

			SysMessage("Login?:\n");
			return true;
		}

		// ConnectUO Status
		case 0xF1:
		{
			// ConnectUO sends a 4-byte packet when requesting status info
			// BYTE Cmd		(0xF1)
			// WORD Unk		(0x04)
			// BYTE SubCmd	(0xFF)

			if ( iLen != MAKEWORD( pData[2], pData[1] ) )
				break;

			if ( pData[3] != 0xFF )
				break;

			// enter 'remote admin mode'
			SetConnectType( CONNECT_TELNET );

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:CUO Status request from %s\n", m_Socket.GetSocket(), (LPCTSTR) m_PeerName.GetAddrStr());

			SysMessage( g_Serv.GetStatusString( 0x25 ) );

			// exit 'remote admin mode'
			SetConnectType( CONNECT_UNK );
			return false;
		}

		// UOGateway Status
		case 0xFF:
		case 0x22:
		{
			if ( iLen > 1 )
				break;

			// enter 'remote admin mode'
			SetConnectType( CONNECT_TELNET );

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:UOG Status request from %s\n", m_Socket.GetSocket(), (LPCTSTR) m_PeerName.GetAddrStr());

			SysMessage( g_Serv.GetStatusString( 0x22 ) );

			// exit 'remote admin mode'
			SetConnectType( CONNECT_UNK );
			return false;
		}
	}

	g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:Unknown/invalid ping data '0x%x' from %s (Len: %d)\n", m_Socket.GetSocket(), pData[0], (LPCTSTR) m_PeerName.GetAddrStr(), iLen);
	return false;
}

bool CClient::OnRxWebPageRequest( BYTE * pRequest, int iLen )
{
	ADDTOCALLSTACK("CClient::OnRxWebPageRequest");
	// Seems to be a web browser pointing at us ? typical stuff :
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

	pRequest[iLen] = 0;
	if ( strlen((char*)pRequest) > 1024 )			// too long request
		return false;

	if ( !strpbrk( (char*)pRequest, " \t\012\015" ) )	// malformed request
		return false;

	TCHAR	*ppLines[16];
	int iQtyLines = Str_ParseCmds((TCHAR*)pRequest, ppLines, COUNTOF(ppLines), "\r\n");
	if (( iQtyLines < 1 ) || ( iQtyLines >= 15 ))	// too long request
		return false;

	// Look for what they want to do with the connection.
	bool	fKeepAlive = false;
	CGTime	dateIfModifiedSince;
	TCHAR	*pszReferer = NULL;
	int		iContentLength = 0;
	for ( int j = 1; j < iQtyLines; j++ )
	{
		TCHAR	*pszArgs = Str_TrimWhitespace(ppLines[j]);
		if ( !strnicmp(pszArgs, "Connection:", 11 ) )
		{
			if ( strstr(pszArgs + 11, "Keep-Alive") )
				fKeepAlive = true;
		}
		else if ( !strnicmp(pszArgs, "Referer:", 8) )
		{
			pszReferer = pszArgs+8;
		}
		else if ( !strnicmp(pszArgs, "Content-Length:", 15) )
		{
			pszArgs += 15;
			GETNONWHITESPACE(pszArgs);
			iContentLength = atol(pszArgs);
		}
		else if ( ! strnicmp( pszArgs, "If-Modified-Since:", 18 ))
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			dateIfModifiedSince.Read(pszArgs);
		}
	}

	TCHAR	*ppRequest[4];
	int iQtyArgs = Str_ParseCmds((TCHAR*)ppLines[0], ppRequest, COUNTOF(ppRequest), " ");
	if (( iQtyArgs < 2 ) || ( strlen(ppRequest[1]) >= _MAX_PATH ))
		return false;

	if ( strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0x0c) )
		return false;

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	m_Socket.SetSockOpt(SO_LINGER, (char*)&llinger, sizeof(struct linger));
	BOOL nbool = true;
	m_Socket.SetSockOpt(SO_KEEPALIVE, &nbool, sizeof(BOOL));

	// disable NAGLE algorythm for data compression
	nbool=true;
	m_Socket.SetSockOpt( TCP_NODELAY,&nbool,sizeof(BOOL),IPPROTO_TCP);
	
	if ( !memcmp(ppLines[0], "POST", 4) )
	{
		if ( iContentLength > strlen(ppLines[iQtyLines-1]) )
			return false;

		// POST /--WEBBOT-SELF-- HTTP/1.1
		// Referer: http://127.0.0.1:2593/spherestatus.htm
		// Content-Type: application/x-www-form-urlencoded
		// Host: 127.0.0.1:2593
		// Content-Length: 29
		// T1=stuff1&B1=Submit&T2=stuff2

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Post '%s'\n", m_Socket.GetSocket(), (LPCTSTR)ppRequest[1]);

		CWebPageDef	*pWebPage = g_Cfg.FindWebPage(ppRequest[1]);
		if ( !pWebPage )
			pWebPage = g_Cfg.FindWebPage(pszReferer);
		if ( pWebPage )
		{
			if ( pWebPage->ServPagePost(this, ppRequest[1], ppLines[iQtyLines-1], iContentLength) )
				return fKeepAlive;
			return false;
		}
	}
	else if ( !memcmp(ppLines[0], "GET", 3) )
	{
		// GET /pagename.htm HTTP/1.1\r\n
		// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
		// Host: localhost:2593\r\n
		// \r\n

		TCHAR szPageName[_MAX_PATH];
		if ( !Str_GetBare( szPageName, Str_TrimWhitespace(ppRequest[1]), sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`" ) )
			return false;

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Request '%s', alive=%d\n", m_Socket.GetSocket(), (LPCTSTR)szPageName, fKeepAlive);
		if ( CWebPageDef::ServPage(this, szPageName, &dateIfModifiedSince) )
			return fKeepAlive;
	}


	return false;
}

void CClient::xProcessMsg(int fGood)
{
	ADDTOCALLSTACK("CClient::xProcessMsg");
	// Done with the current packet.
	// m_bin_msg_len = size of the current packet we are processing.

	if ( !m_bin_msg_len )	// hmm, nothing to do !
		return;

	if ( fGood < 1 )	// toss all.
	{
		if ( !fGood )
		{
#ifdef VJAKA_REDO
			DEBUG_ERR(("%s (%x):Bad Msg(%x) Eat %d bytes, prv=0%x, type=%d\n", m_pAccount ? m_pAccount->GetName() : "", 
					m_Socket.GetSocket(), m_bin_ErrMsg, m_bin.bytes(), m_bin_PrvMsg, GetConnectType() ));
#else
			DEBUG_ERR(("%s (%x):Bad Msg(%x) Eat %d bytes, prv=0%x, type=%d\n", m_pAccount ? m_pAccount->GetName() : "", 
					m_Socket.GetSocket(), m_bin_ErrMsg, m_bin.GetDataQty(), m_bin_PrvMsg, GetConnectType() ));
#endif

#ifdef _DEBUG
			xDumpPacket(m_bin.GetDataQty(), m_bin.RemoveDataLock());
#endif
		}
#ifdef VJAKA_REDO
		m_bin.empty();
#else
		m_bin.Empty();	// eat the buffer.
#endif
		if ( GetConnectType() == CONNECT_LOGIN )	// tell them about it.
		{
			addLoginErr(LOGIN_ERR_OTHER);
		}
	}
	else
	{
#ifdef VJAKA_REDO
		m_bin.remove(m_bin_msg_len);
#else
		m_bin.RemoveDataAmount(m_bin_msg_len);
#endif
	}

	m_bin_msg_len = 0;
}

bool CClient::xProcessClientSetup( CEvent * pEvent, int iLen )
{
	ADDTOCALLSTACK("CClient::xProcessClientSetup");
	// If this is a login then try to process the data and figure out what client it is.
	// try to figure out which client version we are talking to.
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed !

	ASSERT( GetConnectType() == CONNECT_CRYPT );
	ASSERT( !m_Crypt.IsInit());
	ASSERT( iLen );

#if _DEBUG
	DEBUG_ERR(("CClient::xProcessClientSetup\n"));
	xDumpPacket(iLen, (const BYTE *)pEvent);
#endif
	
	// Try all client versions on the msg.
	CEvent bincopy;		// in buffer. (from client)
	ASSERT( iLen <= sizeof(bincopy));
	memcpy( bincopy.m_Raw, pEvent->m_Raw, iLen );

	if ( !m_Crypt.Init( m_tmSetup.m_dwIP, bincopy.m_Raw, iLen ) )
	{
		DEBUG_MSG(( "%x:Odd login message length %d?\n", m_Socket.GetSocket(), iLen ));
		addLoginErr( LOGIN_ERR_OTHER );
		return( false );
	}
	
	SetConnectType( m_Crypt.GetConnectType() );
	
	if ( !xCanEncLogin() || (m_Crypt.GetConnectType() == CONNECT_LOGIN && !xCanEncLogin(true)) )
	{
		addLoginErr( LOGIN_ERR_OTHER );
		return( false );
	}

	if ( IsBlockedIP())	// we are a blocked ip so i guess it does not matter.
	{
		addLoginErr( LOGIN_ERR_BLOCKED );
		return( false );
	}


	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;
	
	m_Crypt.Decrypt( pEvent->m_Raw, bincopy.m_Raw, iLen );
	
	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	
	if ( pEvent->Default.m_Cmd == XCMD_ServersReq )
	{
		if ( iLen < sizeof( pEvent->ServersReq ))
			return(false);

		lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
		if ( lErr == LOGIN_SUCCESS )
		{
			int iLenAccount = Str_GetBare( szAccount, pEvent->ServersReq.m_acctname, sizeof(szAccount)-1 );
			CAccountRef pAcc = g_Accounts.Account_Find( szAccount );
			if (pAcc)
			{
				pAcc->m_TagDefs.SetNum("clientversion", m_Crypt.GetClientVer());
			}
			else
			{
				// If i can't set the tag is better to stop login now
				lErr = LOGIN_ERR_OTHER;
			}
		}
	}
	else if ( pEvent->Default.m_Cmd == XCMD_CharListReq )
	{
		if ( iLen < sizeof( pEvent->CharListReq ))
			return(false);

		lErr = Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true );
		if ( lErr == LOGIN_SUCCESS )
		{
			// pass detected client version to the game server to make valid cliver used
			int iLenAccount = Str_GetBare( szAccount, pEvent->CharListReq.m_acctname, sizeof(szAccount)-1 );
			CAccountRef pAcc = g_Accounts.Account_Find( szAccount );
			if (pAcc)
			{
				DWORD tmVer = pAcc->m_TagDefs.GetKeyNum("clientversion"); pAcc->m_TagDefs.DeleteKey("clientversion");
				DWORD tmSid = 0x7f000001;
				if ( g_Cfg.m_fUseAuthID )
				{
					tmSid = pAcc->m_TagDefs.GetKeyNum("customerid");
					pAcc->m_TagDefs.DeleteKey("customerid");
				}

				DEBUG_MSG(( "%x:xProcessClientSetup for %s, with AuthId %d and CliVersion 0x%x\n", m_Socket.GetSocket(), 
					pAcc->GetName(), tmSid, tmVer ));

				if ( tmSid != NULL && tmSid == pEvent->CharListReq.m_Account )
				{
					if ( tmVer != NULL )
						m_Crypt.SetClientVerEnum(tmVer, false);

					if ( !xCanEncLogin(true) )
						lErr = LOGIN_ERR_OTHER;
				}
				else
				{
					lErr = LOGIN_ERR_OTHER;
				}
			}
			else
			{
				lErr = LOGIN_ERR_OTHER;
			}
		}
	}
#ifdef __UOKRSCARYADDONS
	else if (pEvent->Default.m_Cmd == 0xE4)
	{
		// Lets just ignore it and try and move on? :E
		lErr = LOGIN_SUCCESS;
	}
#endif
#if _DEBUG
	else
	{
		DEBUG_ERR(("Unknown/bad packet to receive at this time: 0x%X\n", pEvent->Default.m_Cmd));
	}
#endif
	
	if ( lErr == LOGIN_ERR_OTHER )	// it never matched any crypt format.
	{
		addLoginErr( lErr );
	}

	return( lErr == LOGIN_SUCCESS );
}

bool CClient::xCanEncLogin(bool bCheckCliver)
{
	ADDTOCALLSTACK("CClient::xCanEncLogin");
	if ( !bCheckCliver )
	{
		if ( m_Crypt.GetEncryptionType() == ENC_NONE )
			return ( g_Cfg.m_fUsenocrypt ); // Server don't want no-crypt clients 
		
		return ( g_Cfg.m_fUsecrypt ); // Server don't want crypt clients
	}
	else
	{
		if ( !g_Serv.m_ClientVersion.GetClientVer() ) // Any Client allowed
			return( true );
		
		if ( m_Crypt.GetEncryptionType() != ENC_NONE )
			return( m_Crypt.GetClientVer() == g_Serv.m_ClientVersion.GetClientVer() );
		else
			return( true );	// if unencrypted we check that later
	}
}

void CClient::xSendPkt(const CCommand * pCmd, int length)
{
	ADDTOCALLSTACK("CClient::xSendPkt");
	xSendReady((const void *)( pCmd->m_Raw ), length );
}

void CClient::xSendPktNow( const CCommand * pCmd, int length )
{
	ADDTOCALLSTACK("CClient::xSendPktNow");
	const void *pData = (const void*)pCmd->m_Raw;

	xFlush();	// Flush old packets.
	
	// Add my packet
	if ( ! m_Socket.IsOpen() )
		return;

	if ( GetConnectType() != CONNECT_HTTP )	// acting as a login server to this client.
	{
		if ( length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out TOO BIG %d!\n", m_Socket.GetSocket(), length ));
			m_fClosed	= true;
			return;
		}
#ifdef VJAKA_REDO
		if ( m_bout.bytes() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!\n", m_Socket.GetSocket(), m_bout.bytes(), length ));
#else
		if ( m_bout.GetDataQty() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!\n", m_Socket.GetSocket(), m_bout.GetDataQty(), length ));
#endif
			m_fClosed	= true;
			return;
		}
	}

#ifdef VJAKA_REDO
	m_bout.append((BYTE *)pData, length);
#else
	m_bout.AddNewData((const BYTE*) pData, length);
#endif
	xFlush();
}

void CClient::xProcessTooltipQueue()
{
	ADDTOCALLSTACK("CClient::xProcessTooltipQueue");
	if ( m_TooltipQueue.empty() )
		return;

	long llCurrentTick = g_World.GetCurrentTime().GetTimeRaw();

	if ( m_LastTooltipSend == llCurrentTick )
		return;

	if ( (llCurrentTick % 2) != 1 )
		return;

	std::vector<CTooltipData *>::iterator i = m_TooltipQueue.begin();
	int iMaxTooltipPerTick = g_Cfg.m_iMaxTooltipForTick;
	CTooltipData * pTemp = NULL;
	CChar * pChar = GetChar();
	if ( !pChar )
		return;

	while ( i != m_TooltipQueue.end() )
	{
		bool bTooltipSent = false;
		pTemp = (*i);

		// Check that the object is still valid and that the client
		// can still actually see it
		if ( pTemp && pTemp->IsObjectValid() && ( pChar->GetTopDist( pTemp->GetObject()->GetTopLevelObj() ) <= UO_MAP_VIEW_SIZE ) )
		{
			DEBUG_MSG(("Sending tooltip for 0%x (%d)\n", pTemp->GetObjUid(), llCurrentTick));

			// Don't bother sending the tooltip if the client has
			// already been waiting for over 30 seconds for it
			if ( (pTemp->GetTime() + (TICK_PER_SEC * 30)) > llCurrentTick )
			{
				xSend(pTemp->GetData(), pTemp->GetLength(), false);
				bTooltipSent = true;
			}
		}

		m_TooltipQueue.erase(i);
		i = m_TooltipQueue.begin();

		if ( pTemp )
			delete pTemp;

		// This tooltip was not valid, don't count it towards the limit
		if ( !bTooltipSent )
			continue;

		if ( --iMaxTooltipPerTick == 0 )
			break;
	}

	m_LastTooltipSend = llCurrentTick;
}

void CClient::xFlush()
{
	ADDTOCALLSTACK("CClient::xFlush");
	// Sends buffered data at once
	// NOTE:
	// Make sure we do not overflow the Sockets Tx buffers!

#ifdef VJAKA_REDO
	int iLen = m_bout.bytes();
#else
	int iLen = m_bout.GetDataQty();
#endif
	if ( !iLen || !m_Socket.IsOpen() || m_fClosed )
		return;

	m_timeLastSend = CServTime::GetCurrentTime();

	int iLenRet;
	if ( GetConnectType() != CONNECT_GAME )	// acting as a login server to this client.
	{
#ifdef VJAKA_REDO
		iLenRet = m_Socket.Send( m_bout.raw(), iLen );
#else
		iLenRet = m_Socket.Send( m_bout.RemoveDataLock(), iLen );
#endif
		if ( iLenRet != SOCKET_ERROR )
		{
			// Tx overflow may be handled gracefully.
			g_Serv.m_Profile.Count( PROFILE_DATA_TX, iLenRet );
#ifdef VJAKA_REDO
			m_bout.remove(iLenRet);
#else
			m_bout.RemoveDataAmount(iLenRet);
#endif
		}
		else	
		{
			// Assume nothing was transmitted.
Do_Handle_Error:
			int iErrCode = CGSocket::GetLastError();
#ifdef _WIN32
			if ( iErrCode == WSAECONNRESET || iErrCode == WSAECONNABORTED )
			{
				m_fClosed	= true;
				return;
			}
			if ( iErrCode == WSAEWOULDBLOCK )
			{
				// just try back later. or select() will close it for us later.
				return;
			}
#endif
			DEBUG_ERR(( "%x:Tx Error %d\n", m_Socket.GetSocket(), iErrCode ));

//To fix the crash bug on linux we will disconnect all clients with broken socket
#ifndef _WIN32
			//CharDisconnect();
			m_fClosed=true;
#endif
			return;
		}
	}
	else
	{
		// Only the game server does this.
		// This acts as a compression alg. tho it may expand the data some times.

#ifdef VJAKA_REDO
		int iLenComp = xCompress( sm_xCompress_Buffer, m_bout.raw(), iLen );
#else
		int iLenComp = xCompress( sm_xCompress_Buffer, m_bout.RemoveDataLock(), iLen );
#endif
		ASSERT( iLenComp <= sizeof(sm_xCompress_Buffer));

		// This now works. Thx to Necr0 post and library, i've understood how to do it.
		// ( MD5 server -> client )
		if ( m_Crypt.GetEncryptionType() == ENC_TFISH )
		{
			m_Crypt.Encrypt( sm_xCompress_Buffer, sm_xCompress_Buffer, iLenComp );
		}

		iLenRet = m_Socket.Send( sm_xCompress_Buffer, iLenComp );
		if ( iLenRet != SOCKET_ERROR )
		{
			g_Serv.m_Profile.Count( PROFILE_DATA_TX, iLen );
#ifdef VJAKA_REDO
			m_bout.remove(iLen);
#else
			m_bout.RemoveDataAmount(iLen);	// must use all of it since we have no idea what was really sent.
#endif
		}

		if ( iLenRet != iLenComp )
		{
			// Tx overflow is not allowed here !
			// no idea what effect this would have. assume nothing is sent.
			goto Do_Handle_Error;
		}
	}
}

void CClient::xSend( const void *pData, int length, bool bQueue)
{
	ADDTOCALLSTACK("CClient::xSend");
	// buffer a packet to client.
	if ( ! m_Socket.IsOpen() )
		return;

	if ( GetConnectType() != CONNECT_HTTP )	// acting as a login server to this client.
	{
		if ( length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out TOO BIG %d!\n", m_Socket.GetSocket(), length ));
			m_fClosed	= true;
			return;
		}
#ifdef VJAKA_REDO
		if ( m_bout.bytes() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!\n", m_Socket.GetSocket(), m_bout.bytes(), length ));
#else
		if ( m_bout.GetDataQty() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!\n", m_Socket.GetSocket(), m_bout.GetDataQty(), length ));
#endif
			m_fClosed	= true;
			return;
		}
	}

#ifdef VJAKA_REDO
	m_bout.append((BYTE*)pData, length);
#else
	m_bout.AddNewData( (const BYTE*) pData, length );
#endif

	if ( GetConnectType() == CONNECT_LOGIN ) // During login we flush always, so we have no problem with any client version
	{
		if ( !bQueue )
			xFlush();
	}
	else if ( GetConnectType() == CONNECT_GAME )
	{
		if ( IsClientVer( 0x400000 ) || IsNoCryptVer( 0x400000 ) )
			if ( !bQueue )
				xFlush();
	}
}

void CClient::xSendReady( const void *pData, int length, bool bNextFlush ) // We could send the packet now if we wanted to but wait til we have more.
{
	ADDTOCALLSTACK("CClient::xSendReady");
	// We could send the packet now if we wanted to but wait til we have more.
#ifdef VJAKA_REDO
	if ( m_bout.bytes() + length >= MAX_BUFFER )
#else
	if ( m_bout.GetDataQty() + length >= MAX_BUFFER )
#endif
	{
		xFlush();
	}
//	DEBUG_ERR(("SEND: %x:adding %d bytes\n", m_Socket.GetSocket(), length));
	xSend( pData, length );
#ifdef VJAKA_REDO
	if ( bNextFlush && ( m_bout.bytes() >= MAX_BUFFER / 2 ) )	// send only if we have a bunch.
#else
	if ( bNextFlush && ( m_bout.GetDataQty() >= MAX_BUFFER / 2 ) )	// send only if we have a bunch.
#endif
	{
		xFlush();
	}
}

bool CClient::xRecvData() // Receive message from client
{
	ADDTOCALLSTACK("CClient::xRecvData");
	// High level Rx from Client.
	// RETURN: false = dump the client.
	CEvent Event;
	int iCountNew = m_Socket.Receive( &Event, sizeof(Event), 0 );

	if ( iCountNew <= 0 )	// I should always get data here.
		return( false ); // this means that the client is gone.

#if _DEBUG
	DEBUG_ERR(("CClient::xRecvData RAW\n"));
	xDumpPacket(iCountNew, Event.m_Raw);
#endif

	g_Serv.m_Profile.Count( PROFILE_DATA_RX, iCountNew );

	if ( GetConnectType() == CONNECT_UNK ) // first thing
	{
		// This is the first data we get on a new connection.
		// Figure out what the other side wants.

		if ( iCountNew < 4 )	// just a ping for server info. (maybe, or CONNECT_TELNET?)
		{
			if ( IsBlockedIP())
				return( false );
			return( OnRxPing( Event.m_Raw, iCountNew ));
		}

		if ( iCountNew > 5 )
		{
			// Is it a HTTP request ?
			// Is it HTTP post ?
			if ( ! memcmp( Event.m_Raw, "POST /", 6 ) ||
				! memcmp( Event.m_Raw, "GET /", 5 ))
			{
				if ( g_Cfg.m_fUseHTTP != 2 )
					return( false );

				// IsBlockedIP
				SetConnectType( CONNECT_HTTP );	// we are serving web pages to this client.

				CLogIP * pLogIP = GetLogIP();
				if ( pLogIP == NULL )
					return( false );
				if ( pLogIP->CheckPingBlock( false ))
					return( false );

				// We might have an existing account connection.
				// ??? Is this proper to connect this way ?
				m_pAccount = pLogIP->GetAccount();

				return( OnRxWebPageRequest( Event.m_Raw, iCountNew ));
			}
		}

		// Assume it's a normal client log in.
		m_tmSetup.m_dwIP = UNPACKDWORD(Event.m_Raw);
		iCountNew -= sizeof( DWORD );
		SetConnectType( CONNECT_CRYPT );
		if ( iCountNew <= 0 )
		{
#ifdef __UOKRSCARYADDONS
			if (m_tmSetup.m_dwIP == 0xFFFFFFFF)
			{
				// UOKR Client opens connection with 255.255.255.255
				DEBUG_ERR(("UOKR Client Detected.\n"));
				BYTE pData[77] = {	0xe3,
									0x00, 0x4d,
									0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x03,
									0x00, 0x00, 0x00, 0x13, 0x02, 0x11, 0x00, 0xfc, 0x2f, 0xe3, 0x81, 0x93, 0xcb, 0xaf, 0x98, 0xdd, 0x83, 0x13, 0xd2, 0x9e, 0xea, 0xe4, 0x13,
									0x00, 0x00, 0x00, 0x10, 0x78, 0x13, 0xb7, 0x7b, 0xce, 0xa8, 0xd7, 0xbc, 0x52, 0xde, 0x38, 0x30, 0xea, 0xe9, 0x1e, 0xa3,
									0x00, 0x00, 0x00, 0x20,
									0x00, 0x00, 0x00, 0x10, 0x5a, 0xce, 0x3e, 0xe3, 0x97, 0x92, 0xe4, 0x8a, 0xf1, 0x9a, 0xd3, 0x04, 0x41, 0x03, 0xcb, 0x53 };

				/*
				// Randomise 0x10 content (client won't accept)
				for (int i = 0; i < 0x10; i++)
				{
					pData[37+i] = Calc_GetRandVal(0xFF);
					pData[61+i] = Calc_GetRandVal(0xFF);
				}

				xDumpPacket(77, pData);
				*/

				/*
				// Causes client to respond with 0s in 0xE4
				BYTE pData2[77] = {0xe3,
									0x00, 0x4d,
									0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x03,
									0x00, 0x00, 0x00, 0x13, 0x02, 0x11, 0x00, 0xfc, 0x2f, 0xe3, 0x81, 0x93, 0xcb, 0xaf, 0x98, 0xdd, 0x83, 0x13, 0xd2, 0x9e, 0xea, 0xe4, 0x13,
									0x00, 0x00, 0x00, 0x10, 0x00, 0x13, 0xb7, 0x7b, 0xce, 0xa8, 0xd7, 0xbc, 0x52, 0xde, 0x38, 0x30, 0xea, 0xe9, 0x1e, 0xa3,
									0x00, 0x00, 0x00, 0x20,
									0x00, 0x00, 0x00, 0x10, 0x00, 0xce, 0x3e, 0xe3, 0x97, 0x92, 0xe4, 0x8a, 0xf1, 0x9a, 0xd3, 0x04, 0x41, 0x03, 0xcb, 0x53 };
				*/
				/*
				// Attempt to zero as much as possible
				BYTE pData3[77] = {0xe3,
									0x00, 0x4d,
									0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x03,
									0x00, 0x00, 0x00, 0x13, 0x02, 0x11, 0x00, 0x00, 0x2f, 0xe3, 0x81, 0x93, 0xcb, 0xaf, 0x98, 0xdd, 0x83, 0x13, 0xd2, 0x9e, 0xea, 0xe4, 0x13,
									0x00, 0x00, 0x00, 0x10, 0x00, 0x13, 0xb7, 0x00, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x20,
									0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
				*/
				this->xSend(pData, 77);
			}
#endif
			return( true );
		}

		if ( iCountNew < 5 )
		{
			// Not enough data to be an actual client?
			SetConnectType( CONNECT_UNK );
			return( OnRxPing( Event.m_Raw+4, iCountNew ) );
		}

		return( xProcessClientSetup( (CEvent*)(Event.m_Raw+4), iCountNew ));
	}

	if ( ! m_Crypt.IsInit())
	{
		// This is not a client connection it seems.
		// Must process the whole thing as one packet right now.

		if ( GetConnectType() == CONNECT_CRYPT )
		{
			if ( iCountNew < 5 )
			{
				// Not enough data to be an actual client?
				SetConnectType( CONNECT_UNK );
				return( OnRxPing( Event.m_Raw, iCountNew ) );
			}

			// try to figure out which client version we are talking to.
			// (CEvent::ServersReq) or (CEvent::CharListReq)
			return( xProcessClientSetup( &Event, iCountNew ));
		}

		if ( GetConnectType() == CONNECT_HTTP )
		{
			// we are serving web pages to this client.
			return( OnRxWebPageRequest( Event.m_Raw, iCountNew ));
		}
		if ( GetConnectType() == CONNECT_TELNET )
		{
			// We already logged in or are in the process of logging in.
			return( OnRxConsole( Event.m_Raw, iCountNew ));
		}

		g_Log.Event( LOGM_CLIENTS_LOG, "xRecvData() %x:Junk with non inited Crypt\n", m_Socket.GetSocket());
		return( false );	// No idea what this junk is.
	}

	// Decrypt the client data.
	// TCP = no missed packets ! If we miss a packet we are screwed !

#ifdef VJAKA_REDO
	m_Crypt.Decrypt(m_bin.appendStart(iCountNew), Event.m_Raw, iCountNew );
	m_bin.appendFinish(iCountNew);
#else
	m_Crypt.Decrypt( m_bin.AddNewDataLock(iCountNew), Event.m_Raw, iCountNew );
	m_bin.AddNewDataFinish(iCountNew);
#endif

	return( true );
}


void CClient::xDumpPacket( int iDataLen, const BYTE * pData )
{
	ADDTOCALLSTACK("CClient::xDumpPacket");

	char lineBytes[64] = { '\0' };
	char lineChars[64] = { '\0' };
	int i = 0;

	g_Log.EventDebug("Packet Dump for 0x%0.2X (len=%d):\n", pData[0], iDataLen);

	for (i = 0; i < iDataLen; i++)
	{
		sprintf(lineBytes, "%s%0.2X ", lineBytes, pData[i]);
		sprintf(lineChars, "%s%c", lineChars, (pData[i]? (pData[i]<0xF? '?':pData[i]):'.'));

		if (((i + 1) % 0x10) == 0)
		{
			g_Log.EventDebug("%s--- %s\n", lineBytes, lineChars);
			lineBytes[0] = '\0';
			lineChars[0] = '\0';
		}

	}

	if (lineBytes[0] != '\0')
	{
		for ( ; (i % 0x10) != 0; i++)
		{
			sprintf(lineBytes, "%s   ", lineBytes);
			sprintf(lineChars, "%s ", lineChars);
		}

		g_Log.EventDebug("%s--- %s\n", lineBytes, lineChars);
	}

	g_Log.EventDebug("-------------\n");
}
