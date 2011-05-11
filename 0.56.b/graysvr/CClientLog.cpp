//
// CClientLog.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Login and low level stuff for the client.
//

#include "graysvr.h"	// predef header.
#include "../network/network.h"
#include "../network/send.h"
#pragma warning(disable:4096)
#include "../common/zlib/zlib.h"
#pragma warning(default:4096)

CHuffman CClient::m_Comp;
#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

/////////////////////////////////////////////////////////////////
// -CClient stuff.

size_t CClient::xCompress( BYTE * pOutput, const BYTE * pInput, size_t iLen ) // static
{
	ADDTOCALLSTACK("CClient::xCompress");
	// The game server will compress the outgoing data to the clients.
	return m_Comp.Compress( pOutput, pInput, iLen );
}

bool CClient::IsConnecting() const
{
	ADDTOCALLSTACK("CClient::IsConnecting");
	switch ( GetConnectType() )
	{
		case CONNECT_TELNET:
		case CONNECT_HTTP:
		case CONNECT_GAME:
			return false;

		default:
			return true;
	}
}


void CClient::SetConnectType( CONNECT_TYPE iType )
{
	ADDTOCALLSTACK("CClient::SetConnectType");
	m_iConnectType = iType;
	if ( iType == CONNECT_GAME )
	{
#ifndef _MTNETWORK
		HistoryIP& history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
		HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
		history.m_connecting--;
	}
}

//---------------------------------------------------------------------
// Push world display data to this client only.

bool CClient::addLoginErr(BYTE code)
{
	ADDTOCALLSTACK("CClient::addLoginErr");
	// code
	// 0 = no account
	// 1 = account used.
	// 2 = blocked.
	// 3 = no password
	// LOGIN_ERR_OTHER

	if (code == PacketLoginError::Success)
		return( true );

	// console message to display for each login error code
	static LPCTSTR const sm_Login_ErrMsg[] =
	{
		"Account does not exist",
		"The account entered is already being used",
		"This account or IP is blocked",
		"The password entered is not correct",
		"Timeout / Wrong encryption / Unknown error",
		"Invalid client version. See the CLIENTVERSION setting in " GRAY_FILE ".ini",
		"Invalid character selected (chosen character does not exist)",
		"AuthID is not correct. This normally means that the client did not log in via the login server",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"Encryption error (packet length does not match what was expected)",
		"Encryption error (unknown encryption or bad login packet)",
		"Encrypted client not permitted. See the USECRYPT setting in " GRAY_FILE ".ini",
		"Unencrypted client not permitted. See the USENOCRYPT setting in " GRAY_FILE ".ini",
		"Another character on this account is already ingame",
		"Account is full. Cannot create a new character",
		"This IP is blocked",
		"The maximum number of clients has been reached. See the CLIENTMAX setting in " GRAY_FILE ".ini",
		"The maximum number of guests has been reached. See the GUESTSMAX setting in " GRAY_FILE ".ini",
		"The maximum number of password tries has been reached",
	};

	if (code >= COUNTOF(sm_Login_ErrMsg))
		code = PacketLoginError::Other;
	
	DEBUG_ERR(( "%lx:Bad Login %d (%s)\n", GetSocketID(), code, sm_Login_ErrMsg[((int)code)] ));

	// translate the code into a code the client will understand
	switch (code)
	{
		case PacketLoginError::Invalid:
			code = PacketLoginError::Invalid;
			break;
		case PacketLoginError::InUse:
		case PacketLoginError::CharIdle:
			code = PacketLoginError::InUse;
			break;
		case PacketLoginError::Blocked:
		case PacketLoginError::BlockedIP:
		case PacketLoginError::MaxClients:
		case PacketLoginError::MaxGuests:
			code = PacketLoginError::Blocked;
			break;
		case PacketLoginError::BadPass:
		case PacketLoginError::BadAccount:
		case PacketLoginError::BadPassword:
			code = PacketLoginError::BadPass;
			break;
		case PacketLoginError::Other:
		case PacketLoginError::BadVersion:
		case PacketLoginError::BadCharacter:
		case PacketLoginError::BadAuthID:
		case PacketLoginError::BadEncLength:
		case PacketLoginError::EncCrypt:
		case PacketLoginError::EncNoCrypt:
		case PacketLoginError::TooManyChars:
		case PacketLoginError::MaxPassTries:
		case PacketLoginError::EncUnknown:
		default:
			code = PacketLoginError::Other;
			break;
	}

	new PacketLoginError(this, (PacketLoginError::Reason)code);
	GetNetState()->markReadClosed();
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

	new PacketWebPage(this, pPage);
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
		ipAddr = m_net->m_socket.GetSockName();
		DEBUG_MSG(( "%lx:Login_Relay to %s\n", GetSocketID(), ipAddr.GetAddrStr() ));
	}

	if ( GetPeer().IsLocalAddr() || GetPeer().IsSameIP( ipAddr ))	// weird problem with client relaying back to self.
	{
		DEBUG_MSG(( "%lx:Login_Relay loopback to server %s\n", GetSocketID(), ipAddr.GetAddrStr() ));
		ipAddr.SetAddrIP( SOCKET_LOCAL_ADDRESS );
	}

	EXC_SET("customer id");
	DWORD dwAddr = ipAddr.GetAddrIP();
	DWORD dwCustomerId = 0x7f000001;
	if ( g_Cfg.m_fUseAuthID )
	{
		CGString sCustomerID(pServ->GetName());
		sCustomerID.Add(GetAccount()->GetName());

		dwCustomerId = z_crc32(0L, Z_NULL, 0);
		dwCustomerId = z_crc32(dwCustomerId, (const z_Bytef *)sCustomerID.GetPtr(), sCustomerID.GetLength());

		GetAccount()->m_TagDefs.SetNum("customerid", dwCustomerId);
	}

	DEBUG_MSG(( "%lx:Login_Relay to server %s with AuthId %lu\n", GetSocketID(), ipAddr.GetAddrStr(), dwCustomerId ));

	EXC_SET("server relay packet");
	new PacketServerRelay(this, dwAddr, pServ->m_ip.GetPort(), dwCustomerId);
	
	m_Targ_Mode = CLIMODE_SETUP_RELAY;
	return( true );
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("account '%s'\n", GetAccount() ? GetAccount()->GetName() : "");
	EXC_DEBUG_END;
	return( false );
}

bool CClient::Login_Relay( unsigned int iRelay ) // Relay player to a selected IP
{
	ADDTOCALLSTACK("CClient::Login_Relay");
	// Client wants to be relayed to another server. XCMD_ServerSelect
	// iRelay = 0 = this local server.

	// Sometimes we get an extra 0x80 ???
	if ( iRelay >= 0x80 )
	{
		iRelay -= 0x80;
	}

	// >= 1.26.00 clients list Gives us a 1 based index for some reason.
	if ( iRelay > 0 )
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
			DEBUG_ERR(( "%lx:Login_Relay BAD index! %u\n", GetSocketID(), iRelay ));
			return( false );
		}
	}

	return addRelay( pServ );
}

BYTE CClient::Login_ServerList( const char * pszAccount, const char * pszPassword )
{
	ADDTOCALLSTACK("CClient::Login_ServerList");
	// XCMD_ServersReq
	// Initial login (Login on "loginserver", new format)
	// If the messages are garbled make sure they are terminated to correct length.

	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	size_t iLenAccount = Str_GetBare( szAccount, pszAccount, sizeof(szAccount)-1 );
	if ( iLenAccount > MAX_ACCOUNT_NAME_SIZE )
		return( PacketLoginError::BadAccount );
	if ( iLenAccount != strlen(pszAccount))
		return( PacketLoginError::BadAccount );

	TCHAR szPassword[MAX_NAME_SIZE+3];
	size_t iLenPassword = Str_GetBare( szPassword, pszPassword, sizeof( szPassword )-1 );
	if ( iLenPassword > MAX_NAME_SIZE )
		return( PacketLoginError::BadPassword );
	if ( iLenPassword != strlen(pszPassword))
		return( PacketLoginError::BadPassword );

	// don't bother logging in yet.
	// Give the server list to everyone.
	// if ( LogIn( pszAccount, pszPassword ) )
	//   return( PacketLoginError::BadPass );
	CGString sMsg;
	BYTE lErr = LogIn( pszAccount, pszPassword, sMsg );
	if ( lErr != PacketLoginError::Success )
	{
		return( lErr );
	}

	new PacketServerList(this);

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	return( PacketLoginError::Success );
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

	if ( ! GetPeer().IsValidAddr())
		return( false );

	SysMessagef( "%s '%s','%s'\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_2), GetName(), GetPeerStr());
	return( true );
}

bool CClient::OnRxConsole( const BYTE * pData, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxConsole");
	// A special console version of the client. (Not game protocol)
	if ( !iLen || ( GetConnectType() != CONNECT_TELNET ))
		return false;

	if ( IsSetEF( EF_AllowTelnetPacketFilter ) )
	{
		bool fFiltered = xPacketFilter(pData, iLen);
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
					if ( m_Targ_Text.GetLength() > (sizeof(m_zLogin) - 1) )
					{
						SysMessage("Login?:\n");
					}
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
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == PacketLoginError::Success )
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

				if (g_Cfg.m_fTelnetLog && GetPrivLevel() >= g_Cfg.m_iCommandLog)
					g_Log.Event(LOGM_GM_CMDS, "%lx:'%s' commands '%s'=%d\n", GetSocketID(), (LPCTSTR)GetName(), (LPCTSTR)m_Targ_Text, iRet);
			}
		}
	}
	return true;
}

bool CClient::OnRxPing( const BYTE * pData, size_t iLen )
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
			if ( (iLen > 1) &&
				 (iLen != 2 || pData[1] != '\n') &&
				 (iLen != 3 || pData[1] != '\r' || pData[2] != '\n') &&
				 (iLen != 3 || pData[1] != '\n' || pData[2] != '\0') )
				break;

			// enter into remote admin mode. (look for password).
			SetConnectType( CONNECT_TELNET );
			m_zLogin[0] = 0;
			SysMessagef( "%s %s Admin Telnet\n", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_1), (LPCTSTR) g_Serv.GetName());

			if ( g_Cfg.m_fLocalIPAdmin )
			{
				// don't bother logging in if local.

				if ( GetPeer().IsLocalAddr() )
				{
					CAccountRef pAccount = g_Accounts.Account_Find("Administrator");
					if ( !pAccount )
						pAccount = g_Accounts.Account_Find("RemoteAdmin");
					if ( pAccount )
					{
						CGString sMsg;
						BYTE lErr = LogIn( pAccount, sMsg );
						if ( lErr != PacketLoginError::Success )
						{
							if ( lErr != PacketLoginError::Invalid )
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

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:CUO Status request from %s\n", GetSocketID(), GetPeerStr());

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

			g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:UOG Status request from %s\n", GetSocketID(), GetPeerStr());

			SysMessage( g_Serv.GetStatusString( 0x22 ) );

			// exit 'remote admin mode'
			SetConnectType( CONNECT_UNK );
			return false;
		}
	}

	g_Log.Event( LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Unknown/invalid ping data '0x%x' from %s (Len: %" FMTSIZE_T ")\n", GetSocketID(), pData[0], GetPeerStr(), iLen);
	return false;
}

bool CClient::OnRxWebPageRequest( BYTE * pRequest, size_t iLen )
{
	ADDTOCALLSTACK("CClient::OnRxWebPageRequest");
	// Seems to be a web browser pointing at us ? typical stuff :
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

	// ensure request is null-terminated (if the request is well-formed, we are overwriting a trailing \n here)
	pRequest[iLen - 1] = '\0';

	if ( strlen(reinterpret_cast<char *>(pRequest)) > 1024 )			// too long request
		return false;

	if ( !strpbrk( reinterpret_cast<char *>(pRequest), " \t\012\015" ) )	// malformed request
		return false;

	TCHAR * ppLines[16];
	size_t iQtyLines = Str_ParseCmds(reinterpret_cast<char *>(pRequest), ppLines, COUNTOF(ppLines), "\r\n");
	if (( iQtyLines < 1 ) || ( iQtyLines >= 15 ))	// too long request
		return false;

	// Look for what they want to do with the connection.
	bool fKeepAlive = false;
	CGTime dateIfModifiedSince;
	TCHAR * pszReferer = NULL;
	size_t iContentLength = 0;
	for ( size_t j = 1; j < iQtyLines; j++ )
	{
		TCHAR	*pszArgs = Str_TrimWhitespace(ppLines[j]);
		if ( !strnicmp(pszArgs, "Connection:", 11 ) )
		{
			pszArgs += 11;
			GETNONWHITESPACE(pszArgs);
			if ( !strnicmp(pszArgs, "Keep-Alive", 10) )
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
			iContentLength = strtoul(pszArgs, NULL, 10);
		}
		else if ( ! strnicmp( pszArgs, "If-Modified-Since:", 18 ))
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			dateIfModifiedSince.Read(pszArgs);
		}
	}

	TCHAR	*ppRequest[4];
	size_t iQtyArgs = Str_ParseCmds((TCHAR*)ppLines[0], ppRequest, COUNTOF(ppRequest), " ");
	if (( iQtyArgs < 2 ) || ( strlen(ppRequest[1]) >= _MAX_PATH ))
		return false;

	if ( strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0x0c) )
		return false;

	// if the client hasn't requested a keep alive, we must act as if they had
	// when async networking is used, otherwise data may not be completely sent
	if ( fKeepAlive == false )
	{
		fKeepAlive = m_net->isAsyncMode();

		// must switch to a blocking socket when the connection is not being kept
		// alive, or else pending data will be lost when the socket shuts down

		if ( fKeepAlive == false )
			m_net->m_socket.SetNonBlocking(false);
	}

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	m_net->m_socket.SetSockOpt(SO_LINGER, (char*)&llinger, sizeof(linger));
	BOOL nbool = true;
	m_net->m_socket.SetSockOpt(SO_KEEPALIVE, &nbool, sizeof(BOOL));

	// disable NAGLE algorythm for data compression
	nbool = true;
	m_net->m_socket.SetSockOpt( TCP_NODELAY, &nbool, sizeof(BOOL), IPPROTO_TCP);
	
	if ( memcmp(ppLines[0], "POST", 4) == 0 )
	{
		if ( iContentLength > strlen(ppLines[iQtyLines-1]) )
			return false;

		// POST /--WEBBOT-SELF-- HTTP/1.1
		// Referer: http://127.0.0.1:2593/spherestatus.htm
		// Content-Type: application/x-www-form-urlencoded
		// Host: 127.0.0.1:2593
		// Content-Length: 29
		// T1=stuff1&B1=Submit&T2=stuff2

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%lx:HTTP Page Post '%s'\n", GetSocketID(), (LPCTSTR)ppRequest[1]);

		CWebPageDef	*pWebPage = g_Cfg.FindWebPage(ppRequest[1]);
		if ( !pWebPage )
			pWebPage = g_Cfg.FindWebPage(pszReferer);
		if ( pWebPage )
		{
			if ( pWebPage->ServPagePost(this, ppRequest[1], ppLines[iQtyLines-1], iContentLength) )
			{
				if ( fKeepAlive )
					return true;
				return false;
			}
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

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%lx:HTTP Page Request '%s', alive=%d\n", GetSocketID(), (LPCTSTR)szPageName, fKeepAlive);
		if ( CWebPageDef::ServPage(this, szPageName, &dateIfModifiedSince) )
		{
			if ( fKeepAlive )
				return true;
			return false;
		}
	}


	return false;
}

bool CClient::xProcessClientSetup( CEvent * pEvent, size_t iLen )
{
	ADDTOCALLSTACK("CClient::xProcessClientSetup");
	// If this is a login then try to process the data and figure out what client it is.
	// try to figure out which client version we are talking to.
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed !
	ASSERT( GetConnectType() == CONNECT_CRYPT );
	ASSERT( !m_Crypt.IsInit());
	ASSERT( pEvent != NULL );
	ASSERT( iLen > 0 );

	// Try all client versions on the msg.
	CEvent bincopy;		// in buffer. (from client)
	ASSERT( iLen <= sizeof(bincopy));
	memcpy( bincopy.m_Raw, pEvent->m_Raw, iLen );

	if ( !m_Crypt.Init( m_net->m_seed, bincopy.m_Raw, iLen, GetNetState()->isClientKR() ) )
	{
		DEBUG_MSG(( "%lx:Odd login message length %" FMTSIZE_T "?\n", GetSocketID(), iLen ));
#ifdef _DEBUG
		xRecordPacketData(this, (const BYTE *)pEvent, iLen, "client->server");
#endif
		addLoginErr( PacketLoginError::BadEncLength );
		return( false );
	}
	
	GetNetState()->detectAsyncMode();
	SetConnectType( m_Crypt.GetConnectType() );

	if ( !xCanEncLogin() )
	{
		addLoginErr((m_Crypt.GetEncryptionType() == ENC_NONE? PacketLoginError::EncNoCrypt : PacketLoginError::EncCrypt) );
		return( false );
	}
	else if ( m_Crypt.GetConnectType() == CONNECT_LOGIN && !xCanEncLogin(true) )
	{
		addLoginErr( PacketLoginError::BadVersion );
		return( false );
	}
	
	BYTE lErr = PacketLoginError::EncUnknown;
	
	m_Crypt.Decrypt( pEvent->m_Raw, bincopy.m_Raw, iLen );
	
	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];

	switch ( pEvent->Default.m_Cmd )
	{
		case XCMD_ServersReq:
		{
			if ( iLen < sizeof( pEvent->ServersReq ))
				return(false);

			lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
			if ( lErr == PacketLoginError::Success )
			{
				Str_GetBare( szAccount, pEvent->ServersReq.m_acctname, sizeof(szAccount)-1 );
				CAccountRef pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
					pAcc->m_TagDefs.SetNum("clientversion", m_Crypt.GetClientVer());
					pAcc->m_TagDefs.SetNum("reportedcliver", GetNetState()->getReportedVersion());
				}
				else
				{
					// If i can't set the tag is better to stop login now
					lErr = PacketLoginError::Invalid;
				}
			}

			break;
		}

		case XCMD_CharListReq:
		{
			if ( iLen < sizeof( pEvent->CharListReq ))
				return(false);

			lErr = Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true );
			if ( lErr == PacketLoginError::Success )
			{
				// pass detected client version to the game server to make valid cliver used
				Str_GetBare( szAccount, pEvent->CharListReq.m_acctname, sizeof(szAccount)-1 );
				CAccountRef pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
					DWORD tmVer = pAcc->m_TagDefs.GetKeyNum("clientversion"); pAcc->m_TagDefs.DeleteKey("clientversion");
					DWORD tmVerReported = pAcc->m_TagDefs.GetKeyNum("reportedcliver"); pAcc->m_TagDefs.DeleteKey("reportedcliver");
					DWORD tmSid = 0x7f000001;
					if ( g_Cfg.m_fUseAuthID )
					{
						tmSid = pAcc->m_TagDefs.GetKeyNum("customerid");
						pAcc->m_TagDefs.DeleteKey("customerid");
					}

					DEBUG_MSG(( "%lx:xProcessClientSetup for %s, with AuthId %lu and CliVersion 0x%lx\n", GetSocketID(), 
						pAcc->GetName(), tmSid, tmVer ));

					if ( tmSid != 0 && tmSid == pEvent->CharListReq.m_Account )
					{
						if ( tmVer != 0 || tmVerReported != 0)
						{
							//// a client version change may toggle async mode, it's important
							//// to flush pending data to the client before this happens
							if ( tmVer != 0 )
							{
								m_Crypt.SetClientVerEnum(tmVer, false);
								GetNetState()->m_clientVersion = tmVer;
							}

							if ( tmVerReported != 0)
								GetNetState()->m_reportedVersion = tmVerReported;

							GetNetState()->detectAsyncMode();
						}

						if ( !xCanEncLogin(true) )
							lErr = PacketLoginError::BadVersion;
					}
					else
					{
						lErr = PacketLoginError::BadAuthID;
					}
				}
				else
				{
					lErr = PacketLoginError::Invalid;
				}
			}

			break;
		}

#ifdef _DEBUG
		default:
		{
			DEBUG_ERR(("Unknown/bad packet to receive at this time: 0x%X\n", pEvent->Default.m_Cmd));
		}
#endif
	}
	
	xRecordPacketData(this, (const BYTE *)pEvent, iLen, "client->server");

	if ( lErr != PacketLoginError::Success )	// it never matched any crypt format.
	{
		addLoginErr( lErr );
	}

	return( lErr == PacketLoginError::Success );
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
