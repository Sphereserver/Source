#include "graysvr.h"	// predef header
#include "../network/send.h"
#include "../common/CFileList.h"
#include "../common/zlib/zlib.h"

CHuffman CClient::m_Comp;
#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

///////////////////////////////////////////////////////////
// CClient stuff

size_t CClient::xCompress(BYTE *pOutput, const BYTE *pInput, size_t iInputLen)	// static
{
	ADDTOCALLSTACK("CClient::xCompress");
	// The game server will compress the outgoing data to client
	return m_Comp.Compress(pOutput, pInput, iInputLen);
}

void CClient::SetConnectType(CONNECT_TYPE iType)
{
	ADDTOCALLSTACK("CClient::SetConnectType");
	m_iConnectType = iType;
	if ( iType == CONNECT_GAME )
	{
#ifndef _MTNETWORK
		HistoryIP &history = g_NetworkIn.getIPHistoryManager().getHistoryForIP(GetPeer());
#else
		HistoryIP &history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(GetPeer());
#endif
		--history.m_connecting;
	}
}

///////////////////////////////////////////////////////////
// Push world display data to this client only

bool CClient::addLoginErr(BYTE bCode)
{
	ADDTOCALLSTACK("CClient::addLoginErr");
	if ( bCode == PacketLoginError::Success )
		return true;

	// Console message to display for each login error code
	static LPCTSTR const sm_Login_ErrMsg[] =
	{
		"Account does not exist",
		"The account entered is already being used",
		"This account or IP is blocked",
		"The password entered is not correct",
		"Timeout / Wrong encryption / Unknown error",
		"Invalid client version. See the CLIENTVERSION setting in " SPHERE_FILE ".ini",
		"Invalid character selected (chosen character does not exist)",
		"AuthID is not correct. This normally means that the client did not log in via the login server",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"The account details entered are invalid (username or password is too short, too long or contains invalid characters). This can sometimes be caused by incorrect/missing encryption keys",
		"Encryption error (packet length does not match what was expected)",
		"Encryption error (unknown encryption or bad login packet)",
		"Encrypted client not permitted. See the USECRYPT setting in " SPHERE_FILE ".ini",
		"Unencrypted client not permitted. See the USENOCRYPT setting in " SPHERE_FILE ".ini",
		"Another character on this account is already ingame",
		"Account is full. Cannot create a new character",
		"Character creation blocked",
		"This IP is blocked",
		"The maximum number of clients has been reached. See the CLIENTMAX setting in " SPHERE_FILE ".ini",
		"The maximum number of guests has been reached. See the GUESTSMAX setting in " SPHERE_FILE ".ini",
		"The maximum number of password tries has been reached"
	};

	if ( bCode >= COUNTOF(sm_Login_ErrMsg) )
		bCode = PacketLoginError::Other;

	if ( g_Log.GetLogMask() & LOGM_CLIENTS_LOG )
		g_Log.EventWarn("%lx:Bad Login %d (%s)\n", GetSocketID(), bCode, sm_Login_ErrMsg[bCode]);

	if ( m_NetState->m_clientVersion || m_NetState->m_reportedVersion )		// only reply the packet to valid clients
	{
		// Translate the code into a value the client will understand
		switch ( bCode )
		{
			case PacketLoginError::Invalid:
				bCode = PacketLoginError::Invalid;
				break;
			case PacketLoginError::InUse:
			case PacketLoginError::CharIdle:
				bCode = PacketLoginError::InUse;
				break;
			case PacketLoginError::Blocked:
			case PacketLoginError::BlockedIP:
			case PacketLoginError::MaxClients:
			case PacketLoginError::MaxGuests:
				bCode = PacketLoginError::Blocked;
				break;
			case PacketLoginError::BadPass:
			case PacketLoginError::BadAccount:
			case PacketLoginError::BadPassword:
				bCode = PacketLoginError::BadPass;
				break;
			default:
				bCode = PacketLoginError::Other;
				break;
		}
		new PacketLoginError(this, static_cast<PacketLoginError::Reason>(bCode));
	}

	m_NetState->markReadClosed();
	return false;
}

void CClient::addSysMessage(LPCTSTR pszMsg)
{
	ADDTOCALLSTACK("CClient::addSysMessage");
	if ( !pszMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && (GetPrivLevel() <= PLEVEL_Player) )
	{
		if ( !strcmpi(pszMsg, m_zLastMessage) )
			return;

		if ( strlen(pszMsg) < SCRIPT_MAX_LINE_LEN )
			strcpy(m_zLastMessage, pszMsg);
	}

	addBarkParse(pszMsg, NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM);
}

void CClient::addWebLaunch(LPCTSTR pszURL)
{
	ADDTOCALLSTACK("CClient::addWebLaunch");
	// Direct client to a web page
	if ( !pszURL )
		return;

	SysMessageDefault(DEFMSG_WEB_BROWSER_START);
	new PacketWebPage(this, pszURL);
}

///////////////////////////////////////////////////////////
// Login server

bool CClient::addRelay(const CServerDef *pServ)
{
	ADDTOCALLSTACK("CClient::addRelay");
	// Tell the client to play on this server
	if ( !pServ )
		return false;

	EXC_TRY("addRelay");
	CSocketAddressIP ipAddr = pServ->m_ip;
	if ( ipAddr.IsLocalAddr() )		// local server address not yet filled in
	{
		ipAddr = m_NetState->m_socket.GetSockName();
		DEBUG_MSG(("%lx:Login_Relay to %s\n", GetSocketID(), ipAddr.GetAddrStr()));
	}
	if ( GetPeer().IsLocalAddr() || GetPeer().IsSameIP(ipAddr) )	// weird problem with client relaying back to self
	{
		DEBUG_MSG(("%lx:Login_Relay loopback to server %s\n", GetSocketID(), ipAddr.GetAddrStr()));
		ipAddr.SetAddrIP(SOCKET_LOCAL_ADDRESS);
	}

	EXC_SET("customer id");
	DWORD dwAddr = ipAddr.GetAddrIP();
	DWORD dwCustomerId = 0x7F000001;
	if ( g_Cfg.m_fUseAuthID )
	{
		CGString sCustomerID = pServ->GetName();
		sCustomerID.Add(m_pAccount->GetName());

		dwCustomerId = z_crc32(0L, Z_NULL, 0);
		dwCustomerId = z_crc32(dwCustomerId, reinterpret_cast<const z_Bytef *>(sCustomerID.GetPtr()), sCustomerID.GetLength());

		m_pAccount->m_TagDefs.SetNum("CustomerID", dwCustomerId);
	}

	DEBUG_MSG(("%lx:Login_Relay to server %s with AuthID %lu\n", GetSocketID(), ipAddr.GetAddrStr(), dwCustomerId));

	EXC_SET("server relay packet");
	new PacketServerRelay(this, dwAddr, pServ->m_ip.GetPort(), dwCustomerId);

	m_Targ_Mode = CLIMODE_SETUP_RELAY;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("account '%s'\n", m_pAccount ? m_pAccount->GetName() : "");
	EXC_DEBUG_END;
	return false;
}

bool CClient::Login_Relay(WORD wRelay)
{
	ADDTOCALLSTACK("CClient::Login_Relay");
	// Client wants to be relayed to another server (PACKET_ServerSelect)

	// Sometimes we get an extra 0x80 ???
	if ( wRelay >= 0x80 )
		wRelay -= 0x80;

	// Clients >= 1.26.00 gives a list with 1-based index
	if ( wRelay > 0 )
		--wRelay;

	CServerRef pServ;
	if ( !wRelay )
		pServ = &g_Serv;	// always list ourself first
	else
	{
		--wRelay;
		pServ = g_Cfg.Server_GetDef(wRelay);
		if ( !pServ )
		{
			DEBUG_ERR(("%lx:Login_Relay Bad index %hu\n", GetSocketID(), wRelay));
			return false;
		}
	}

	return addRelay(pServ);
}

BYTE CClient::Login_ServerList(LPCTSTR pszAccount, LPCTSTR pszPassword)
{
	ADDTOCALLSTACK("CClient::Login_ServerList");
	// PACKET_ServersReq
	// Initial login (Login on "loginserver", new format)

	CGString sMsg;
	BYTE bResult = LogIn(pszAccount, pszPassword, sMsg);
	if ( bResult != PacketLoginError::Success )
		return bResult;

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	new PacketServerList(this);
	return PacketLoginError::Success;
}

///////////////////////////////////////////////////////////

bool CClient::OnRxConsole(const BYTE *pData, size_t iLen)
{
	ADDTOCALLSTACK("CClient::OnRxConsole");
	// A special console version of the client (not game protocol)
	if ( !iLen || (GetConnectType() != CONNECT_TELNET) || !GetPeer().IsValidAddr() )
		return false;

	if ( IsSetEF(EF_AllowTelnetPacketFilter) )
	{
		if ( xPacketFilter(pData, iLen) )
			return true;
	}

	while ( iLen-- )
	{
		int iRet = OnConsoleKey(m_Targ_Text, *pData++, m_pAccount != NULL);
		if ( !iRet )
			return false;

		if ( iRet == 2 )
		{
			if ( !m_pAccount )
			{
				if ( !m_zLogin[0] )
				{
					if ( m_Targ_Text.GetLength() > sizeof(m_zLogin) - 1 )
						SysMessage("\nLogin: ");
					else
					{
						strncpy(m_zLogin, m_Targ_Text, sizeof(m_zLogin) - 1);
						SysMessage("\nPassword: ");
					}
					m_Targ_Text.Empty();
				}
				else
				{
					CAccount *pAccount = g_Accounts.Account_Find(m_zLogin);
					if ( !pAccount || (pAccount->GetPrivLevel() < PLEVEL_Admin) )
					{
						SysMessagef("\n%s", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_NOT_PRIV));
						m_Targ_Text.Empty();
						return false;
					}
					SysMessage("\n");

					CGString sMsg;
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg) == PacketLoginError::Success )
					{
						g_Log.Event(LOGM_ACCOUNTS, "%lx:Account '%s' connected on remote admin console\n", GetSocketID(), GetName());
						m_Targ_Text.Empty();
						return true;
					}
					else if ( !sMsg.IsEmpty() )
					{
						SysMessage(sMsg);
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
			else
			{
				iRet = g_Serv.OnConsoleCmd(m_Targ_Text, this);
				if ( g_Cfg.m_fTelnetLog && (GetPrivLevel() >= g_Cfg.m_iCommandLog) )
					g_Log.Event(LOGM_GM_CMDS, "%lx:'%s' commands '%s'=%d\n", GetSocketID(), GetName(), static_cast<LPCTSTR>(m_Targ_Text), iRet);
			}
		}
	}
	return true;
}

bool CClient::OnRxAxis(const BYTE *pData, size_t iLen)
{
	ADDTOCALLSTACK("CClient::OnRxAxis");
	if ( !iLen || (GetConnectType() != CONNECT_AXIS) || !GetPeer().IsValidAddr() )
		return false;

	while ( iLen-- )
	{
		int iRet = OnConsoleKey(m_Targ_Text, *pData++, m_pAccount != NULL);
		if ( !iRet )
			return false;

		if ( iRet == 2 )
		{
			if ( !m_pAccount )
			{
				if ( !m_zLogin[0] )
				{
					strncpy(m_zLogin, m_Targ_Text, sizeof(m_zLogin) - 1);
					m_Targ_Text.Empty();
				}
				else
				{
					CAccount *pAccount = g_Accounts.Account_Find(m_zLogin);
					if ( !pAccount || (pAccount->GetPrivLevel() < PLEVEL_Counsel) )
					{
						SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_NOT_PRIV));
						m_Targ_Text.Empty();
						return false;
					}

					CGString sMsg;
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg) == PacketLoginError::Success )
					{
						m_Targ_Text.Empty();

						CScriptTriggerArgs Args;
						Args.m_VarsLocal.SetStrNew("Account", GetName());
						Args.m_VarsLocal.SetStrNew("IP", GetPeer().GetAddrStr());

						TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
						r_Call("f_axis_preload", this, &Args, NULL, &tr);
						if ( tr == TRIGRET_RET_FALSE )
							return false;
						else if ( tr == TRIGRET_RET_TRUE )
						{
							SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_DENIED));
							return false;
						}

						time_t dateChange;
						DWORD dwSize;
						if ( !CFileList::ReadFileInfo("Axis.db", dateChange, dwSize) )
						{
							SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_INFO_ERROR));
							return false;
						}

						CGFile FileRead;
						if ( !FileRead.Open("Axis.db", OF_READ|OF_BINARY) )
						{
							SysMessagef("\"MSG:%s\"", g_Cfg.GetDefaultMsg(DEFMSG_AXIS_FILE_ERROR));
							return false;
						}

						TCHAR szTmp[8 * 1024];
						PacketWeb packet;
						for (;;)
						{
							size_t iLength = FileRead.Read(szTmp, sizeof(szTmp));
							if ( iLength <= 0 )
								break;
							packet.setData((BYTE *)szTmp, iLength);
							packet.send(this);
							dwSize -= iLength;
							if ( dwSize <= 0 )
								break;
						}

						g_Log.Event(LOGM_ACCOUNTS, "%lx:Account '%s' connected on Axis remote profile\n", GetSocketID(), GetName());
						return true;
					}
					else if ( !sMsg.IsEmpty() )
					{
						SysMessagef("\"MSG:%s\"", static_cast<LPCTSTR>(sMsg));
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
		}
	}
	return true;
}

bool CClient::OnRxPing(const BYTE *pData, size_t iLen)
{
	ADDTOCALLSTACK("CClient::OnRxPing");
	// RETURN:
	//  true = keep the connection open

	if ( !iLen || (iLen > 4) || (GetConnectType() != CONNECT_UNK) )
		return false;

	switch ( pData[0] )
	{
		// Remote admin console (Telnet)
		case '\x1':
		case ' ':
		{
			if ( (iLen > 1) && ((iLen != 2) || (pData[1] != '\n')) && ((iLen != 3) || (pData[1] != '\r') || (pData[2] != '\n')) && ((iLen != 3) || (pData[1] != '\n') || (pData[2] != '\0')) )
				break;

			SetConnectType(CONNECT_TELNET);
			m_zLogin[0] = 0;
			SysMessagef("\n%s", g_Cfg.GetDefaultMsg(DEFMSG_CONSOLE_WELCOME_2));
			SysMessage("\nLogin: ");
			return true;
		}

		// Axis remote profile
		case '@':
		{
			if ( (iLen > 1) && ((iLen != 2) || (pData[1] != '\n')) && ((iLen != 3) || (pData[1] != '\r') || (pData[2] != '\n')) && ((iLen != 3) || (pData[1] != '\n') || (pData[2] != '\0')) )
				break;

			SetConnectType(CONNECT_AXIS);
			m_zLogin[0] = 0;

			time_t dateChange;
			DWORD dwSize;
			CFileList::ReadFileInfo("Axis.db", dateChange, dwSize);
			SysMessagef("%lu", dwSize);
			return true;
		}

		// ConnectUO status request
		case 0xF1:
		{
			// ConnectUO sends a 4-byte packet when requesting status info
			// WORD Unk		(0x04)
			// BYTE Cmd		(0xF1)
			// BYTE SubCmd	(0xFF)

			if ( (iLen != MAKEWORD(pData[2], pData[1])) || (pData[3] != 0xFF) )
				break;

			if ( !g_Cfg.m_fCUOStatus )
			{
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:CUO status request from %s has been rejected\n", GetSocketID(), GetPeerStr());
				return false;
			}

			SetConnectType(CONNECT_TELNET);
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:CUO status request from %s\n", GetSocketID(), GetPeerStr());
			SysMessage(g_Serv.GetStatusString(0x25));
			SetConnectType(CONNECT_UNK);
			return false;
		}

		// UOGateway status request
		case 0x22:
		case 0x7F:
		case 0xFF:
		{
			if ( iLen > 1 )
				break;

			if ( !g_Cfg.m_fUOGStatus )
			{
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:UOG status request from %s has been rejected\n", GetSocketID(), GetPeerStr());
				return false;
			}

			SetConnectType((pData[0] == 0x7F) ? CONNECT_UOG : CONNECT_TELNET);
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:UOG status request from %s\n", GetSocketID(), GetPeerStr());
			SysMessage(g_Serv.GetStatusString(0x22));
			SetConnectType(CONNECT_UNK);
			return false;
		}
	}

	g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Unknown/invalid ping data '0x%x' from %s (Len: %" FMTSIZE_T ")\n", GetSocketID(), pData[0], GetPeerStr(), iLen);
	return false;
}

bool CClient::OnRxWebPageRequest(BYTE *pRequest, size_t iLen)
{
	ADDTOCALLSTACK("CClient::OnRxWebPageRequest");
	// A web browser pointing at us
	if ( GetConnectType() != CONNECT_HTTP )
		return false;

	// Ensure request is null-terminated (if the request is well-formed, we are overwriting a trailing \n here)
	pRequest[iLen - 1] = '\0';

	if ( strlen(reinterpret_cast<char *>(pRequest)) > 1024 )	// too long request
		return false;
	if ( !strpbrk(reinterpret_cast<char *>(pRequest), " \t\012\015") )		// malformed request
		return false;

	TCHAR *ppLines[16];
	size_t iQtyLines = Str_ParseCmds(reinterpret_cast<char *>(pRequest), ppLines, COUNTOF(ppLines), "\r\n");
	if ( (iQtyLines < 1) || (iQtyLines >= 15) )		// too long request
		return false;

	// Look for what they want to do with the connection
	bool fKeepAlive = false;
	TCHAR *pszReferer = NULL;
	size_t iContentLength = 0;
	CGTime timeLastModified;
	for ( size_t i = 1; i < iQtyLines; ++i )
	{
		TCHAR *pszArgs = Str_TrimWhitespace(ppLines[i]);
		if ( !strnicmp(pszArgs, "Connection:", 11) )
		{
			pszArgs += 11;
			GETNONWHITESPACE(pszArgs);
			if ( !strnicmp(pszArgs, "Keep-Alive", 10) )
				fKeepAlive = true;
		}
		else if ( !strnicmp(pszArgs, "Referer:", 8) )
		{
			pszReferer = pszArgs + 8;
		}
		else if ( !strnicmp(pszArgs, "Content-Length:", 15) )
		{
			pszArgs += 15;
			GETNONWHITESPACE(pszArgs);
			iContentLength = strtoul(pszArgs, NULL, 10);
		}
		else if ( !strnicmp(pszArgs, "If-Modified-Since:", 18) )
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			timeLastModified.Read(pszArgs);
		}
	}

	TCHAR *ppRequest[4];
	size_t iQtyArgs = Str_ParseCmds(ppLines[0], ppRequest, COUNTOF(ppRequest), " ");
	if ( (iQtyArgs < 2) || (strlen(ppRequest[1]) >= _MAX_PATH) )
		return false;

	if ( strchr(ppRequest[1], '\r') || strchr(ppRequest[1], 0xC) )
		return false;

	// If the client hasn't requested a keep alive, we must act as if they had
	// when async networking is used, otherwise data may not be completely sent
	if ( !fKeepAlive )
	{
		fKeepAlive = m_NetState->isAsyncMode();

		// Must switch to a blocking socket when the connection is not being kept
		// alive, or else pending data will be lost when the socket shuts down
		if ( !fKeepAlive )
		{
			if ( m_NetState->m_socket.SetNonBlocking(false) == SOCKET_ERROR )
				g_Log.Event(LOGL_FATAL, "Unable to unset listen socket nonblocking mode\n");
		}
	}

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;		// in mSec
	m_NetState->m_socket.SetSockOpt(SO_LINGER, reinterpret_cast<const void *>(&llinger), sizeof(linger));

	BOOL iBool = true;
	m_NetState->m_socket.SetSockOpt(SO_KEEPALIVE, &iBool, sizeof(BOOL));

	// Disable NAGLE algorithm for data compression
	iBool = true;
	m_NetState->m_socket.SetSockOpt(TCP_NODELAY, &iBool, sizeof(BOOL), IPPROTO_TCP);

	if ( memcmp(ppLines[0], "POST", 4) == 0 )
	{
		if ( iContentLength > strlen(ppLines[iQtyLines - 1]) )
			return false;

		// POST /--WEBBOT-SELF-- HTTP/1.1
		// Referer: http://127.0.0.1:2593/spherestatus.htm
		// Content-Type: application/x-www-form-urlencoded
		// Host: 127.0.0.1:2593
		// Content-Length: 29
		// T1=stuff1&B1=Submit&T2=stuff2

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%lx:HTTP Page Post '%s'\n", GetSocketID(), static_cast<LPCTSTR>(ppRequest[1]));

		CWebPageDef *pWebPage = g_Cfg.FindWebPage(ppRequest[1]);
		if ( !pWebPage )
			pWebPage = g_Cfg.FindWebPage(pszReferer);

		if ( pWebPage )
		{
			if ( pWebPage->ServPagePost(this, ppLines[iQtyLines - 1], iContentLength) )
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
		if ( !Str_GetBare(szPageName, Str_TrimWhitespace(ppRequest[1]), sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`") )
			return false;

		g_Log.Event(LOGM_HTTP|LOGL_EVENT, "%lx:HTTP Page Request '%s', alive=%d\n", GetSocketID(), static_cast<LPCTSTR>(szPageName), fKeepAlive);
		if ( CWebPageDef::ServPage(this, szPageName, &timeLastModified) )
			return fKeepAlive;
	}
	return false;
}

bool CClient::xProcessClientSetup(CEvent *pEvent, size_t iLen)
{
	ADDTOCALLSTACK("CClient::xProcessClientSetup");
	// If this is a login, try to process the data and get client version
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed!
	ASSERT(GetConnectType() == CONNECT_CRYPT);
	ASSERT(!m_Crypt.IsInit());
	ASSERT(pEvent);
	ASSERT(iLen > 0);

	// Try all client versions on the message
	CEvent evInputBuffer;
	ASSERT(iLen <= sizeof(evInputBuffer));
	memcpy(evInputBuffer.m_Raw, pEvent->m_Raw, iLen);

	if ( !m_Crypt.Init(m_NetState->m_seed, evInputBuffer.m_Raw, iLen, m_NetState->isClientKR()) )
	{
#ifdef _DEBUG
		xRecordPacketData(this, (const BYTE *)pEvent, iLen, "client->server");
#endif
		DEBUG_MSG(("%lx:Odd login message length %" FMTSIZE_T "\n", GetSocketID(), iLen));
		addLoginErr(PacketLoginError::BadEncLength);
		return false;
	}

	SetConnectType(m_Crypt.GetConnectType());

	if ( !xCanEncLogin() )
	{
		addLoginErr((m_Crypt.GetEncryptionType() == ENC_NONE) ? static_cast<BYTE>(PacketLoginError::EncNoCrypt) : static_cast<BYTE>(PacketLoginError::EncCrypt));
		return false;
	}
	else if ( (m_Crypt.GetConnectType() == CONNECT_LOGIN) && !xCanEncLogin(true) )
	{
		addLoginErr(PacketLoginError::BadVersion);
		return false;
	}

	m_Crypt.Decrypt(pEvent->m_Raw, evInputBuffer.m_Raw, iLen);
	BYTE bErr = PacketLoginError::EncUnknown;

	switch ( pEvent->Default.m_Cmd )
	{
		case PACKET_ServersReq:
		{
			if ( iLen < sizeof(pEvent->ServersReq) )
				return false;

			bErr = Login_ServerList(pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass);
			if ( bErr == PacketLoginError::Success )
			{
				TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE + 3];
				Str_GetBare(szAccount, pEvent->ServersReq.m_acctname, sizeof(szAccount) - 1);
				CAccount *pAccount = g_Accounts.Account_Find(szAccount);

				if ( pAccount )
				{
					// On login proccess, the client will connect on IP only to request the servers list, and after select an server,
					// it will disconnect from this IP and connect again now on server IP to request the account character list. But
					// on the 2nd connection the client doesn't report its version to server again, so we must use tags to temporarily
					// store the client version on PACKET_ServersReq and restore it on PACKET_CharListReq
					if ( m_Crypt.GetClientVer() )
						pAccount->m_TagDefs.SetNum("ClientVersion", m_Crypt.GetClientVer());
					if ( m_NetState->getReportedVersion() )
						pAccount->m_TagDefs.SetNum("ReportedCliVer", m_NetState->getReportedVersion());
					else
						new PacketClientVersionReq(this);
				}
				else
					bErr = PacketLoginError::Invalid;
			}
			break;
		}

		case PACKET_CharListReq:
		{
			if ( iLen < sizeof(pEvent->CharListReq) )
				return false;

			TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE + 3];
			Str_GetBare(szAccount, pEvent->CharListReq.m_acctname, sizeof(szAccount) - 1);
			CAccount *pAccount = g_Accounts.Account_Find(szAccount);

			DWORD dwCustomerID = 0x7F000001;
			if ( pAccount )
			{
				if ( g_Cfg.m_fUseAuthID )
				{
					dwCustomerID = static_cast<DWORD>(pAccount->m_TagDefs.GetKeyNum("CustomerID"));
					pAccount->m_TagDefs.DeleteKey("CustomerID");
				}

				DWORD dwVer = static_cast<DWORD>(pAccount->m_TagDefs.GetKeyNum("ClientVersion"));
				if ( dwVer )
				{
					m_Crypt.SetClientVerEnum(dwVer, false);
					m_NetState->m_clientVersion = dwVer;
					pAccount->m_TagDefs.DeleteKey("ClientVersion");
				}

				DWORD dwVerReported = static_cast<DWORD>(pAccount->m_TagDefs.GetKeyNum("ReportedCliVer"));
				if ( dwVerReported )
				{
					m_NetState->m_reportedVersion = dwVerReported;
					pAccount->m_TagDefs.DeleteKey("ReportedCliVer");
				}

				// Enhanced clients must enable some specific features on CClient::UpdateFeatureFlags() but they only report
				// client type to server after this function, so we must estimate the client type based on client version
				// to enable these features properly even when the client has not reported its client type to server yet
				if ( m_NetState->isClientVersion(MASK_CLIENTTYPE_EC) )
					m_NetState->m_clientType = CLIENTTYPE_EC;
			}

			bErr = Setup_ListReq(pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true);
			if ( bErr == PacketLoginError::Success )
			{
				if ( pAccount )
				{
					DEBUG_MSG(("%lx:xProcessClientSetup for %s, with AuthId %lu and CliVersion %lu / CliVersionReported %lu\n", GetSocketID(), pAccount->GetName(), dwCustomerID, m_Crypt.GetClientVer(), m_NetState->getReportedVersion()));

					if ( (dwCustomerID != 0) && (dwCustomerID == pEvent->CharListReq.m_Account) )
					{
						if ( !xCanEncLogin(true) )
							bErr = PacketLoginError::BadVersion;
					}
					else
						bErr = PacketLoginError::BadAuthID;
				}
				else
					bErr = PacketLoginError::Invalid;
			}
			break;
		}

#ifdef _DEBUG
		default:
			DEBUG_ERR(("Unknown/bad packet to receive at this time: 0x%X\n", pEvent->Default.m_Cmd));
#endif
	}

#ifdef _DEBUG
	xRecordPacketData(this, (const BYTE *)pEvent, iLen, "client->server");
#endif

	if ( bErr != PacketLoginError::Success )
		addLoginErr(bErr);

	return (bErr == PacketLoginError::Success);
}

bool CClient::xCanEncLogin(bool fCheckCliver)
{
	ADDTOCALLSTACK("CClient::xCanEncLogin");
	if ( !fCheckCliver )
		return (m_Crypt.GetEncryptionType() == ENC_NONE) ? g_Cfg.m_fUsenocrypt : g_Cfg.m_fUsecrypt;

	if ( !g_Serv.m_ClientVersion.GetClientVer() )	// any client version allowed
		return true;
	if ( m_Crypt.GetEncryptionType() == ENC_NONE )	// if unencrypted we check that later
		return true;
	return (m_Crypt.GetClientVer() == g_Serv.m_ClientVersion.GetClientVer());
}
