#include "graysvr.h"
#include "network/network.h"
#include "network/send.h"

BYTE CClient::sm_xCompress_Buffer[MAX_BUFFER];	// static
CCompressTree CClient::sm_xComp;

/////////////////////////////////////////////////////////////////
// -CClient stuff.

int CClient::xCompress( BYTE * pOutput, const BYTE * pInput, int iLen ) // static
{
	// The game server will compress the outgoing data to the clients.
	return sm_xComp.Encode( pOutput, pInput, iLen );
}

bool	CClient::IsConnecting()
{
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
	m_iConnectType	= iType;
	if ( iType == CONNECT_GAME )
	{
		NetworkIn::HistoryIP *hist = &g_Network->getHistoryForIP(peer());
		hist->m_connecting--;
	}
}
	
//---------------------------------------------------------------------
// Push world display data to this client only.

bool CClient::addLoginErr(BYTE code)
{
	if ( code == PacketLoginError::Success )
		return true;

	g_Log.Error("%x:Bad Login %d\n", socketId(), code);
	PacketLoginError *cmd = new PacketLoginError((PacketLoginError::Reason)code, this);
	return false;
}


void CClient::addSysMessage(LPCTSTR pszMsg) // System message (In lower left corner)
{
	if ( !pszMsg || !*pszMsg )
		return;

	//	flood protection
	if ( GetPrivLevel() <= PLEVEL_Player )
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
	// Direct client to a web page
	if ( !pPage || !pPage[0] )
		return;

	CCommand cmd;
	cmd.Web.m_Cmd = XCMD_Web;
	int iLen = sizeof(cmd.Web) + strlen(pPage);
	cmd.Web.m_len = iLen;
	strcpy( cmd.Web.m_page, pPage );
	xSend( &cmd, iLen );
}

///////////////////////////////////////////////////////////////
// Login server.

bool CClient::Login_Relay(int iRelay) // Relay player to a selected IP
{
	// Client wants to be relayed to another server.
	// iRelay = 0 = this local server.

	CServerDef *pServ = &g_Serv;
	if ( iRelay > 1 )
		pServ = g_Cfg.Server_GetDef(iRelay - 2);

	if ( !pServ )
	{
#ifdef DEBUGPACKETS
		g_Log.Error("%x:Login - bad relay index %d!\n", socketId(), iRelay);
#endif
		return false;
	}

	DWORD dwAddr = pServ->m_ip.GetAddrIP();
	DWORD dwCustomerId = Calc_GetRandVal(INT_MAX - 1);

	CCommand cmd;
	cmd.Relay.m_Cmd = XCMD_Relay;
	cmd.Relay.m_ip[3] = ( dwAddr >> 24 ) & 0xFF;
	cmd.Relay.m_ip[2] = ( dwAddr >> 16 ) & 0xFF;
	cmd.Relay.m_ip[1] = ( dwAddr >> 8  ) & 0xFF;
	cmd.Relay.m_ip[0] = ( dwAddr	   ) & 0xFF;
	cmd.Relay.m_port = pServ->m_ip.GetPort();
	cmd.Relay.m_Account = dwCustomerId;

	GetAccount()->m_TagDefs.SetNum("customerid", dwCustomerId);
	m_net->m_sock.Send(&cmd, sizeof(cmd.Relay));

	m_Crypt.InitFast(UNPACKDWORD(cmd.Relay.m_ip), CONNECT_GAME); // Init decryption table
	SetConnectType(m_Crypt.GetConnectType());
	m_Targ_Mode = CLIMODE_SETUP_RELAY;

	return true;
}

BYTE CClient::Login_ServerList( const char * pszAccount, const char * pszPassword )
{
	// XCMD_ServersReq
	// Initial login (Login on "loginserver", new format)
	// If the messages are garbled make sure they are terminated to correct length.

	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	int iLenAccount = Str_GetBare( szAccount, pszAccount, sizeof(szAccount)-1 );
	if ( iLenAccount > MAX_ACCOUNT_NAME_SIZE )
		return PacketLoginError::Other;
	if ( iLenAccount != strlen(pszAccount))
		return PacketLoginError::Other;

	TCHAR szPassword[MAX_NAME_SIZE+3];
	int iLenPassword = Str_GetBare( szPassword, pszPassword, sizeof( szPassword )-1 );
	if ( iLenPassword > MAX_NAME_SIZE )
		return PacketLoginError::Other;
	if ( iLenPassword != strlen(pszPassword))
		return PacketLoginError::Other;

	// don't bother logging in yet.
	// Give the server list to everyone.
	CGString sMsg;
	BYTE lErr = PacketLoginError::Other;

	lErr = LogIn( pszAccount, pszPassword, sMsg );
	
	if ( lErr != PacketLoginError::Success )
	{
		if ( lErr != PacketLoginError::Other )
		{
			addLoginErr(lErr);
		}
		return( lErr );
	}

	CCommand cmd;
	cmd.ServerList.m_Cmd = XCMD_ServerList;

	int indexoffset = 2;

	// always list myself first here.
	g_Serv.addToServersList( cmd, indexoffset-1, 0 );

	//	too many servers in list can crash the client
	int j = 1;
	int maxServers = config.get("login.servers.max");
	for ( int i=0; j < maxServers; i++ )
	{
		CServerDef *pServ = g_Cfg.Server_GetDef(i);
		if ( pServ == NULL )
			break;
		pServ->addToServersList( cmd, i+indexoffset, j );
		j++;
	}

	int iLen = sizeof(cmd.ServerList) - sizeof(cmd.ServerList.m_serv) + ( j * sizeof(cmd.ServerList.m_serv[0]));
	cmd.ServerList.m_len = iLen;
	cmd.ServerList.m_count = j;
	cmd.ServerList.m_unk3 = 0xFF;
	xSend( &cmd, iLen );

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	return PacketLoginError::Success;
}

//*****************************************

bool CClient::OnRxConsoleLoginComplete()
{
	if ( GetConnectType() != CONNECT_TELNET )
		return false;

 	if ( GetPrivLevel() < PLEVEL_Admin )	// this really should not happen.
	{
		SysMessage("Sorry you don't have admin level access\n");
		return false;
	}

	if ( !peer().IsValidAddr() )
		return false;

	SysMessagef( "Welcome to the Remote Admin Console '%s','%s'\n", GetName(), speer());
	return true;
}

bool CClient::OnRxConsole( const BYTE * pData, int iLen )
{
	// A special console version of the client. (Not game protocol)
	if ( !iLen || ( GetConnectType() != CONNECT_TELNET ))
		return false;

	while ( iLen -- )
	{
		int iRet = OnConsoleKey( m_Targ_Text, *pData++, GetAccount() != NULL );
		if ( ! iRet )
			return false;
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
						SysMessage("This account does not exist or is not privileged to log in via telnet.");
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
			}
			if ( ! iRet )
				return false;
		}
	}
	return true;
}

bool CClient::OnRxWebPageRequest( BYTE * pRequest, int iLen )
{
	// Seems to be a web browser pointing at us ? typical stuff :
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

	pRequest[iLen] = 0;
	if ( strlen((char*)pRequest) > 1024 )			// too long request
		return false;

	TCHAR	*ppLines[16];
	int iQtyLines = Str_ParseCmds((TCHAR*)pRequest, ppLines, COUNTOF(ppLines), "\r\n");
	if (( iQtyLines < 1 ) || ( iQtyLines >= 15 ))	// too long request
		return false;

	// Look for what they want to do with the connection.
	bool	fKeepAlive = false;
	TCHAR	*pszReferer = NULL;
	int		iContentLength = 0;
	for ( int j = 1; j < iQtyLines; j++ )
	{
		TCHAR	*pszArgs = ppLines[j];
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
			iContentLength = Exp_GetSingle(pszArgs);
		}
	}

	TCHAR	*ppRequest[4];
	int iQtyArgs = Str_ParseCmds((TCHAR*)ppLines[0], ppRequest, COUNTOF(ppRequest), " ");
	if (( iQtyArgs < 2 ) || ( strlen(ppRequest[1]) > 256 ) || strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0x0c) )
		return false;

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	m_net->m_sock.SetSockOpt(SO_LINGER, (char*)&llinger, sizeof(struct linger));
	BOOL nbool = true;
	m_net->m_sock.SetSockOpt(SO_KEEPALIVE, &nbool, sizeof(BOOL));

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

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Post '%s'\n",
			socketId(), (LPCTSTR)ppRequest[1]);

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

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Request '%s', alive=%d\n",
			socketId(), (LPCTSTR)ppRequest[1], fKeepAlive);
		if ( CWebPageDef::ServPage(this, ppRequest[1]) )
			return fKeepAlive;
	}
	return false;
}

void CClient::xProcessClientSetup(CEvent *pEvent, int iLen)
{
	// If this is a login then try to process the data and figure out what client it is.
	// either (CEvent::ServersReq) or (CEvent::CharListReq)
	CEvent bincopy;
	memcpy(bincopy.m_Raw, pEvent->m_Raw, iLen);

	if ( !m_Crypt.Init(m_net->m_seed, bincopy.m_Raw, iLen) )
	{
#ifdef DEBUGPACKETS
		g_Log.Warn("%x:Odd login message (len=%d).\n", socketId(), iLen);
#endif
		addLoginErr(PacketLoginError::Other);
		return;
	}
	
	SetConnectType(m_Crypt.GetConnectType());
	if ( !xCanEncLogin() || (m_Crypt.GetConnectType() == CONNECT_LOGIN && !xCanEncLogin(true)) )
	{
		addLoginErr(PacketLoginError::Other);
		return;
	}
	
	BYTE lErr = PacketLoginError::Other;
	m_Crypt.Decrypt(pEvent->m_Raw, bincopy.m_Raw, iLen);
	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	
	if ( pEvent->Default.m_Cmd == XCMD_ServersReq )
	{
		if ( iLen >= sizeof(pEvent->ServersReq) )
		{
			lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
			if ( lErr == PacketLoginError::Success )
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
					lErr = PacketLoginError::Other;
				}
			}
		}
	}
	else if ( pEvent->Default.m_Cmd == XCMD_CharListReq )
	{
		if ( iLen >= sizeof( pEvent->CharListReq ))
		{
			lErr = Setup_ListReq(pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true);
			if ( lErr == PacketLoginError::Success )
			{
				// pass detected client version to the game server to make valid cliver used
				int iLenAccount = Str_GetBare( szAccount, pEvent->CharListReq.m_acctname, sizeof(szAccount)-1 );
				CAccountRef pAcc = g_Accounts.Account_Find( szAccount );
				if (pAcc)
				{
					DWORD tmVer = pAcc->m_TagDefs.GetKeyNum("clientversion");
					DWORD tmSid = pAcc->m_TagDefs.GetKeyNum("customerid");
					if (  tmSid == pEvent->CharListReq.m_Account )
					{
						if ( tmVer != 1 )
						{
							m_Crypt.SetClientVerEnum(tmVer, false);
							m_net->m_clientVersion = tmVer;
						}
							
						if ( !xCanEncLogin(true) )
							lErr = PacketLoginError::Other;
					}
					else
						lErr = PacketLoginError::Other;

					pAcc->m_TagDefs.DeleteKey("clientversion");
					pAcc->m_TagDefs.DeleteKey("customerid");
				}
				else
					lErr = PacketLoginError::Other;
			}
		}
	}
	
	if ( lErr == PacketLoginError::Other )
	{
		addLoginErr(lErr);
	}
}

bool CClient::xCanEncLogin(bool bCheckCliver)
{
	if ( !bCheckCliver )
	{
		return ( m_Crypt.GetEncryptionType() != ENC_NONE );
	}
	else return true;
}

void CClient::xSend( const void *pData, int length)
{
	PacketGeneric *packet = new PacketGeneric((BYTE*)pData, length, this);
}
