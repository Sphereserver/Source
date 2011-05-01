#include "network.h"
#include "send.h"
#include "receive.h"

#if !defined(_WIN32) || defined(_LIBEV)
	extern LinuxEv g_NetworkEvent;
#endif

#ifdef _WIN32
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); closesocket(_x_); }
#else
#	define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); close(_x_); }
#endif

#ifndef _MTNETWORK
NetworkIn g_NetworkIn;
NetworkOut g_NetworkOut;
#else
NetworkManager g_NetworkManager;
#endif

//
// Packet logging
//
#if defined(_PACKETDUMP) || defined(_DUMPSUPPORT)

void xRecordPacketData(const CClient* client, const BYTE* data, size_t length, LPCTSTR heading)
{
#ifdef _DUMPSUPPORT
	if (client->GetAccount() != NULL && strnicmp(client->GetAccount()->GetName(), (LPCTSTR) g_Cfg.m_sDumpAccPackets, strlen( client->GetAccount()->GetName())))
		return;
#else
	if (!(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS))
		return;
#endif

	Packet packet(data, length);
	xRecordPacket(client, &packet, heading);
}

void xRecordPacket(const CClient* client, Packet* packet, LPCTSTR heading)
{
#ifdef _DUMPSUPPORT
	if (client->GetAccount() != NULL && strnicmp(client->GetAccount()->GetName(), (LPCTSTR) g_Cfg.m_sDumpAccPackets, strlen( client->GetAccount()->GetName())))
		return;
#else
	if (!(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS))
		return;
#endif

	TemporaryString dump;
	packet->dump(dump);

#ifdef _DEBUG
	// write to console
	g_Log.EventDebug("%lx:%s %s\n", client->GetSocketID(), heading, (LPCTSTR)dump);
#endif

	// build file name
	TCHAR fname[64];
	strcpy(fname, "packets_");
	if (client->GetAccount())
		strcat(fname, client->GetAccount()->GetName());
	else
	{
		strcat(fname, "(");
		strcat(fname, client->GetPeerStr());
		strcat(fname, ")");
	}

	strcat(fname, ".log");

	CGString sFullFileName = CGFile::GetMergedFileName(g_Log.GetLogDir(), fname);
	
	// write to file
	CFileText out;
	if (out.Open(sFullFileName, OF_READWRITE|OF_TEXT))
	{
		out.Printf("%s %s\n\n", heading, (LPCTSTR)dump);
		out.Close();
	}
}
#endif


/***************************************************************************
 *
 *
 *	class NetState				Network status (client info, etc)
 *
 *
 ***************************************************************************/

NetState::NetState(long id)
{
	m_id = id;
	m_client = NULL;
	m_needsFlush = false;
	m_useAsync = false;
	m_currentTransaction = NULL;
	m_pendingTransaction = NULL;
	m_packetExceptions = 0;
	m_receiveBuffer = NULL;
	m_clientType = CLIENTTYPE_2D;
	m_clientVersion = 0;
	m_reportedVersion = 0;
	m_isInUse = false;
#ifdef _MTNETWORK
	m_parent = NULL;
#endif

	clear();
}

NetState::~NetState(void)
{
}

void NetState::clear(void)
{
	ADDTOCALLSTACK("NetState::clear");
	DEBUGNETWORK(("%lx:Clearing client state.\n", id()));

	m_isReadClosed = true;
	m_isWriteClosed = true;
	m_needsFlush = false;

	CClient* client = m_client;
	if (client != NULL)
	{
		m_client = NULL;

		g_Serv.StatDec(SERV_STAT_CLIENTS);
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Client disconnected [Total:%lu] ('%s')\n",
			m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_peerAddress.GetAddrStr());
		
#if !defined(_WIN32) || defined(_LIBEV)
		if (m_socket.IsOpen() && g_Cfg.m_fUseAsyncNetwork != 0)
			g_NetworkEvent.unregisterClient(this);
#endif

		//	record the client reference to the garbage collection to be deleted on it's time
		g_World.m_ObjDelete.InsertHead(client);
	}
	
#ifdef _WIN32
	if (m_socket.IsOpen() && isAsyncMode())
		m_socket.ClearAsync();
#endif


	m_socket.Close();
	m_client = NULL;

	// empty queues
	clearQueues();

	// clean junk queue entries
	for (size_t i = 0; i < PacketSend::PRI_QTY; i++)
		m_queue[i].clean();
	m_asyncQueue.clean();

	if (m_currentTransaction != NULL)
	{
		delete m_currentTransaction;
		m_currentTransaction = NULL;
	}

	if (m_pendingTransaction != NULL)
	{
		delete m_pendingTransaction;
		m_pendingTransaction = NULL;
	}

	if (m_receiveBuffer != NULL)
	{
		delete m_receiveBuffer;
		m_receiveBuffer = NULL;
	}

	m_sequence = 0;
	m_seeded = false;
	m_newseed = false;
	m_seed = 0;
	m_clientVersion = m_reportedVersion = 0;
	m_clientType = CLIENTTYPE_2D;
	m_isSendingAsync = false;
	m_packetExceptions = 0;
	setAsyncMode(false);
	m_isInUse = false;
}

void NetState::clearQueues(void)
{
	ADDTOCALLSTACK("NetState::clearQueues");

	// clear packet queues
	for (size_t i = 0; i < PacketSend::PRI_QTY; i++)
	{
		while (m_queue[i].empty() == false)
		{
			delete m_queue[i].front();
			m_queue[i].pop();
		}
	}

	// clear async queue
	while (m_asyncQueue.empty() == false)
	{
		delete m_asyncQueue.front();
		m_asyncQueue.pop();
	}

	// clear byte queue
	m_byteQueue.Empty();
}

void NetState::init(SOCKET socket, CSocketAddress addr)
{
	ADDTOCALLSTACK("NetState::init");

	clear();

	DEBUGNETWORK(("%lx:Initialising client\n", id()));
	m_peerAddress = addr;
	m_socket.SetSocket(socket);
	m_socket.SetNonBlocking();

	// disable NAGLE algorythm for data compression/coalescing.
	// Send as fast as we can. we handle packing ourselves.
	BOOL nbool = true;
	m_socket.SetSockOpt(TCP_NODELAY, &nbool, sizeof(BOOL), IPPROTO_TCP);

	g_Serv.StatInc(SERV_STAT_CLIENTS);
	CClient* client = new CClient(this);
	m_client = client;
	
#if !defined(_WIN32) || defined(_LIBEV)
	if (g_Cfg.m_fUseAsyncNetwork != 0)
	{
		DEBUGNETWORK(("%lx:Registering async client\n", id()));
		g_NetworkEvent.registerClient(this, LinuxEv::Write);
	}
#endif
	
	DEBUGNETWORK(("%lx:Opening network state\n", id()));
	m_isWriteClosed = false;
	m_isReadClosed = false;
	m_isInUse = true;

	DEBUGNETWORK(("%lx:Determining async mode\n", id()));
	detectAsyncMode();
}

bool NetState::isInUse(const CClient* client) const volatile
{
	if (m_isInUse == false)
		return false;

	return client == NULL || m_client == client;
}

void NetState::markReadClosed(void) volatile
{
#ifdef _MTNETWORK
	ADDTOCALLSTACK("NetState::markReadClosed");
#endif

	DEBUGNETWORK(("%lx:Client being closed by read-thread\n", m_id));
	m_isReadClosed = true;
#ifdef _MTNETWORK
	if (m_parent != NULL && m_parent->getPriority() == IThread::Disabled)
		m_parent->awaken();
#endif
}

void NetState::markWriteClosed(void) volatile
{
	DEBUGNETWORK(("%lx:Client being closed by write-thread\n", m_id));
	m_isWriteClosed = true;
}

void NetState::markFlush(bool needsFlush) volatile
{
	m_needsFlush = needsFlush;
}

void NetState::detectAsyncMode(void)
{
	ADDTOCALLSTACK("NetState::detectAsyncMode");
	bool wasAsync = isAsyncMode();

	// is async mode enabled?
	if ( !g_Cfg.m_fUseAsyncNetwork || !isInUse() )
		setAsyncMode(false);

	// if the version mod flag is not set, always use async mode
	else if ( g_Cfg.m_fUseAsyncNetwork != 2 )
		setAsyncMode(true);

	// http clients do not want to be using async networking unless they have keep-alive set
	else if (getClient() != NULL && getClient()->GetConnectType() == CONNECT_HTTP)
		setAsyncMode(false);

	// only use async with clients newer than 4.0.0
	// - normally the client version is unknown for the first 1 or 2 packets, so all clients will begin
	//   without async networking (but should switch over as soon as it has been determined)
	// - a minor issue with this is that for clients without encryption we cannot determine their version
	//   until after they have fully logged into the game server and sent a client version packet.
	else if (isClientVersion(MINCLIVER_AUTOASYNC) || isClientKR() || isClientSA())
		setAsyncMode(true);
	else
		setAsyncMode(false);

	if (wasAsync != isAsyncMode())
		DEBUGNETWORK(("%lx:Switching async mode from %s to %s.\n", id(), wasAsync? "1":"0", isAsyncMode()? "1":"0"));
}

bool NetState::hasPendingData(void) const
{
	ADDTOCALLSTACK("NetState::hasPendingData");
	// check if state is even valid
	if (isInUse() == false || m_socket.IsOpen() == false)
		return false;

	// even if there are packets, it doesn't matter once the write thread considers the state closed
	if (isWriteClosed())
		return false;

	// check packet queues (only count high priority+ for closed states)
	for (int i = (isClosing() ? NETWORK_DISCONNECTPRI : PacketSend::PRI_IDLE); i < PacketSend::PRI_QTY; i++)
	{
		if (m_queue[i].empty() == false)
			return true;
	}

	// check async data
	if (isAsyncMode() && m_asyncQueue.empty() == false)
		return true;

	// check byte queue
	if (m_byteQueue.GetDataQty() > 0)
		return true;

	// check current transaction
	if (m_currentTransaction != NULL)
		return true;

	return false;
}

bool NetState::canReceive(PacketSend* packet) const
{
	if (isInUse() == false || m_socket.IsOpen() == false || packet == NULL)
		return false;

	if (isClosing() && packet->getPriority() < NETWORK_DISCONNECTPRI)
		return false;

	if (packet->getTarget()->m_client == NULL)
		return false;

	return true;
}

void NetState::beginTransaction(long priority)
{
	ADDTOCALLSTACK("NetState::beginTransaction");
	if (m_pendingTransaction != NULL)
	{
		DEBUGNETWORK(("%lx:New network transaction started whilst a previous is still open.\n", id()));
	}

	// ensure previous transaction is committed
	endTransaction();

	//DEBUGNETWORK(("%lx:Starting a new packet transaction.\n", id()));

	m_pendingTransaction = new ExtendedPacketTransaction(this, g_Cfg.m_fUsePacketPriorities? priority : (long)PacketSend::PRI_NORMAL);
}

void NetState::endTransaction(void)
{
	ADDTOCALLSTACK("NetState::endTransaction");
	if (m_pendingTransaction == NULL)
		return;

	//DEBUGNETWORK(("%lx:Scheduling packet transaction to be sent.\n", id()));

#ifndef _MTNETWORK
	g_NetworkOut.scheduleOnce(m_pendingTransaction);
#else
	m_parent->queuePacketTransaction(m_pendingTransaction);
#endif
	m_pendingTransaction = NULL;
}


/***************************************************************************
 *
 *
 *	struct HistoryIP			Simple interface for IP history maintainence
 *
 *
 ***************************************************************************/
void HistoryIP::update(void)
{
	// reset ttl
	m_ttl = NETHISTORY_TTL;
}

bool HistoryIP::checkPing(void)
{
	// ip is pinging, check if blocked
	update();

	return ( m_blocked || ( m_pings++ >= NETHISTORY_MAXPINGS ));
}

void HistoryIP::setBlocked(bool isBlocked, int timeout)
{
	// block ip
	ADDTOCALLSTACK("HistoryIP:setBlocked");
	if (isBlocked == true)
	{
		CScriptTriggerArgs args(m_ip.GetAddrStr());
		args.m_iN1 = timeout;
		g_Serv.r_Call("f_onserver_blockip", &g_Serv, &args);
		timeout = args.m_iN1;
	}

	m_blocked = isBlocked;

	if (isBlocked && timeout >= 0)
		m_blockExpire = CServTime::GetCurrentTime() + timeout;
	else
		m_blockExpire.Init();
}
	
/***************************************************************************
 *
 *
 *	class IPHistoryManager		Holds IP records (bans, pings, etc)
 *
 *
 ***************************************************************************/
IPHistoryManager::IPHistoryManager(void)
{
	m_lastDecayTime.Init();
}

IPHistoryManager::~IPHistoryManager(void)
{
	m_ips.clear();
}

void IPHistoryManager::tick(void)
{
	// periodic events
	ADDTOCALLSTACK("IPHistoryManager::tick");

	// check if ttl should decay (only do this once every second)
	bool decayTTL = ( !m_lastDecayTime.IsTimeValid() || (-g_World.GetTimeDiff(m_lastDecayTime)) >= TICK_PER_SEC );
	if (decayTTL)
		m_lastDecayTime = CServTime::GetCurrentTime();

	for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
	{
		if (it->m_blocked)
		{
			// blocked ips don't decay, but check if the ban has expired
			if (it->m_blockExpire.IsTimeValid() && CServTime::GetCurrentTime() > it->m_blockExpire)
				it->setBlocked(false);
		}
		else if (decayTTL)
		{
			if (it->m_connected == 0 && it->m_connecting == 0)
			{
				// start to forget about clients who aren't connected
				if (it->m_ttl >= 0)
					--it->m_ttl;
			}

			 // wait a 5th of TTL between each ping decay, but do not wait less than 30 seconds
			if (it->m_pings > 0 && --it->m_pingDecay < 0)
			{
				--it->m_pings;
				it->m_pingDecay = NETHISTORY_PINGDECAY;
			}
		}
	}

	// clear old ip history
	for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
	{
		if (it->m_ttl >= 0)
			continue;

		m_ips.erase(it);
		break;
	}
}

HistoryIP& IPHistoryManager::getHistoryForIP(const CSocketAddressIP& ip)
{
	// get history for an ip
	ADDTOCALLSTACK("IPHistoryManager::getHistoryForIP");

	// find existing entry
	for (IPHistoryList::iterator it = m_ips.begin(); it != m_ips.end(); ++it)
	{
		if ((*it).m_ip == ip)
			return *it;
	}

	// create a new entry
	HistoryIP hist;
	memset(&hist, 0, sizeof(hist));
	hist.m_ip = ip;
	hist.m_pingDecay = NETHISTORY_PINGDECAY;
	hist.update();

	m_ips.push_back(hist);
	return getHistoryForIP(ip);
}

HistoryIP& IPHistoryManager::getHistoryForIP(const char* ip)
{
	// get history for an ip
	ADDTOCALLSTACK("IPHistoryManager::getHistoryForIP");

	CSocketAddressIP me(ip);
	return getHistoryForIP(me);
}


/***************************************************************************
 *
 *
 *	class PacketManager             Holds lists of packet handlers
 *
 *
 ***************************************************************************/
PacketManager::PacketManager(void)
{
	memset(m_handlers, 0, sizeof(m_handlers));
	memset(m_extended, 0, sizeof(m_extended));
	memset(m_encoded, 0, sizeof(m_encoded));
}

PacketManager::~PacketManager(void)
{
	// delete standard packet handlers
	for (size_t i = 0; i < COUNTOF(m_handlers); ++i)
		unregisterPacket(i);

	// delete extended packet handlers
	for (size_t i = 0; i < COUNTOF(m_extended); ++i)
		unregisterExtended(i);

	// delete encoded packet handlers
	for (size_t i = 0; i < COUNTOF(m_encoded); ++i)
		unregisterEncoded(i);
}

void PacketManager::registerStandardPackets(void)
{
	ADDTOCALLSTACK("PacketManager::registerStandardPackets");

	// standard packets
	registerPacket(XCMD_Create, new PacketCreate());							// create character
	registerPacket(XCMD_Walk, new PacketMovementReq());							// movement request
	registerPacket(XCMD_Talk, new PacketSpeakReq());							// speak
	registerPacket(XCMD_Attack, new PacketAttackReq());							// begin attack
	registerPacket(XCMD_DClick, new PacketDoubleClick());						// double click object
	registerPacket(XCMD_ItemPickupReq, new PacketItemPickupReq());				// pick up item
	registerPacket(XCMD_ItemDropReq, new PacketItemDropReq());					// drop item
	registerPacket(XCMD_Click, new PacketSingleClick());						// single click object
	registerPacket(XCMD_ExtCmd, new PacketTextCommand());						// extended text command
	registerPacket(XCMD_ItemEquipReq, new PacketItemEquipReq());				// equip item
	registerPacket(XCMD_WalkAck, new PacketResynchronize());					//
	registerPacket(XCMD_DeathMenu, new PacketDeathStatus());					//
	registerPacket(XCMD_CharStatReq, new PacketCharStatusReq());				// status request
	registerPacket(XCMD_Skill, new PacketSkillLockChange());					// change skill lock
	registerPacket(XCMD_VendorBuy, new PacketVendorBuyReq());					// buy items from vendor
	registerPacket(XCMD_MapEdit, new PacketMapEdit());							// edit map pins
	registerPacket(XCMD_CharPlay, new PacketCharPlay());						// select character
	registerPacket(XCMD_BookPage, new PacketBookPageEdit());					// edit book content
	registerPacket(XCMD_Options, new PacketUnknown());						// unused options packet
	registerPacket(XCMD_Target, new PacketTarget());							// target an object
	registerPacket(XCMD_SecureTrade, new PacketSecureTradeReq());				// secure trade action
	registerPacket(XCMD_BBoard, new PacketBulletinBoardReq());					// bulletin board action
	registerPacket(XCMD_War, new PacketWarModeReq());							// toggle war mode
	registerPacket(XCMD_Ping, new PacketPingReq());								// ping request
	registerPacket(XCMD_CharName, new PacketCharRename());						// change character name
	registerPacket(XCMD_MenuChoice, new PacketMenuChoice());					// select menu item
	registerPacket(XCMD_ServersReq, new PacketServersReq());					// request server list
	registerPacket(XCMD_CharDelete, new PacketCharDelete());					// delete character
	registerPacket(XCMD_CreateNew, new PacketCreateNew());						// create character
	registerPacket(XCMD_CharListReq, new PacketCharListReq());					// request character list
	registerPacket(XCMD_BookOpen, new PacketBookHeaderEdit());					// edit book
	registerPacket(XCMD_DyeVat, new PacketDyeObject());							// colour selection dialog
	registerPacket(XCMD_AllNames3D, new PacketAllNamesReq());					// all names macro (ctrl+shift)
	registerPacket(XCMD_Prompt, new PacketPromptResponse());					// prompt response
	registerPacket(XCMD_HelpPage, new PacketHelpPageReq());						// GM help page request
	registerPacket(XCMD_VendorSell, new PacketVendorSellReq());					// sell items to vendor
	registerPacket(XCMD_ServerSelect, new PacketServerSelect());				// select server
	registerPacket(XCMD_Spy, new PacketSystemInfo());							//
	registerPacket(XCMD_Scroll, new PacketUnknown(5));							// scroll closed
	registerPacket(XCMD_TipReq, new PacketTipReq());							// request tip of the day
	registerPacket(XCMD_GumpInpValRet, new PacketGumpValueInputResponse());		// text input dialog
	registerPacket(XCMD_TalkUNICODE, new PacketSpeakReqUNICODE());				// speech (unicode)
	registerPacket(XCMD_GumpDialogRet, new PacketGumpDialogRet());				// dialog response (button press)
	registerPacket(XCMD_ChatText, new PacketChatCommand());						// chat command
	registerPacket(XCMD_Chat, new PacketChatButton());							// chat button
	registerPacket(XCMD_ToolTipReq, new PacketToolTipReq());					// popup help request
	registerPacket(XCMD_CharProfile, new PacketProfileReq());					// profile read/write request
	registerPacket(XCMD_MailMsg, new PacketMailMessage());						//
	registerPacket(XCMD_ClientVersion, new PacketClientVersion());				// client version
	registerPacket(XCMD_ExtData, new PacketExtendedCommand());					//
	registerPacket(XCMD_PromptUNICODE, new PacketPromptResponseUnicode());		// prompt response (unicode)
	registerPacket(XCMD_ViewRange, new PacketViewRange());						//
	registerPacket(XCMD_ConfigFile, new PacketUnknown());						//
	registerPacket(XCMD_LogoutStatus, new PacketLogout());						//
	registerPacket(XCMD_AOSBookPage, new PacketBookHeaderEditNew());			// edit book
	registerPacket(XCMD_AOSTooltip, new PacketAOSTooltipReq());					// request tooltip data
	registerPacket(XCMD_ExtAosData, new PacketEncodedCommand());				//
	registerPacket(XCMD_Spy2, new PacketHardwareInfo());						// client hardware info
	registerPacket(XCMD_BugReport, new PacketBugReport());						// bug report
	registerPacket(XCMD_KRClientType, new PacketClientType());					// report client type (2d/kr/sa)
	registerPacket(XCMD_HighlightUIRemove, new PacketRemoveUIHighlight());		//
	registerPacket(XCMD_UseHotbar, new PacketUseHotbar());						//
	registerPacket(XCMD_MacroEquipItem, new PacketEquipItemMacro());			//
	registerPacket(XCMD_MacroUnEquipItem, new PacketUnEquipItemMacro());		//
	registerPacket(XCMD_WalkNew, new PacketMovementReqNew());					// movement request (SA)
	registerPacket(XCMD_WalkUnknown, new PacketUnknown(9));						//
	registerPacket(XCMD_CrashReport, new PacketCrashReport());					//

	// extended packets (0xBF)
	registerExtended(EXTDATA_ScreenSize, new PacketScreenSize());				// client screen size
	registerExtended(EXTDATA_Party_Msg, new PacketPartyMessage());				// party command
	registerExtended(EXTDATA_Arrow_Click, new PacketArrowClick());				// click quest arrow
	registerExtended(EXTDATA_Wrestle_DisArm, new PacketWrestleDisarm());		// wrestling disarm macro
	registerExtended(EXTDATA_Wrestle_Stun, new PacketWrestleStun());			// wrestling stun macro
	registerExtended(EXTDATA_Lang, new PacketLanguage());						// client language
	registerExtended(EXTDATA_StatusClose, new PacketStatusClose());				// status window closed
	registerExtended(EXTDATA_Yawn, new PacketAnimationReq());					// play animation
	registerExtended(EXTDATA_Unk15, new PacketClientInfo());					// client information
	registerExtended(EXTDATA_OldAOSTooltipInfo, new PacketAosTooltipInfo());	//
	registerExtended(EXTDATA_Popup_Request, new PacketPopupReq());				// request popup menu
	registerExtended(EXTDATA_Popup_Select, new PacketPopupSelect());			// select popup option
	registerExtended(EXTDATA_Stats_Change, new PacketChangeStatLock());			// change stat lock
	registerExtended(EXTDATA_NewSpellSelect, new PacketSpellSelect());			//
	registerExtended(EXTDATA_HouseDesignDet, new PacketHouseDesignReq());		// house design request
	registerExtended(EXTDATA_AntiCheat, new PacketAntiCheat());					// anti-cheat / unknown
	registerExtended(EXTDATA_BandageMacro, new PacketBandageMacro());			//
	registerExtended(EXTDATA_GargoyleFly, new PacketGargoyleFly());				// gargoyle flying action

	// encoded packets (0xD7)
	registerEncoded(EXTAOS_HcBackup, new PacketHouseDesignBackup());			// house design - backup
	registerEncoded(EXTAOS_HcRestore, new PacketHouseDesignRestore());			// house design - restore
	registerEncoded(EXTAOS_HcCommit, new PacketHouseDesignCommit());			// house design - commit
	registerEncoded(EXTAOS_HcDestroyItem, new PacketHouseDesignDestroyItem());	// house design - remove item
	registerEncoded(EXTAOS_HcPlaceItem, new PacketHouseDesignPlaceItem());		// house design - place item
	registerEncoded(EXTAOS_HcExit, new PacketHouseDesignExit());				// house design - exit designer
	registerEncoded(EXTAOS_HcPlaceStair, new PacketHouseDesignPlaceStair());	// house design - place stairs
	registerEncoded(EXTAOS_HcSynch, new PacketHouseDesignSync());				// house design - synchronise
	registerEncoded(EXTAOS_HcClear, new PacketHouseDesignClear());				// house design - clear
	registerEncoded(EXTAOS_HcSwitch, new PacketHouseDesignSwitch());			// house design - change floor
	registerEncoded(EXTAOS_HcPlaceRoof, new PacketHouseDesignPlaceRoof());		// house design - place roof
	registerEncoded(EXTAOS_HcDestroyRoof, new PacketHouseDesignDestroyRoof());	// house design - remove roof
	registerEncoded(EXTAOS_SpecialMove, new PacketSpecialMove());				//
	registerEncoded(EXTAOS_HcRevert, new PacketHouseDesignRevert());			// house design - revert
	registerEncoded(EXTAOS_EquipLastWeapon, new PacketEquipLastWeapon());		//
	registerEncoded(EXTAOS_GuildButton, new PacketGuildButton());				// guild button press
	registerEncoded(EXTAOS_QuestButton, new PacketQuestButton());				// quest button press
}

void PacketManager::registerPacket(unsigned int id, Packet* handler)
{
	// assign standard packet handler
	ADDTOCALLSTACK("PacketManager::registerPacket");
	ASSERT(id < COUNTOF(m_handlers));
	unregisterPacket(id);
	m_handlers[id] = handler;
}

void PacketManager::registerExtended(unsigned int id, Packet* handler)
{
	// assign extended packet handler
	ADDTOCALLSTACK("PacketManager::registerExtended");
	ASSERT(id < COUNTOF(m_extended));
	unregisterExtended(id);
	m_extended[id] = handler;
}

void PacketManager::registerEncoded(unsigned int id, Packet* handler)
{
	// assign encoded packet handler
	ADDTOCALLSTACK("PacketManager::registerEncoded");
	ASSERT(id < COUNTOF(m_encoded));
	unregisterEncoded(id);
	m_encoded[id] = handler;
}

void PacketManager::unregisterPacket(unsigned int id)
{
	// delete standard packet handler
	ADDTOCALLSTACK("PacketManager::unregisterPacket");
	ASSERT(id < COUNTOF(m_handlers));
	if (m_handlers[id] == NULL)
		return;

	delete m_handlers[id];
	m_handlers[id] = NULL;	
}

void PacketManager::unregisterExtended(unsigned int id)
{
	// delete extended packet handler
	ADDTOCALLSTACK("PacketManager::unregisterExtended");
	ASSERT(id < COUNTOF(m_extended));
	if (m_extended[id] == NULL)
		return;

	delete m_extended[id];
	m_extended[id] = NULL;	
}

void PacketManager::unregisterEncoded(unsigned int id)
{
	// delete encoded packet handler
	ADDTOCALLSTACK("PacketManager::unregisterEncoded");
	ASSERT(id < COUNTOF(m_encoded));
	if (m_encoded[id] == NULL)
		return;

	delete m_encoded[id];
	m_encoded[id] = NULL;	
}

Packet* PacketManager::getHandler(unsigned int id) const
{
	// get standard packet handler
	if (id >= COUNTOF(m_handlers))
		return NULL;

	return m_handlers[id];
}

Packet* PacketManager::getExtendedHandler(unsigned int id) const
{
	// get extended packet handler
	if (id >= COUNTOF(m_extended))
		return NULL;

	return m_extended[id];
}

Packet* PacketManager::getEncodedHandler(unsigned int id) const
{
	// get encoded packet handler
	if (id >= COUNTOF(m_encoded))
		return NULL;

	return m_encoded[id];
}


/***************************************************************************
 *
 *
 *	class ClientIterator		Works as client iterator getting the clients
 *
 *
 ***************************************************************************/

#ifndef _MTNETWORK
ClientIterator::ClientIterator(const NetworkIn* network)
{
	m_network = (network == NULL? &g_NetworkIn : network);
	m_nextClient = STATIC_CAST <CClient*> (m_network->m_clients.GetHead());
}
#else
ClientIterator::ClientIterator(const NetworkManager* network)
{
	m_network = (network == NULL? &g_NetworkManager : network);
	m_nextClient = static_cast<CClient*> (m_network->m_clients.GetHead());
}
#endif

ClientIterator::~ClientIterator(void)
{
	m_network = NULL;
	m_nextClient = NULL;
}

CClient* ClientIterator::next(bool includeClosing)
{
	for (CClient* current = m_nextClient; current != NULL; current = current->GetNext())
	{
		// skip clients without a state, or whose state is invalid/closed
		if (current->GetNetState() == NULL || current->GetNetState()->isInUse(current) == false || current->GetNetState()->isClosed())
			continue;

		// skip clients whose connection is being closed
		if (includeClosing == false && current->GetNetState()->isClosing())
			continue;

		m_nextClient = current->GetNext();
		return current;
	}

	return NULL;
}


#ifndef _MTNETWORK
/***************************************************************************
 *
 *
 *	class SafeClientIterator		Works as client iterator getting the clients in a thread-safe way
 *
 *
 ***************************************************************************/

SafeClientIterator::SafeClientIterator(const NetworkIn* network)
{
	m_network = (network == NULL? &g_NetworkIn : network);
	m_id = -1;
	m_max = m_network->m_stateCount;
}

SafeClientIterator::~SafeClientIterator(void)
{
	m_network = NULL;
}

CClient* SafeClientIterator::next(bool includeClosing)
{
	// this method should be thread-safe, but does not loop through clients in the order that they have
	// connected -- ideally CGObList (or a similar container for clients) should be traversed from
	// newest client to oldest and be thread-safe)
	while (++m_id < m_max)
	{
		const NetState* state = m_network->m_states[m_id];

		// skip states which do not have a valid client, or are closed
		if (state->isInUse(state->getClient()) == false || state->isClosed())
			continue;

		// skip states which are being closed
		if (includeClosing == false && state->isClosing())
			continue;

		return state->getClient();
	}

	return NULL;
}


/***************************************************************************
 *
 *
 *	class NetworkIn::HistoryIP	Simple interface for IP history maintainese
 *
 *
 ***************************************************************************/

NetworkIn::NetworkIn(void) : AbstractSphereThread("NetworkIn", IThread::Highest)
{
	m_lastGivenSlot = 0;
	m_buffer = NULL;
	m_decryptBuffer = NULL;
	m_states = NULL;
	m_stateCount = 0;
}

NetworkIn::~NetworkIn(void)
{
	if (m_buffer != NULL)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}

	if (m_decryptBuffer != NULL)
	{
		delete[] m_decryptBuffer;
		m_decryptBuffer = NULL;
	}

	if (m_states != NULL)
	{
		for (long l = 0; l < m_stateCount; l++)
		{
			delete m_states[l];
			m_states[l] = NULL;
		}

		delete[] m_states;
		m_states = NULL;
	}

	m_clients.DeleteAll();
}

void NetworkIn::onStart(void)
{
	AbstractSphereThread::onStart();
	m_lastGivenSlot = -1;
	m_buffer = new BYTE[NETWORK_BUFFERSIZE];
	m_decryptBuffer = new BYTE[NETWORK_BUFFERSIZE];

	DEBUGNETWORK(("Registering packets...\n"));
	m_packets.registerStandardPackets();

	// create states
	m_states = new NetState*[g_Cfg.m_iClientsMax];
	for (size_t l = 0; l < g_Cfg.m_iClientsMax; l++)
		m_states[l] = new NetState(l);
	m_stateCount = g_Cfg.m_iClientsMax;

	DEBUGNETWORK(("Created %ld network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));
}

void NetworkIn::tick(void)
{
	ADDTOCALLSTACK("NetworkIn::tick");

	EXC_TRY("NetworkIn");
	if (g_Serv.m_iExitFlag || g_Serv.m_iModeCode != SERVMODE_Run)
		return;

	// periodically check ip history
	static char iPeriodic = 0;
	if (iPeriodic == 0)
	{
		EXC_SET("periodic");
		periodic();
	}

	if (++iPeriodic > 20)
		iPeriodic = 0;

	// check for incoming data
	EXC_SET("select");
	fd_set readfds;
	int ret = checkForData(&readfds);
	if (ret <= 0)
		return;

	EXC_SET("new connection");
	if (FD_ISSET(g_Serv.m_SocketMain.GetSocket(), &readfds))
		acceptConnection();

	EXC_SET("messages");
	BYTE* buffer = m_buffer;
	for (long i = 0; i < m_stateCount; i++)
	{
		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);

		EXC_SET("messages - next client");
		NetState* client = m_states[i];
		ASSERT(client != NULL);

		EXC_SET("messages - check client");
		if (client->isInUse() == false || client->isClosing() ||
			client->getClient() == NULL || client->m_socket.IsOpen() == false)
			continue;

		EXC_SET("messages - check frozen");
		if (!FD_ISSET(client->m_socket.GetSocket(), &readfds))
		{
			if (client->m_client->GetConnectType() != CONNECT_TELNET)
			{
				// check for timeout
				int iLastEventDiff = -g_World.GetTimeDiff( client->m_client->m_timeLastEvent );
				if ( g_Cfg.m_iDeadSocketTime && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
				{
					g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Frozen client connection disconnected.\n", client->m_id);
					client->markReadClosed();
				}
			}
			continue;
		}

		// receive data
		EXC_SET("messages - receive");
		size_t received = client->m_socket.Receive(buffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			client->markReadClosed();
			continue;
		}

		EXC_SET("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);
		ProfileTask clientTask(PROFILE_CLIENTS);

		EXC_SET("messages - process");
		if (client->m_client->GetConnectType() == CONNECT_UNK)
		{
			if (client->m_seeded == false)
			{
				if (received >= 4) // login connection
				{
					EXC_SET("login message");
					if ( memcmp(buffer, "GET /", 5) == 0 || memcmp(buffer, "POST /", 6) == 0 ) // HTTP
					{
						EXC_SET("http request");
						if ( g_Cfg.m_fUseHTTP != 2 )
						{
							client->markReadClosed();
							continue;
						}

						client->m_client->SetConnectType(CONNECT_HTTP);
						if ( !client->m_client->OnRxWebPageRequest(buffer, received) )
						{
							client->markReadClosed();
							continue;
						}

						continue;
					}

					EXC_SET("game client seed");
					DWORD seed(0);
					size_t iSeedLen(0);
					if (client->m_newseed || (buffer[0] == XCMD_NewSeed && received >= NETWORK_SEEDLEN_NEW))
					{
						DEBUGNETWORK(("%lx:Receiving new client login handshake.\n", client->id()));

						CEvent* pEvent = (CEvent*)buffer;

						if (client->m_newseed)
						{
							// we already received the 0xEF on its own, so move the pointer
							// back 1 byte to align it
							iSeedLen = NETWORK_SEEDLEN_NEW - 1;
							pEvent = (CEvent *)(((BYTE*)pEvent) - 1);
						}
						else
						{
							iSeedLen = NETWORK_SEEDLEN_NEW;
							client->m_newseed = true;
						}

						if (received >= iSeedLen)
						{
							DEBUG_WARN(("%lx:New Login Handshake Detected. Client Version: %lu.%lu.%lu.%lu\n", client->id(),
										 (DWORD)pEvent->NewSeed.m_Version_Maj, 
										 (DWORD)pEvent->NewSeed.m_Version_Min, (DWORD)pEvent->NewSeed.m_Version_Rev, 
										 (DWORD)pEvent->NewSeed.m_Version_Pat));

							client->m_reportedVersion = CCrypt::GetVerFromVersion(pEvent->NewSeed.m_Version_Maj, pEvent->NewSeed.m_Version_Min, pEvent->NewSeed.m_Version_Rev, pEvent->NewSeed.m_Version_Pat);
							seed = (DWORD) pEvent->NewSeed.m_Seed;
						}
						else
						{
							DEBUGNETWORK(("%lx:Not enough data received to be a valid handshake (%" FMTSIZE_T ").\n", client->id(), received));
						}
					}
					else
					{
						// assume it's a normal client log in
						DEBUGNETWORK(("%lx:Receiving old client login handshake.\n", client->id()));

						seed = ( buffer[0] << 24 ) | ( buffer[1] << 16 ) | ( buffer[2] << 8 ) | buffer[3];
						iSeedLen = NETWORK_SEEDLEN_OLD;
					}

					DEBUGNETWORK(("%lx:Client connected with a seed of 0x%lx (new handshake=%d, seed length=%" FMTSIZE_T ", received=%" FMTSIZE_T ", version=0x%lx).\n", client->id(), seed, client->m_newseed? 1 : 0, iSeedLen, received, client->m_reportedVersion));

					if ( !seed || iSeedLen > received )
					{
						g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Invalid client detected, disconnecting.\n", client->id());
						client->markReadClosed();
						continue;
					}

					client->m_seeded = true;
					client->m_seed = seed;
					buffer += iSeedLen;
					received -= iSeedLen;
				}
				else
				{
					if (*buffer == XCMD_NewSeed)
					{
						// Game client
						client->m_newseed = true;
						continue;
					}

					EXC_SET("ping #1");
					if (client->m_client->OnRxPing(buffer, received) == false)
						client->markReadClosed();

					continue;
				}
			}

			if (received == 0)
			{
				if (client->m_seed == 0xFFFFFFFF)
				{
					// UOKR Client opens connection with 255.255.255.255
					EXC_SET("KR client seed");

					DEBUG_WARN(("UOKR Client Detected.\n"));
					client->m_client->SetConnectType(CONNECT_CRYPT);
					client->m_clientType = CLIENTTYPE_KR;
					new PacketKREncryption(client->getClient());
				}
				continue;
			}

			if (received < 5)
			{
				EXC_SET("ping #2");
				if (client->m_client->OnRxPing(buffer, received) == false)
					client->markReadClosed();

				continue;
			}

			// log in the client
			EXC_SET("messages - setup");
			client->m_client->SetConnectType(CONNECT_CRYPT);
			client->m_client->xProcessClientSetup((CEvent*)buffer, received);
			continue;
		}

		client->m_client->m_timeLastEvent = CServTime::GetCurrentTime();

		// first data on a new connection - find out what should come next
		if ( client->m_client->m_Crypt.IsInit() == false )
		{
			EXC_SET("encryption setup");
			ASSERT(received <= sizeof(CEvent));

			CEvent evt;
			memcpy(&evt, buffer, received);

			switch ( client->m_client->GetConnectType() )
			{
				case CONNECT_CRYPT:
					if (received >= 5)
					{
						if (*buffer == XCMD_EncryptionReply && client->isClientKR())
						{
							EXC_SET("encryption reply");

							// receiving response to 0xe3 packet
							size_t iEncKrLen = evt.EncryptionReply.m_len;
							if (received < iEncKrLen)
								break;
							else if (received == iEncKrLen)
								continue;

							received -= iEncKrLen;
							client->m_client->xProcessClientSetup( (CEvent*)(evt.m_Raw + iEncKrLen), received);
							break;
						}
						else
						{
							EXC_SET("encryption detection");

							client->m_client->xProcessClientSetup(&evt, received);
						}
					}
					else
					{
						EXC_SET("ping #3");

						// not enough data to be a real client
						client->m_client->SetConnectType(CONNECT_UNK);
						if (client->m_client->OnRxPing(buffer, received) == false)
						{
							client->markReadClosed();
							continue;
						}
					}
					break;
					
				case CONNECT_HTTP:
					EXC_SET("http message");
					if ( !client->m_client->OnRxWebPageRequest(evt.m_Raw, received) )	
					{
						client->markReadClosed();
						continue;
					}
					break;
					
				case CONNECT_TELNET:
					EXC_SET("telnet message");
					if ( !client->m_client->OnRxConsole(evt.m_Raw, received) )
					{
						client->markReadClosed();
						continue;
					}
					break;
					
				default:
					g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Junk messages with no crypt\n", client->m_id);
					client->markReadClosed();
					continue;
			}

			continue;
		}

		// decrypt the client data and add it to queue
		EXC_SET("decrypt messages");
		client->m_client->m_Crypt.Decrypt(m_decryptBuffer, buffer, received);
		
		if (client->m_receiveBuffer == NULL)
		{
			// create new buffer
			client->m_receiveBuffer = new Packet(m_decryptBuffer, received);
		}
		else
		{
			// append to buffer
			size_t pos = client->m_receiveBuffer->getPosition();
			client->m_receiveBuffer->seek(client->m_receiveBuffer->getLength());
			client->m_receiveBuffer->writeData(m_decryptBuffer, received);
			client->m_receiveBuffer->seek(pos);
		}

		Packet* packet = client->m_receiveBuffer;
		size_t len = packet->getLength() - packet->getPosition();

		EXC_SET("record message");
		xRecordPacket(client->m_client, packet, "client->server");

		// process the message
		EXC_TRYSUB("ProcessMessage");

		while (len > 0 && !client->isClosing())
		{
			BYTE packetID = packet->getData()[packet->getPosition()];
			Packet* handler = m_packets.getHandler(packetID);

			if (handler != NULL)
			{
				size_t packetLength = handler->checkLength(client, packet);
//				DEBUGNETWORK(("Checking length: counted %lu.\n", packetLength));

				//	fall back and delete the packet
				if (packetLength <= 0)
				{
					DEBUGNETWORK(("%lx:Game packet (0x%x) does not match the expected length, waiting for more data...\n", client->id(), packetID));
					break;
				}

				len -= packetLength;

				//	Packet filtering - check if any function triggering is installed
				//		allow skipping the packet which we do not wish to get
				if (client->m_client->xPacketFilter(packet->getData() + packet->getPosition(), packetLength))
				{
					packet->skip(len);
					len = 0;
					break;
				}

				// copy data to handler
				handler->seek();
				for (size_t j = 0; j < packetLength; j++)
				{
					BYTE next = packet->readByte();
					handler->writeByte(next);
				}

				handler->resize(packetLength);
				handler->seek(1);
				handler->onReceive(client);
			}
			else
			{
				//	Packet filtering - check if any function triggering is installed
				//		allow skipping the packet which we do not wish to get
				if (client->m_client->xPacketFilter(packet->getData() + packet->getPosition(), packet->getLength() - packet->getPosition()))
				{
					// packet has been handled by filter but we don't know how big the packet
					// actually is.. we can only assume the entire buffer is used.
					len = 0;
					break;
				}

				// unknown packet.. we could skip 1 byte at a time but this can produce
				// strange behaviours (it's unlikely that only 1 byte is incorrect), so
				// it's best to discard everything we have
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Unknown game packet (0x%x) received.\n", client->id(), packetID);
				packet->skip(len);
				len = 0;
			}
		}

		EXC_CATCHSUB("Network");
		EXC_DEBUGSUB_START;
		TemporaryString dump;
		packet->dump(dump);

		g_Log.EventDebug("%lx:Parsing %s", client->id(), (LPCTSTR)dump);

		client->m_packetExceptions++;
		if (client->m_packetExceptions > 10 && client->m_client != NULL)
		{
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Disconnecting client from account '%s' since it is causing exceptions problems\n", client->id(), client->m_client->GetAccount() ? client->m_client->GetAccount()->GetName() : "");
			client->m_client->addKick(&g_Serv, false);
		}

		EXC_DEBUGSUB_END;

		// delete the buffer once it has been exhausted
		if (len <= 0)
		{
			client->m_receiveBuffer = NULL;
			delete packet;
		}
	}

	EXC_CATCH;
	EXC_DEBUG_START;
	
	EXC_DEBUG_END;
}

int NetworkIn::checkForData(fd_set* storage)
{
//	Berkeley sockets needs nfds to be updated that while in Windows that's ignored
#ifdef _WIN32
#define ADDTOSELECT(_x_)	FD_SET(_x_, storage)
#else
#define ADDTOSELECT(_x_)	{ FD_SET(_x_, storage); if ( _x_ > nfds ) nfds = _x_; }
#endif

	ADDTOCALLSTACK("NetworkIn::checkForData");


	EXC_TRY("CheckForData");
	int nfds = 0;

	EXC_SET("zero");
	FD_ZERO(storage);

#ifndef _WIN32
#ifdef LIBEV_REGISTERMAIN
	if (g_Cfg.m_fUseAsyncNetwork == 0)
#endif
#endif
	{
		EXC_SET("main socket");
		ADDTOSELECT(g_Serv.m_SocketMain.GetSocket());
	}

	EXC_SET("check states");
	for ( long l = 0; l < m_stateCount; l++ )
	{
		EXC_SET("check socket");
		NetState* state = m_states[l];
		if ( state->isInUse() == false )
			continue;

		EXC_SET("cleaning queues");
		for (int i = 0; i < PacketSend::PRI_QTY; i++)
			state->m_queue[i].clean();

		EXC_SET("check closing");
		if (state->isClosing())
		{
			if (state->isClosed() == false)
			{
				EXC_SET("check pending data");
				if (state->hasPendingData())
				{
					if (state->needsFlush() == false)
					{
						DEBUGNETWORK(("%lx:Flushing data for client.\n", state->id()));

						EXC_SET("flush data");
						g_NetworkOut.flush(state->getClient());
					}
					else
					{
						DEBUGNETWORK(("%lx:Waiting for data to be flushed.\n", state->id()));
					}
					continue;
				}
			
				EXC_SET("mark closed");
				state->markReadClosed();
				if (g_NetworkOut.isActive() == false)
					state->markWriteClosed();
			}

			EXC_SET("check closed");
			if (state->isClosed() && state->isSendingAsync() == false)
			{
				EXC_SET("clear socket");
				DEBUGNETWORK(("%lx:Client is being cleared since marked to close.\n", state->id()));
				state->clear();
			}

			continue;
		}

		if (state->m_socket.IsOpen())
		{
			EXC_SET("add to select");
			ADDTOSELECT(state->m_socket.GetSocket());
		}
	}

	EXC_SET("prepare timeout");
	timeval Timeout;	// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000

	EXC_SET("perform select");
	return select(nfds+1, storage, NULL, NULL, &Timeout);

	EXC_CATCH;
	EXC_DEBUG_START;
	
	EXC_DEBUG_END;
	return 0;

#undef ADDTOSELECT
}

void NetworkIn::acceptConnection(void)
{
	ADDTOCALLSTACK("NetworkIn::acceptConnection");

	EXC_TRY("acceptConnection");
	CSocketAddress client_addr;

	DEBUGNETWORK(("Receiving new connection\n"));

	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (( h >= 0 ) && ( h != INVALID_SOCKET ))
	{
		EXC_SET("ip history");
		
		DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
		HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
		long maxIp = g_Cfg.m_iConnectingMaxIP;
		long climaxIp = g_Cfg.m_iClientsMaxIP;

		DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%ld, pings=%ld, connecting=%ld, connected=%ld]\n", 
			ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

		//	ip is blocked
		if ( ip.checkPing() ||
			// or too much connect tries from this ip
			( maxIp && ( ip.m_connecting > maxIp )) ||
			// or too much clients from this ip
			( climaxIp && ( ip.m_connected > climaxIp ))
			)
		{
			EXC_SET("rejecting");
			DEBUGNETWORK(("Closing incoming connection [max ip=%ld, clients max ip=%ld).\n", maxIp, climaxIp));

			CLOSESOCKET(h);

			if (ip.m_blocked)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (Blocked IP)\n", (LPCTSTR)client_addr.GetAddrStr());
			else if ( maxIp && ip.m_connecting > maxIp )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CONNECTINGMAXIP reached %ld/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_connecting, maxIp);
			else if ( climaxIp && ip.m_connected > climaxIp )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAXIP reached l%d/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_connected, climaxIp);
			else if ( ip.m_pings >= NETHISTORY_MAXPINGS )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (MAXPINGS reached %ld/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_pings, NETHISTORY_MAXPINGS);
			else
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected.\n", (LPCTSTR)client_addr.GetAddrStr());
		}
		else
		{
			EXC_SET("detecting slot");
			long slot = getStateSlot();
			if ( slot == -1 )			// we do not have enough empty slots for clients
			{
				EXC_SET("no slot ready");
				DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already.\n"));

				CLOSESOCKET(h);

				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", (LPCTSTR)client_addr.GetAddrStr());
			}
			else
			{
				DEBUGNETWORK(("%lx:Allocated slot for client (%ld).\n", slot, (long)h));

				EXC_SET("assigning slot");
				m_states[slot]->init(h, client_addr);

				DEBUGNETWORK(("%lx:State initialised, registering client instance.\n", slot));

				EXC_SET("recording client");
				if (m_states[slot]->m_client != NULL)
					m_clients.InsertHead(m_states[slot]->getClient());

				DEBUGNETWORK(("%lx:Client successfully initialised.\n", slot));
			}
		}
	}
	EXC_CATCH;
}

long NetworkIn::getStateSlot(long startFrom)
{
	ADDTOCALLSTACK("NetworkIn::getStateSlot");

	if ( startFrom == -1 )
		startFrom = m_lastGivenSlot + 1;

	//	give ordered slot number, each time incrementing by 1 for easier log view
	for ( long l = startFrom; l < m_stateCount; l++ )
	{
		if (m_states[l]->isInUse())
			continue;

		return ( m_lastGivenSlot = l );
	}

	//	we did not find empty slots till the end, try rescan from beginning
	if ( startFrom > 0 )
		return getStateSlot(0);

	//	no empty slots exists, arbitrary too many clients
	return -1;
}

void NetworkIn::periodic(void)
{
	ADDTOCALLSTACK("NetworkIn::periodic");

	EXC_TRY("periodic");
	// check if max connecting limit is obeyed
	long connectingMax = g_Cfg.m_iConnectingMax;
	if (connectingMax > 0)
	{
		EXC_SET("limiting connecting clients");
		long connecting = 0;

		ClientIterator clients(this);
		for (const CClient* client = clients.next(); client != NULL; client = clients.next())
		{
			if (client->IsConnecting())
			{
				if (++connecting > connectingMax)
				{
					DEBUGNETWORK(("%lx:Closing client since '%ld' connecting overlaps '%ld'\n", client->m_net->id(), connecting, connectingMax));

					client->m_net->markReadClosed();
				}
			}
			if (connecting > connectingMax)
			{
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%ld clients in connect mode (max %ld), closing %ld\n", connecting, connectingMax, connecting - connectingMax);
			}
		}
	}

	// tick the ip history, remove some from the list
	EXC_SET("ticking history");
	m_ips.tick();

	// resize m_states to account for m_iClientsMax changes
	long max = g_Cfg.m_iClientsMax;
	if (max > m_stateCount)
	{
		EXC_SET("increasing network state size");
		DEBUGNETWORK(("Increasing number of client slots from %ld to %ld\n", m_stateCount, max));

		// reallocate state buffer to accomodate additional clients
		long prevCount = m_stateCount;
		NetState** prevStates = m_states;

		NetState** newStates = new NetState*[max];
		memcpy(newStates, prevStates, m_stateCount * sizeof(NetState*));
		for (long l = prevCount; l < max; l++)
			newStates[l] = new NetState(l);
		
		m_states = newStates;
		m_stateCount = max;

		// destroy previous states
		delete[] prevStates;
	}
	else if (max < m_stateCount)
	{
		EXC_SET("decreasing network state size");
		DEBUGNETWORK(("Decreasing number of client slots from %ld to %ld\n", m_stateCount, max));

		// move used slots to free spaces if possible
		defragSlots(max);

		// delete excess states but leave array intact
		for (long l = max; l < m_stateCount; l++)
		{
			delete m_states[l];
			m_states[l] = NULL;
		}

		m_stateCount = max;
	}

	EXC_CATCH;
}

void NetworkIn::defragSlots(long fromSlot)
{
	ADDTOCALLSTACK("NetworkIn::defragSlots");

	long l = 0;
	long nextUsedSlot = fromSlot - 1;

	for (l = 0; l < m_stateCount; l++)
	{
		// don't interfere with in-use states
		if (m_states[l] != NULL && m_states[l]->isInUse())
			continue;

		// find next used slot
		bool slotFound = false;
		while (slotFound == false)
		{
			if (++nextUsedSlot >= m_stateCount)
				break;

			NetState* state = m_states[nextUsedSlot];
			if (state != NULL && state->isInUse())
				slotFound = true;
		}

		// no more slots to be moved
		if (slotFound == false)
			break;

		if (nextUsedSlot != l)
		{
			DEBUGNETWORK(("Moving client '%lx' to slot '%lx'.\n", nextUsedSlot, l));

			// swap states
			NetState* usedSlot = m_states[nextUsedSlot];
			usedSlot->setId(l);

			m_states[nextUsedSlot] = m_states[l];
			m_states[l] = usedSlot;
		}
	}
}


/***************************************************************************
 *
 *
 *	class NetworkOut			Networking thread used for queued sending outgoing packets
 *
 *
 ***************************************************************************/

NetworkOut::NetworkOut(void) : AbstractSphereThread("NetworkOut", IThread::RealTime)
{
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);

	m_encryptBuffer = new BYTE[MAX_BUFFER];
}

NetworkOut::~NetworkOut(void)
{
	if (m_encryptBuffer != NULL)
	{
		delete[] m_encryptBuffer;
		m_encryptBuffer = NULL;
	}
}

void NetworkOut::tick(void)
{
	ADDTOCALLSTACK("NetworkOut::tick");
	ProfileTask networkTask(PROFILE_NETWORK_TX);

	if (g_Serv.m_iExitFlag || g_Serv.m_iModeCode == SERVMODE_Exiting)
	{
		setPriority(IThread::Highest);
		return;
	}

	static unsigned char iCount = 0;
	EXC_TRY("NetworkOut");

	iCount++;

	bool toProcess[PacketSend::PRI_QTY];
	if (isActive() == false)
	{
		// process queues faster in single-threaded mode
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= true;
		toProcess[PacketSend::PRI_NORMAL]	= true;
		toProcess[PacketSend::PRI_LOW]		= ((iCount % 2) == 1);
		toProcess[PacketSend::PRI_IDLE]		= ((iCount % 4) == 3);
	}
	else
	{
		// throttle rate of sending in multi-threaded mode
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= ((iCount %  2) == 1);
		toProcess[PacketSend::PRI_NORMAL]	= ((iCount %  4) == 3);
		toProcess[PacketSend::PRI_LOW]		= ((iCount %  8) == 7);
		toProcess[PacketSend::PRI_IDLE]		= ((iCount % 16) == 15);

		EXC_SET("flush");
		proceedFlush();
	}

	int packetsSent = 0;
	
	SafeClientIterator clients;
	while (CClient* client = clients.next())
	{
		const NetState* state = client->GetNetState();
		ASSERT(state != NULL);

		EXC_SET("highest");
		if (toProcess[PacketSend::PRI_HIGHEST] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_HIGHEST);

		EXC_SET("high");
		if (toProcess[PacketSend::PRI_HIGH] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_HIGH);

		EXC_SET("normal");
		if (toProcess[PacketSend::PRI_NORMAL] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_NORMAL);

		EXC_SET("low");
		if (toProcess[PacketSend::PRI_LOW] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_LOW);

		EXC_SET("idle");
		if (toProcess[PacketSend::PRI_IDLE] && state->isWriteClosed() == false)
			packetsSent += proceedQueue(client, PacketSend::PRI_IDLE);

		EXC_SET("async");
		if (state->isWriteClosed() == false)
			packetsSent += proceedQueueAsync(client);

		if (state->isWriteClosed() == false)
			proceedQueueBytes(client);
	}

	// increase priority during 'active' periods
	if (packetsSent > 0)
		setPriority(IThread::RealTime);
	else
		setPriority(IThread::Highest);

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("ActiveThread=%d, TickCount=%d\n", isActive()? 1 : 0, iCount);
	EXC_DEBUG_END;
}

void NetworkOut::schedule(const PacketSend* packet, bool appendTransaction)
{
	ADDTOCALLSTACK("NetworkOut::schedule");

	ASSERT(packet != NULL);
	scheduleOnce(packet->clone(), appendTransaction);
}

void NetworkOut::scheduleOnce(PacketSend* packet, bool appendTransaction)
{
	ADDTOCALLSTACK("NetworkOut::scheduleOnce");

	ASSERT(packet != NULL);
	NetState* state = packet->m_target;
	ASSERT(state != NULL);

	// don't bother queuing packets for invalid sockets
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
	{
		delete packet;
		return;
	}

	if (state->m_pendingTransaction != NULL && appendTransaction)
		state->m_pendingTransaction->push_back(packet);
	else
		scheduleOnce(new SimplePacketTransaction(packet));
}

void NetworkOut::scheduleOnce(PacketTransaction* transaction)
{
	ADDTOCALLSTACK("NetworkOut::scheduleOnce[2]");

	ASSERT(transaction != NULL);
	NetState* state = transaction->getTarget();
	ASSERT(state != NULL);

	// don't bother queuing packets for invalid sockets
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
	{
		delete transaction;
		return;
	}

	long priority = transaction->getPriority();
	ASSERT(priority >= PacketSend::PRI_IDLE && priority < PacketSend::PRI_QTY);

#ifdef NETWORK_MAXQUEUESIZE
	// limit by number of packets to be in queue
	if (priority > PacketSend::PRI_IDLE)
	{
		size_t maxClientPackets = NETWORK_MAXQUEUESIZE;
		if (maxClientPackets > 0)
		{
			if (state->m_queue[priority].size() >= maxClientPackets)
			{
//				DEBUGNETWORK(("%x:Packet decreased priority due to overal amount %d overlapping %d.\n", state->id(), state->m_queue[priority].size(), maxClientPackets));

				transaction->setPriority(priority-1);
				scheduleOnce(transaction);
				return;
			}
		}
	}
#endif

	state->m_queue[priority].push(transaction);
}

void NetworkOut::flushAll(void)
{
	ADDTOCALLSTACK("NetworkOut::flushAll");

	SafeClientIterator clients;
	while (CClient* client = clients.next(true))
		flush(client);
}

void NetworkOut::flush(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::flush");

	ASSERT(client != NULL);
	
	NetState* state = client->GetNetState();
	ASSERT(state != NULL);
	if (state->isInUse(client) == false)
		return;

	// flushing is not thread-safe, and can only be performed by the NetworkOut thread
	if (isActive() && isCurrentThread() == false)
	{
		// mark state to be flushed
		state->markFlush(true);
	}
	else
	{
		for (int priority = 0; priority < PacketSend::PRI_QTY && state->isWriteClosed() == false; priority++)
			proceedQueue(client, priority);

		if (state->isWriteClosed() == false)
			proceedQueueAsync(client);

		if (state->isWriteClosed() == false)
			proceedQueueBytes(client);

		state->markFlush(false);
	}
}

void NetworkOut::proceedFlush(void)
{
	ADDTOCALLSTACK("NetworkOut::proceedFlush");

	SafeClientIterator clients;
	while (CClient* client = clients.next(true))
	{
		NetState* state = client->GetNetState();
		ASSERT(state != NULL);

		if (state->isWriteClosed())
			continue;

		if (state->needsFlush())
			flush(client);

		if (state->isClosing() && state->hasPendingData() == false)
			state->markWriteClosed();
	}
}

int NetworkOut::proceedQueue(CClient* client, long priority)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueue");

	long maxClientPackets = NETWORK_MAXPACKETS;
	long maxClientLength = NETWORK_MAXPACKETLEN;
	CServTime time = CServTime::GetCurrentTime();

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	if (state->isWriteClosed() || (state->m_queue[priority].empty() && state->m_currentTransaction == NULL))
		return 0;

	long length = 0;

	// send N transactions from the queue
	int i = 0;
	for (i = 0; i < maxClientPackets; i++)
	{
		// select next transaction
		while (state->m_currentTransaction == NULL)
		{
			if (state->m_queue[priority].empty())
				break;

			state->m_currentTransaction = state->m_queue[priority].front();
			state->m_queue[priority].pop();
		}

		PacketTransaction* transaction = state->m_currentTransaction;
		if (transaction == NULL)
			break;

		// acquire next packet from the transaction
		PacketSend* packet = transaction->empty()? NULL : transaction->front();
		transaction->pop();

		if (transaction->empty())
		{
			// no more packets left in the transacton, clear it so we can move on to the next one
			state->m_currentTransaction = NULL;
			delete transaction;
		}

		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			// don't count this towards the limit, allow an extra packet to be processed
			delete packet;
			maxClientPackets++;
			continue;
		}

		EXC_TRY("proceedQueue");
		length += packet->getLength();

		EXC_SET("sending");
		if (sendPacket(client, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
			break;
		}

		client->m_timeLastSend = time;

		EXC_SET("check length");
		if (length > maxClientLength)
		{
//			DEBUGNETWORK(("%x:Packets sending stopped at %d packet due to overall length %d overlapping %d.\n", state->id(), i, length, maxClientLength));

			break;
		}

		EXC_CATCH;
		EXC_DEBUG_START;
		g_Log.EventDebug("id='%lx', pri='%ld', packet '%d' of '%ld' to send, length '%ld' of '%ld'\n",
			state->id(), priority, i, maxClientPackets, length, maxClientLength);
		EXC_DEBUG_END;
	}
	return i;
}

int NetworkOut::proceedQueueAsync(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueueAsync");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	state->m_asyncQueue.clean();
	if (state->m_asyncQueue.empty() || state->isSendingAsync())
		return 0;

	// get next packet
	PacketSend* packet = NULL;
	
	while (state->m_asyncQueue.empty() == false)
	{
		packet = state->m_asyncQueue.front();
		state->m_asyncQueue.pop();

		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			if (packet != NULL)
			{
				delete packet;
				packet = NULL;
			}

			continue;
		}

		break;
	}

	// start sending
	if (packet != NULL)
	{
		if (sendPacketNow(client, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
		}

		return 1;
	}

	return 0;
}

void NetworkOut::proceedQueueBytes(CClient* client)
{
	ADDTOCALLSTACK("NetworkOut::proceedQueueBytes");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);
	
	if (state->isWriteClosed() || state->m_byteQueue.GetDataQty() <= 0)
		return;

	int ret = sendBytesNow(client, state->m_byteQueue.RemoveDataLock(), state->m_byteQueue.GetDataQty());
	if (ret > 0)
	{
		state->m_byteQueue.RemoveDataAmount(ret);
	}
	else if (ret < 0)
	{
		state->clearQueues();
		state->markWriteClosed();
	}
}

void NetworkOut::onAsyncSendComplete(NetState* state, bool success)
{
	ADDTOCALLSTACK("NetworkOut::onAsyncSendComplete");

	//DEBUGNETWORK(("AsyncSendComplete\n"));
	ASSERT(state != NULL);

	state->setSendingAsync(false);
	if (success == false)
		return;

	if (proceedQueueAsync(state->getClient()) != 0)
		proceedQueueBytes(state->getClient());
}

bool NetworkOut::sendPacket(CClient* client, PacketSend* packet)
{
	ADDTOCALLSTACK("NetworkOut::sendPacket");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	// only allow high-priority packets to be sent to queued for closed sockets
	if (state->canReceive(packet) == false)
	{
		delete packet;
		return false;
	}

	if (state->isAsyncMode())
	{
		state->m_asyncQueue.push(packet);
		return true;
	}

	return sendPacketNow(client, packet);
}

#ifdef _WIN32

void CALLBACK SendCompleted(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	UNREFERENCED_PARAMETER(dwFlags);
	ADDTOCALLSTACK("SendCompleted");

	NetState* state = reinterpret_cast<NetState *>(lpOverlapped->hEvent);
	if (state == NULL)
	{
		DEBUGNETWORK(("Async i/o operation completed without client context.\n"));
		return;
	}

	if (dwError != 0)
	{
		DEBUGNETWORK(("%lx:Async i/o operation completed with error code 0x%x, %ld bytes sent.\n", state->id(), dwError, cbTransferred));
	}
	//else
	//{
	//	DEBUGNETWORK(("%lx:Async i/o operation completed successfully, %ld bytes sent.\n", state->id(), cbTransferred));
	//}

	g_NetworkOut.onAsyncSendComplete(state, dwError == 0 && cbTransferred > 0);
}

#endif

bool NetworkOut::sendPacketNow(CClient* client, PacketSend* packet)
{
	ADDTOCALLSTACK("NetworkOut::sendPacketNow");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	EXC_TRY("proceedQueue");

	xRecordPacket(client, packet, "server->client");

	EXC_SET("send trigger");
	if (packet->onSend(client))
	{
		BYTE* sendBuffer = NULL;
		DWORD sendBufferLength = 0;

		if (state->m_client == NULL)
		{
			DEBUGNETWORK(("%lx:Sending packet to closed client?\n", state->id()));

			sendBuffer = packet->getData();
			sendBufferLength = packet->getLength();
		}
		else
		{
			if (client->GetConnectType() == CONNECT_GAME)
			{
				// game clients require encryption
				EXC_SET("compress and encrypt");

				// compress
				size_t compressLength = client->xCompress(m_encryptBuffer, packet->getData(), packet->getLength());

				// encrypt
				if (client->m_Crypt.GetEncryptionType() == ENC_TFISH)
					client->m_Crypt.Encrypt(m_encryptBuffer, m_encryptBuffer, compressLength);

				sendBuffer = m_encryptBuffer;
				sendBufferLength = compressLength;
			}
			else
			{
				// other clients expect plain data
				sendBuffer = packet->getData();
				sendBufferLength = packet->getLength();
			}
		}
		
		if ( g_Cfg.m_fUseExtraBuffer )
		{
			// queue packet data
			state->m_byteQueue.AddNewData(sendBuffer, sendBufferLength);
		}
		else
		{
			// send packet data now
			size_t totalSent = 0;

			do
			{
				// a single send attempt may not send the entire buffer, so we need to
				// loop through this process until we have sent the expected number of bytes
				int sent = sendBytesNow(client, sendBuffer + totalSent, sendBufferLength - totalSent);
				if (sent > 0)
				{
					totalSent += sent;
				}
				else if (sent == 0 && totalSent == 0)
				{
					// if no bytes were sent then we can try to ignore the error
					// by re-queueing the packet, but this is only viable if no
					// data has been sent (if part of the packet has gone, we have
					// no choice but to disconnect the client since we'll be oos)

					// WARNING: scheduleOnce is intended to be used by the main
					// thread, and is likely to cause stability issues when called
					// from here!
					DEBUGNETWORK(("%lx:Send failure occurred with a non-critical error. Requeuing packet may affect stability.\n", state->id()));
					scheduleOnce(packet, true);
					return true;
				}
				else
				{
					// critical error occurred
					delete packet;
					return false;
				}
			}
			while (totalSent < sendBufferLength);
		}

		EXC_SET("sent trigger");
		packet->onSent(client);
	}

	delete packet;
	return true;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%lx', packet '0x%x', length '%" FMTSIZE_T "'\n",
		state->id(), *packet->getData(), packet->getLength());
	EXC_DEBUG_END;
	return false;
}

int NetworkOut::sendBytesNow(CClient* client, const BYTE* data, DWORD length)
{
	ADDTOCALLSTACK("NetworkOut::sendBytesNow");

	NetState* state = client->GetNetState();
	ASSERT(state != NULL);

	EXC_TRY("sendBytesNow");
	int ret = 0;

	// send data
	EXC_SET("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = length;
		state->m_bufferWSA.buf = (CHAR*)data;

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, SendCompleted) == 0)
		{
			ret = bytesSent;
			state->setSendingAsync(true);
		}
		else
			ret = 0;
	}
	else
#endif
	{
		// send via standard api
		ret = state->m_socket.Send(data, length);
	}

	// check for error
	if (ret <= 0)
	{
		EXC_SET("error parse");
		int errCode = CGSocket::GetLastError(true);

#ifdef _WIN32
		if (state->isAsyncMode() && errCode == WSA_IO_PENDING)
		{
			// safe to ignore this, data has actually been sent
			ret = length;
		}
		else if (state->isAsyncMode() == false && errCode == WSAEWOULDBLOCK)
#else
		if (errCode == EAGAIN || errCode == EWOULDBLOCK)
#endif
		{
			// send failed but it is safe to ignore and try again later
			ret = 0;
		}
#ifdef _WIN32
		else if (errCode == WSAECONNRESET || errCode == WSAECONNABORTED)
#else
		else if (errCode == ECONNRESET || errCode == ECONNABORTED)
#endif
		{
			// connection has been lost, client should be cleared
			ret = INT_MIN;
		}
		else
		{
			EXC_SET("unexpected connection error - delete packet");

			if (state->isClosing() == false)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:TX Error %d\n", state->id(), errCode);

#ifndef _WIN32
			return INT_MIN;
#else
			ret = 0;
#endif
		}
	}

	if (ret > 0)
		CurrentProfileData.Count(PROFILE_DATA_TX, ret);

	return ret;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%lx', packet '0x%x', length '%lu'\n", state->id(), *data, length);
	EXC_DEBUG_END;
	return INT_MIN;
}

#else

/***************************************************************************
 *
 *
 *	void AddSocketToSet				Add a socket to an fd_set
 *
 *
 ***************************************************************************/
inline void AddSocketToSet(fd_set& fds, SOCKET socket, int& count)
{
#ifdef _WIN32
	UNREFERENCED_PARAMETER(count);
	FD_SET(socket, &fds);
#else
	FD_SET(socket, &fds);
	if (socket > count)
		count = socket;
#endif
}

/***************************************************************************
 *
 *
 *	void GenerateNetworkThreadName				Generate network thread name
 *
 *
 ***************************************************************************/
const char * GenerateNetworkThreadName(size_t id)
{
	char * name = new char[25];
	sprintf(name, "NetworkThread #%" FMTSIZE_T, id);
	return name;
}

#ifdef _WIN32

/***************************************************************************
 *
 *
 *	void SendCompleted			Winsock event handler for when async operation completes
 *
 *
 ***************************************************************************/
void CALLBACK SendCompleted(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	UNREFERENCED_PARAMETER(dwFlags);
	ADDTOCALLSTACK("SendCompleted");

	NetState* state = reinterpret_cast<NetState*>(lpOverlapped->hEvent);
	if (state == NULL)
	{
		DEBUGNETWORK(("Async i/o operation completed without client context.\n"));
		return;
	}

	NetworkThread* thread = state->getParentThread();
	if (thread == NULL)
	{
		DEBUGNETWORK(("%lx:Async i/o operation completed.\n", state->id()));
		return;
	}

	if (dwError != 0)
	{
		DEBUGNETWORK(("%lx:Async i/o operation completed with error code 0x%x, %ld bytes sent.\n", state->id(), dwError, cbTransferred));
	}
	//else
	//{
	//	DEBUGNETWORK(("%lx:Async i/o operation completed successfully, %ld bytes sent.\n", state->id(), cbTransferred));
	//}

	thread->onAsyncSendComplete(state, dwError == 0 && cbTransferred > 0);
}

#endif

/***************************************************************************
 *
 *
 *	class NetworkManager            Network manager, handles incoming connections and
 *                                  spawns child threads to process i/o
 *
 *
 ***************************************************************************/
NetworkManager::NetworkManager(void)
{
	m_states = NULL;
	m_stateCount = 0;
	m_lastGivenSlot = (std::numeric_limits<size_t>::max)();
	m_isThreaded = false;
}

NetworkManager::~NetworkManager(void)
{
	stop();
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); )
	{
		delete *it;
		it = m_threads.erase(it);
	}
}

void NetworkManager::createNetworkThreads(size_t count)
{
	// create n threads to handle client i/o
	ADDTOCALLSTACK("NetworkManager::createNetworkThreads");

	// always need at least 1
	if (count < 1)
		count = 1;

	// limit the number of threads to avoid stupid values, the maximum is calculated
	// to allow a maximum of 32 clients per thread at full load
	size_t maxThreads = maximum((FD_SETSIZE / 32), 1);
	if (count > maxThreads)
	{
		count = maxThreads;
		g_Log.Event(LOGL_WARN|LOGM_INIT, "Too many network threads requested. Reducing number to %" FMTSIZE_T ".\n", count);
	}

	ASSERT(m_threads.empty());
	for (size_t i = 0; i < count; ++i)
		m_threads.push_back(new NetworkThread(*this, i));
}

NetworkThread* NetworkManager::selectBestThread(void)
{
	// select the most suitable thread for handling a
	// new client
	ADDTOCALLSTACK("NetworkManager::selectBestThread");

	NetworkThread* bestThread = NULL;
	size_t bestThreadSize = (std::numeric_limits<size_t>::max)();
	DEBUGNETWORK(("Searching for a suitable thread to handle a new client..\n"));

	// search for quietest thread
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		if ((*it)->getClientCount() < bestThreadSize)
		{
			bestThread = *it;
			bestThreadSize = bestThread->getClientCount();
		}
	}

	ASSERT(bestThread != NULL);
	DEBUGNETWORK(("Selected thread #%" FMTSIZE_T ".\n", bestThread->id()));
	return bestThread;
}

void NetworkManager::assignNetworkState(NetState* state)
{
	// assign a state to a thread
	ADDTOCALLSTACK("NetworkManager::assignNetworkState");

	NetworkThread* thread = selectBestThread();
	ASSERT(thread != NULL);
	thread->assignNetworkState(state);
}

bool NetworkManager::checkNewConnection(void)
{
	// check for any new connections
	ADDTOCALLSTACK("NetworkManager::checkNewConnection");

	SOCKET mainSocket = g_Serv.m_SocketMain.GetSocket();

	fd_set fds;
	int count = 0;

	FD_ZERO(&fds);
	AddSocketToSet(fds, mainSocket, count);

	timeval Timeout;		// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000

	count = select(count+1, &fds, NULL, NULL, &Timeout);
	if (count <= 0)
		return false;

	return FD_ISSET(mainSocket, &fds) != 0;
}

void NetworkManager::acceptNewConnection(void)
{
	// accept new connection
	ADDTOCALLSTACK("NetworkManager::acceptNewConnection");

	EXC_TRY("acceptNewConnection");
	CSocketAddress client_addr;

	DEBUGNETWORK(("Receiving new connection.\n"));

	// accept socket connection
	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (h == INVALID_SOCKET)
		return;

	// check ip history
	EXC_SET("ip history");

	DEBUGNETWORK(("Retrieving IP history for '%s'.\n", client_addr.GetAddrStr()));
	HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
	long maxIp = g_Cfg.m_iConnectingMaxIP;
	long climaxIp = g_Cfg.m_iClientsMaxIP;

	DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%ld, pings=%ld, connecting=%ld, connected=%ld]\n",
		ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

	// check if ip is allowed to connect
	if ( ip.checkPing() ||									// check for ip ban
			( maxIp > 0 && ip.m_connecting > maxIp ) ||		// check for too many connecting
			( climaxIp > 0 && ip.m_connected > climaxIp ) )	// check for too many connected
	{
		EXC_SET("rejected");
		DEBUGNETWORK(("Closing incoming connection [max ip=%ld, clients max ip=%ld].\n", maxIp, climaxIp));

		CLOSESOCKET(h);

		if (ip.m_blocked)
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (Blocked IP)\n", (LPCTSTR)client_addr.GetAddrStr());
		else if ( maxIp && ip.m_connecting > maxIp )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CONNECTINGMAXIP reached %ld/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_connecting, maxIp);
		else if ( climaxIp && ip.m_connected > climaxIp )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAXIP reached %ld/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_connected, climaxIp);
		else if ( ip.m_pings >= NETHISTORY_MAXPINGS )
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (MAXPINGS reached %ld/%ld)\n", (LPCTSTR)client_addr.GetAddrStr(), ip.m_pings, static_cast<long>(NETHISTORY_MAXPINGS) );
		else
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected.\n", (LPCTSTR)client_addr.GetAddrStr());

		return;
	}

	// select an empty slot
	EXC_SET("detecting slot");
	NetState* state = findFreeSlot();
	if (state == NULL)
	{
		// not enough empty slots
		EXC_SET("no slot available");
		DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already connected.\n"));
		CLOSESOCKET(h);

		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected. (CLIENTMAX reached)\n", (LPCTSTR)client_addr.GetAddrStr());
		return;
	}

	DEBUGNETWORK(("%lx:Allocated slot for client (%lu).\n", state->id(), (unsigned long)h));

	// assign slot
	EXC_SET("assigning slot");
	state->init(h, client_addr);

	DEBUGNETWORK(("%lx:State initialised, registering client instance.\n", state->id()));

	EXC_SET("recording client");
	if (state->getClient() != NULL)
		m_clients.InsertHead(state->getClient());

	EXC_SET("assigning thread");
	DEBUGNETWORK(("%lx:Selecting a thread to assign to.\n", state->id()));
	assignNetworkState(state);

	DEBUGNETWORK(("%lx:Client successfully initialised.\n", state->id()));

	EXC_CATCH;
}

NetState* NetworkManager::findFreeSlot(size_t start)
{
	// find slot for new client
	ADDTOCALLSTACK("NetworkManager::findFreeSlot");

	// start searching from the last given slot to try and give incremental
	// ids to clients
	if (start == (std::numeric_limits<size_t>::max)())
		start = m_lastGivenSlot + 1;

	// find unused slot
	for (size_t i = start; i < m_stateCount; ++i)
	{
		if (m_states[i]->isInUse())
			continue;

		m_lastGivenSlot = i;
		return m_states[i];
	}

	if (start == 0)
		return NULL;

	// since we started somewhere in the middle, go back to the start
	return findFreeSlot(0);
}

void NetworkManager::start(void)
{
	DEBUGNETWORK(("Registering packets...\n"));
	m_packets.registerStandardPackets();

	// create network states
	ASSERT(m_states == NULL);
	ASSERT(m_stateCount == 0);
	m_states = new NetState*[g_Cfg.m_iClientsMax];
	for (size_t l = 0; l < g_Cfg.m_iClientsMax; l++)
		m_states[l] = new NetState(l);
	m_stateCount = g_Cfg.m_iClientsMax;

	DEBUGNETWORK(("Created %" FMTSIZE_T " network slots (system limit of %d clients)\n", m_stateCount, FD_SETSIZE));

	// create network threads
	createNetworkThreads(g_Cfg.m_iNetworkThreads);

	m_isThreaded = g_Cfg.m_iNetworkThreads > 0;
	if (isThreaded())
	{
		// start network threads
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->start();

		DEBUGNETWORK(("Started %" FMTSIZE_T " network threads.\n", m_threads.size()));
	}
	else
	{
		// initialise network threads
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->onStart();
	}
}

void NetworkManager::stop(void)
{
	// terminate child threads
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->waitForClose();
}

void NetworkManager::tick(void)
{
	ADDTOCALLSTACK("NetworkManager::tick");

	EXC_TRY("Tick");
	for (size_t i = 0; i < m_stateCount; ++i)
	{
		NetState* state = m_states[i];
		if (state->isInUse() == false)
			continue;

		// clean packet queue entries
		EXC_SET("cleaning queues");
		for (int priority = 0; priority < PacketSend::PRI_QTY; ++priority)
			state->m_queue[priority].clean();

		EXC_SET("check closing");
		if (state->isClosing() == false)
		{
#ifdef _MTNETWORK
			// if data is queued whilst the thread is in the middle of processing then the signal
			// sent from NetworkOutput::QueuePacketTransaction can be ignored
			// the safest solution to this is to send additional signals from here
			NetworkThread* thread = state->getParentThread();
			if (thread != NULL && state->hasPendingData() && thread->getPriority() == IThread::Disabled)
				thread->awaken();
#endif
			continue;
		}

		// the state is closing, see if it can be cleared
		if (state->isClosed() == false)
		{
			EXC_SET("check pending data");
			if (state->hasPendingData())
			{
				// data is waiting to be sent, so flag that a flush is needed
				EXC_SET("flush data");
				if (state->needsFlush() == false)
				{
					DEBUGNETWORK(("%lx:Flushing data for client.\n", state->id()));
					state->markFlush(true);
				}
				continue;
			}
			
			// state is finished with as far as we're concerned
			EXC_SET("mark closed");
			state->markReadClosed();
		}

		EXC_SET("check closed");
		if (state->isClosed() && state->isSendingAsync() == false)
		{
			EXC_SET("clear socket");
			DEBUGNETWORK(("%lx:Client is being cleared since marked to close.\n", state->id()));
			state->clear();
		}
	}

	// tick ip history
	m_ips.tick();
	
	// tick child threads, if single-threaded mode
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		NetworkThread* thread = *it;
		if (thread->isActive() == false)
			thread->tick();
	}

	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_DEBUG_END;
}

void NetworkManager::processAllInput(void)
{
	// process network input
	ADDTOCALLSTACK("NetworkManager::processAllInput");
	if (checkNewConnection())
		acceptNewConnection();

	if (isInputThreaded() == false)
	{
		// force each thread to process input (NOT THREADSAFE)
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->processInput();
	}
}

void NetworkManager::processAllOutput(void)
{
	// process network output
	ADDTOCALLSTACK("NetworkManager::processAllOutput");

	if (isOutputThreaded() == false)
	{
		// force each thread to process output (NOT THREADSAFE)
		for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
			(*it)->processOutput();
	}
}

void NetworkManager::flushAllClients(void)
{
	// flush data for every client
	ADDTOCALLSTACK("NetworkManager::flushAllClients");
	for (size_t i = 0; i < m_stateCount; ++i)
	{
		NetState* state = m_states[i];
		if (state->isInUse() && state->isClosed() == false)
			state->markFlush(true);
	}
}

/***************************************************************************
 *
 *
 *	class NetworkThread             Handles i/o for a set of clients, owned
 *                                  by a single NetworkManager instance
 *
 *
 ***************************************************************************/
NetworkThread::NetworkThread(NetworkManager& manager, size_t id)
	: AbstractSphereThread(GenerateNetworkThreadName(id), IThread::Disabled),
	m_manager(manager), m_id(id)
{
}

NetworkThread::~NetworkThread(void)
{
	// thread name was allocated by GenerateNetworkThreadName, so should be delete[]'d
	delete[] getName();
}

void NetworkThread::assignNetworkState(NetState* state)
{
	ADDTOCALLSTACK("NetworkThread::assignNetworkState");
	m_assignQueue.push(state);
	if (getPriority() == IThread::Disabled)
		awaken();
}

void NetworkThread::checkNewStates(void)
{
	// check for states that have been assigned but not moved to our list
	ADDTOCALLSTACK("NetworkThread::checkNewStates");
	ASSERT(!isActive() || isCurrentThread());

	while (m_assignQueue.empty() == false)
	{
		NetState* state = m_assignQueue.front();
		m_assignQueue.pop();

		ASSERT(state != NULL);
		state->setParentThread(this);
		m_states.push_back(state);
	}
}

void NetworkThread::dropInvalidStates(void)
{
	// check for states in our list that don't belong to us
	ADDTOCALLSTACK("NetworkThread::dropInvalidStates");
	ASSERT(!isActive() || isCurrentThread());

	for (NetworkStateList::iterator it = m_states.begin(); it != m_states.end(); )
	{
		NetState* state = *it;
		if (state->getParentThread() != this)
		{
			// state has been unassigned or reassigned elsewhere
			it = m_states.erase(it);
		}
		else if (state->isInUse() == false)
		{
			// state is invalid
			state->setParentThread(NULL);
			it = m_states.erase(it);
		}
		else
		{
			// state is good
			++it;
		}
	}
}

void NetworkThread::onStart(void)
{
	AbstractSphereThread::onStart();
	m_input.setOwner(this);
	m_output.setOwner(this);
#ifdef MTNETWORK_INPUT
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_DATA_RX);
#endif
#ifdef MTNETWORK_OUTPUT
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);
#endif
}

void NetworkThread::tick(void)
{
	// process periodic actions
	ADDTOCALLSTACK("NetworkThread::tick");
	checkNewStates();
	dropInvalidStates();

	if (m_states.empty())
	{
		// we haven't been assigned any clients, so go idle for now
		if (getPriority() != IThread::Disabled)
			setPriority(IThread::Low);
		return;
	}

#ifdef MTNETWORK_INPUT
	processInput();
#endif
#ifdef MTNETWORK_OUTPUT
	processOutput();
#endif

	// we're active, take priority
	setPriority(static_cast<IThread::Priority>(g_Cfg.m_iNetworkThreadPriority));
}

void NetworkThread::processInput(void)
{
	ADDTOCALLSTACK("NetworkThread::processInput");
#ifdef MTNETWORK_INPUT
	ASSERT(!isActive() || isCurrentThread());
#endif
	m_input.processInput();
}

void NetworkThread::processOutput(void)
{
	ADDTOCALLSTACK("NetworkThread::processOutput");
#ifdef MTNETWORK_OUTPUT
	ASSERT(!isActive() || isCurrentThread());
#endif
	m_output.processOutput();
}
	

/***************************************************************************
 *
 *
 *	class NetworkInput					Handles network input from clients
 *
 *
 ***************************************************************************/
NetworkInput::NetworkInput(void) : m_thread(NULL)
{
	m_receiveBuffer = new BYTE[NETWORK_BUFFERSIZE];
	m_decryptBuffer = new BYTE[NETWORK_BUFFERSIZE];
}

NetworkInput::~NetworkInput()
{
	if (m_receiveBuffer != NULL)
		delete[] m_receiveBuffer;
	if (m_decryptBuffer != NULL)
		delete[] m_decryptBuffer;
}

bool NetworkInput::processInput()
{
	ADDTOCALLSTACK("NetworkInput::processInput");
	ASSERT(m_thread != NULL);
#ifdef MTNETWORK_INPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	EXC_TRY("NetworkInput");

	// check for incoming data
	EXC_SET("select");
	fd_set fds;
	if (checkForData(fds) == false)
		return false;

	// check which states have data
	EXC_SET("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);

		const CClient* client = state->getClient();
		ASSERT(client != NULL);

		EXC_SET("check message");
		if ( ! FD_ISSET(state->m_socket.GetSocket(), &fds))
		{
			// check for timeout
			EXC_SET("check frozen");
			int iLastEventDiff = -g_World.GetTimeDiff( client->m_timeLastEvent );
			if ( g_Cfg.m_iDeadSocketTime > 0 && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
			{
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Frozen client connection disconnected.\n", state->id());
				state->markReadClosed();
			}

			EXC_SET("next state");
			continue;
		}
			
		// receive data
		EXC_SET("messages - receive");
		size_t received = state->m_socket.Receive(m_receiveBuffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			state->markReadClosed();
			EXC_SET("next state");
			continue;
		}

		EXC_SET("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);
		ProfileTask clientTask(PROFILE_CLIENTS);

		EXC_SET("messages - process");
		// todo: to allow MTNETWORK_INPUT to work, this loop needs to be in the main thread, if
		//       we could at least ensure that Packet::checkLength is thread-safe then we could
		//       reduce main thread work even further by storing entire packets in a buffer
		//       (e.g. ThreadSafeQueue<Packet> m_receivedPackets) rather than queuing batches of
		//       BYTE*s that the main thread has to piece together
		BYTE* buffer = m_receiveBuffer;
		while (received > 0)
		{
			// process next block of data
			size_t processed = processData(state, buffer, received);
			if (processed <= 0)
			{
				// nobody wants this data which is a bit odd and normally
				// means invalid data was received, so we'll close the
				// connection here
				state->markReadClosed();
				break;
			}

			// skip past processed data
			ASSERT(processed <= received);
			buffer += processed;
			received -= processed;
		}

		EXC_SET("next state");
	}

	return true;
	EXC_CATCH;
	return false;
}

bool NetworkInput::checkForData(fd_set& fds)
{
	// select() against each socket we own
	ADDTOCALLSTACK("NetworkInput::checkForData");

	EXC_TRY("CheckForData");

	int count = 0;
	FD_ZERO(&fds);

	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("check socket");
		if (state->isInUse() == false)
			continue;

		EXC_SET("cleaning queues");
		for (unsigned int i = 0; i < PacketSend::PRI_QTY; ++i)
			state->m_queue[i].clean();

		EXC_SET("check closing");
		if (state->isClosing() || state->m_socket.IsOpen() == false)
			continue;

		AddSocketToSet(fds, state->m_socket.GetSocket(), count);
	}
		
	EXC_SET("prepare timeout");
	timeval timeout; // time to wait for data.
	timeout.tv_sec = 0;
	timeout.tv_usec = 100; // micro seconds = 1/1000000

	EXC_SET("select");
	return select(count + 1, &fds, NULL, NULL, &timeout) > 0;

	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_DEBUG_END;
	return false;
}

size_t NetworkInput::processData(NetState* state, BYTE* buffer, size_t bufferSize)
{
	ADDTOCALLSTACK("NetworkInput::processData");

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	if (client->GetConnectType() == CONNECT_UNK)
		return processUnknownClientData(state, buffer, bufferSize);
		
	client->m_timeLastEvent = CServTime::GetCurrentTime();

	if ( client->m_Crypt.IsInit() == false )
		return processOtherClientData(state, buffer, bufferSize);

	return processGameClientData(state, buffer, bufferSize);
}

size_t NetworkInput::processGameClientData(NetState* state, BYTE* buffer, size_t bufferSize)
{
	ADDTOCALLSTACK("NetworkInput::processGameClientData");
	EXC_TRY("ProcessGameData");

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	EXC_SET("decrypt message");
	client->m_Crypt.Decrypt(m_decryptBuffer, buffer, bufferSize);

	if (state->m_receiveBuffer == NULL)
	{
		// create new buffer
		state->m_receiveBuffer = new Packet(m_decryptBuffer, bufferSize);
	}
	else
	{
		// append to buffer
		size_t pos = state->m_receiveBuffer->getPosition();
		state->m_receiveBuffer->seek(state->m_receiveBuffer->getLength());
		state->m_receiveBuffer->writeData(m_decryptBuffer, bufferSize);
		state->m_receiveBuffer->seek(pos);
	}

	Packet* packet = state->m_receiveBuffer;
	size_t remainingLength = packet->getLength() - packet->getPosition();

	EXC_SET("record message");
	xRecordPacket(client, packet, "client->server");

	// process the message
	EXC_TRYSUB("ProcessMessage");

	while (remainingLength > 0 && state->isClosing() == false)
	{
		BYTE packetId = packet->getData()[packet->getPosition()];
		Packet* handler = m_thread->m_manager.getPacketManager().getHandler(packetId);

		if (handler != NULL)
		{
			size_t packetLength = handler->checkLength(state, packet);
			if (packetLength <= 0)
			{
				DEBUGNETWORK(("%lx:Game packet (0x%x) does not match the expected length, waiting for more data...\n", state->id(), packetId));
				break;
			}

			remainingLength -= packetLength;

			// Packet filtering - check if any function trigger is installed
			//  allow skipping the packet which we do not wish to get
			if (client->xPacketFilter(packet->getData() + packet->getPosition(), packetLength))
			{
				packet->skip(packetLength);
				remainingLength -= packetLength;
				break;
			}

			// copy data to handler
			handler->seek();
			handler->writeData(packet->getData() + packet->getPosition(), packetLength);
			packet->skip(packetLength);

			// move to position 1 (no need for id) and fire onReceive()
			handler->resize(packetLength);
			handler->seek(1);
			handler->onReceive(state);
		}
		else
		{
			// Packet filtering - check if any function trigger is installed
			//  allow skipping the packet which we do not wish to get
			if (client->xPacketFilter(packet->getData() + packet->getPosition(), remainingLength))
			{
				// todo: adjust packet filter to specify size!
				// packet has been handled by filter but we don't know how big the packet
				// actually is.. we can only assume the entire buffer is used.
				packet->skip(remainingLength);
				remainingLength = 0;
				break;
			}

			// unknown packet.. we could skip 1 byte at a time but this can produce
			// strange behaviours (it's unlikely that only 1 byte is incorrect), so
			// it's best to just discard everything we have
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Unknown game packet (0x%x) received.\n", state->id(), packetId);
			packet->skip(remainingLength);
			remainingLength = 0;
		}
	}
		
	EXC_CATCHSUB("Message");
	EXC_DEBUGSUB_START;
	TemporaryString dump;
	packet->dump(dump);

	g_Log.EventDebug("%lx:Parsing %s", state->id(), (LPCTSTR)dump);

	state->m_packetExceptions++;
	if (state->m_packetExceptions > 10)
	{
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Disconnecting client from account '%s' since it is causing exceptions problems\n", state->id(), client != NULL && client->GetAccount() ? client->GetAccount()->GetName() : "");
		if (client != NULL)
			client->addKick(&g_Serv, false);
		else
			state->markReadClosed();
	}

	EXC_DEBUGSUB_END;

	// delete the buffer once it has been exhausted
	if (remainingLength <= 0)
	{
		state->m_receiveBuffer = NULL;
		delete packet;
	}

	return bufferSize;

	EXC_CATCH;
	return 0;
}

size_t NetworkInput::processOtherClientData(NetState* state, BYTE* buffer, size_t bufferSize)
{
		// process data from a non-game client
	ADDTOCALLSTACK("NetworkInput::processOtherClientData");
	EXC_TRY("ProcessOtherClientData");

	CClient* client = state->getClient();
	ASSERT(client != NULL);
		
	switch (client->GetConnectType())
	{
		case CONNECT_CRYPT:
			if (bufferSize < 5)
			{
				// not enough data to be a real client
				EXC_SET("ping #3");
				client->SetConnectType(CONNECT_UNK);
				if (client->OnRxPing(buffer, bufferSize) == false)
					return 0;
				break;
			}

			// first real data from client which we can use to log in
			EXC_SET("encryption setup");
			ASSERT(bufferSize <= sizeof(CEvent));

			CEvent evt;
			memcpy(&evt, buffer, bufferSize);

			if (buffer[0] == XCMD_EncryptionReply && state->isClientKR())
			{
				EXC_SET("encryption reply");

				// receiving response to 0xE3 packet
				size_t iEncKrLen = evt.EncryptionReply.m_len;
				if (bufferSize < iEncKrLen)
					return 0; // need more data
				else if (bufferSize == iEncKrLen)
					return bufferSize;

				client->xProcessClientSetup(reinterpret_cast<CEvent*>(evt.m_Raw + iEncKrLen), bufferSize - iEncKrLen);
				break;
			}

			client->xProcessClientSetup(&evt, bufferSize);
			break;

		case CONNECT_HTTP:
			EXC_SET("http message");
			if (client->OnRxWebPageRequest(buffer, bufferSize) == false)
				return 0;
			break;

		case CONNECT_TELNET:
			EXC_SET("telnet message");
			if (client->OnRxConsole(buffer, bufferSize) == false)
				return 0;
			break;

		default:
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Junk messages with no crypt\n", state->id());
			return 0;
	}

	return bufferSize;
	EXC_CATCH;
	return 0;
}

size_t NetworkInput::processUnknownClientData(NetState* state, BYTE* buffer, size_t bufferSize)
{
	// process data from an unknown client type
	ADDTOCALLSTACK("NetworkInput::processUnknownClientData");
	EXC_TRY("ProcessNewClient");

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	if (state->m_seeded == false)
	{
		// check for HTTP connection
		if ((bufferSize >= 5 && memcmp(buffer, "GET /", 5) == 0) ||
			(bufferSize >= 6 && memcmp(buffer, "POST /", 6) == 0))
		{
			EXC_SET("http request");
			if (g_Cfg.m_fUseHTTP != 2)
				return 0;

			client->SetConnectType(CONNECT_HTTP);
			if (client->OnRxWebPageRequest(buffer, bufferSize) == false)
				return 0;

			return bufferSize;
		}

		// check for new seed (sometimes it's received on its own)
		else if (bufferSize == 1 && buffer[0] == XCMD_NewSeed)
		{
			state->m_newseed = true;
			return 1;
		}

		// check for ping data
		else if (bufferSize < 4)
		{
			EXC_SET("ping #1");
			if (client->OnRxPing(buffer, bufferSize) == false)
				state->markReadClosed();

			return bufferSize;
		}

		// at least 4 bytes and not a web request, so must assume
		// it is a game client seed
		EXC_SET("game client seed");
		DWORD seed = 0;
		size_t seedLength = 0;
		if (state->m_newseed || (buffer[0] == XCMD_NewSeed && bufferSize >= NETWORK_SEEDLEN_NEW))
		{
			DEBUGNETWORK(("%lx:Receiving new client login handshake.\n", state->id()));

			CEvent* pEvent = reinterpret_cast<CEvent*>(buffer);
			if (state->m_newseed)
			{
				// we already received the 0xEF on its own, so move the pointer back
				// 1 byte to align it
				seedLength = NETWORK_SEEDLEN_NEW - 1;
				pEvent = reinterpret_cast<CEvent*>(buffer - 1);
			}
			else
			{
				seedLength = NETWORK_SEEDLEN_NEW;
				state->m_newseed = true;
			}

			if (bufferSize >= seedLength)
			{
				DEBUG_WARN(("%lx:New Login Handshake Detected. Client Version: %lu.%lu.%lu.%lu\n", state->id(),
							(DWORD)pEvent->NewSeed.m_Version_Maj, (DWORD)pEvent->NewSeed.m_Version_Min,
							(DWORD)pEvent->NewSeed.m_Version_Rev, (DWORD)pEvent->NewSeed.m_Version_Pat));
					
				state->m_reportedVersion = CCrypt::GetVerFromVersion(pEvent->NewSeed.m_Version_Maj, pEvent->NewSeed.m_Version_Min, pEvent->NewSeed.m_Version_Rev, pEvent->NewSeed.m_Version_Pat);
				seed = (DWORD) pEvent->NewSeed.m_Seed;
			}
			else
			{
				DEBUGNETWORK(("%lx:Not enough data received to be a valid handshake (%" FMTSIZE_T ").\n", state->id(), bufferSize));
			}
		}
		else
		{
			// assume it's a normal client login
			DEBUGNETWORK(("%lx:Receiving old client login handshake.\n", state->id()));

			seed = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
			seedLength = NETWORK_SEEDLEN_OLD;
		}

		DEBUGNETWORK(("%lx:Client connected with a seed of 0x%lx (new handshake=%d, seed length=%" FMTSIZE_T ", received=%" FMTSIZE_T ", version=0x%lx).\n",
			state->id(), seed, state->m_newseed? 1 : 0, seedLength, bufferSize, state->m_reportedVersion));

		if (seed == 0 || seedLength > bufferSize)
		{
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%lx:Invalid client detected, disconnecting.\n", state->id());
			return 0;
		}

		state->m_seeded = true;
		state->m_seed = seed;
			
		if (seedLength == bufferSize && state->m_seed == 0xFFFFFFFF)
		{
			// UO:KR Client opens connection with 255.255.255.255 and waits for the
			// server to send a packet before continuing
			EXC_SET("KR client seed");

			DEBUG_WARN(("UO:KR Client Detected.\n"));
			client->SetConnectType(CONNECT_CRYPT);
			state->m_clientType = CLIENTTYPE_KR;
			new PacketKREncryption(client);
		}

		return seedLength;
	}

	if (bufferSize < 5)
	{
		// client has a seed assigned but hasn't send enough data to be anything useful,
		// some programs (ConnectUO) send a fake seed when really they're just sending
		// ping data
		EXC_SET("ping #2");
		if (client->OnRxPing(buffer, bufferSize) == false)
			return 0;

		return bufferSize;
	}

	// process login packet for client
	EXC_SET("messages  - setup");
	client->SetConnectType(CONNECT_CRYPT);
	client->xProcessClientSetup(reinterpret_cast<CEvent*>(buffer), bufferSize);
	return bufferSize;

	EXC_CATCH;
	return 0;
}
	
	
/***************************************************************************
 *
 *
 *	class NetworkOutput					Handles network output to clients
 *
 *
 ***************************************************************************/
NetworkOutput::NetworkOutput() : m_thread(NULL)
{
	m_encryptBuffer = new BYTE[MAX_BUFFER];
}

NetworkOutput::~NetworkOutput()
{
	if (m_encryptBuffer != NULL)
		delete[] m_encryptBuffer;
}

bool NetworkOutput::processOutput()
{
	ADDTOCALLSTACK("NetworkOutput::processOutput");
	ASSERT(m_thread != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	ProfileTask networkTask(PROFILE_NETWORK_TX);
		
	static unsigned char tick = 0;
	EXC_TRY("NetworkOutput");
	tick++;

	// decide which queues need to be processed
	bool toProcess[PacketSend::PRI_QTY];
	if (m_thread->isActive())
	{
		toProcess[PacketSend::PRI_HIGHEST]  = true;
		toProcess[PacketSend::PRI_HIGH]     = ((tick %  2) ==  1);
		toProcess[PacketSend::PRI_NORMAL]   = ((tick %  4) ==  3);
		toProcess[PacketSend::PRI_LOW]      = ((tick %  8) ==  7);
		toProcess[PacketSend::PRI_IDLE]     = ((tick % 16) == 15);
	}
	else
	{
		// we receive less ticks in single-threaded mode, so process packet
		// queues a bit faster
		toProcess[PacketSend::PRI_HIGHEST]	= true;
		toProcess[PacketSend::PRI_HIGH]		= true;
		toProcess[PacketSend::PRI_NORMAL]	= true;
		toProcess[PacketSend::PRI_LOW]		= ((tick % 2) == 1);
		toProcess[PacketSend::PRI_IDLE]		= ((tick % 4) == 3);
	}

	// process clients which have been marked for flushing
	EXC_SET("flush");
	checkFlushRequests();

	size_t packetsSent = 0;
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		if (state->isWriteClosed())
			continue;

		// process packet queues
		for (int priority = PacketSend::PRI_HIGHEST; priority >= 0; --priority)
		{
			if (toProcess[priority] == false)
				continue;
			else if (state->isWriteClosed())
				break;
			packetsSent += processPacketQueue(state, priority);
		}

		// process asynchronous queue
		if (state->isWriteClosed() == false)
			packetsSent += processAsyncQueue(state);

		// process byte queue
		if (state->isWriteClosed() == false && processByteQueue(state))
			packetsSent++;
	}

	if (packetsSent > 0)
	{
		// notify thread there could be more to process
		if (m_thread->getPriority() == IThread::Disabled)
			m_thread->awaken();
	}

	return packetsSent > 0;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("ActiveThread=%d, TickCount=%d\n", m_thread->isActive()? 1 : 0, tick);
	EXC_DEBUG_END;
	return false;
}

void NetworkOutput::checkFlushRequests(void)
{
	// check for clients who need data flushing
	ADDTOCALLSTACK("NetworkOutput::checkFlushRequests");
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif
	
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		if (state->isWriteClosed())
			continue;

		if (state->needsFlush())
			flush(state);

		if (state->isClosing() && state->hasPendingData() == false)
			state->markWriteClosed();
	}
}

size_t NetworkOutput::flush(NetState* state)
{
	// process all queues for a client
	ADDTOCALLSTACK("NetworkOutput::processAllQueues");
	ASSERT(state != NULL);
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	size_t packetsSent = 0;
	for (int priority = PacketSend::PRI_HIGHEST; priority >= 0; --priority)
	{
		if (state->isWriteClosed())
			break;
		packetsSent += processPacketQueue(state, priority);
	}

	if (state->isWriteClosed() == false)
		packetsSent += processAsyncQueue(state);

	if (state->isWriteClosed() == false && processByteQueue(state))
		packetsSent++;

	state->markFlush(false);
	return packetsSent;
}

size_t NetworkOutput::processPacketQueue(NetState* state, unsigned int priority)
{
	// process a client's packet queue
	ADDTOCALLSTACK("NetworkOutput::processPacketQueue");
	ASSERT(state != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() ||
		(state->m_queue[priority].empty() && state->m_currentTransaction == NULL))
		return 0;

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	CServTime time = CServTime::GetCurrentTime();

	size_t maxPacketsToProcess = NETWORK_MAXPACKETS;
	size_t maxLengthToProcess = NETWORK_MAXPACKETLEN;
	size_t packetsProcessed = 0;
	size_t lengthProcessed = 0;

	while (packetsProcessed < maxPacketsToProcess && lengthProcessed < maxLengthToProcess)
	{
		// select next transaction
		while (state->m_currentTransaction == NULL)
		{
			if (state->m_queue[priority].empty())
				break;
				
			state->m_currentTransaction = state->m_queue[priority].front();
			state->m_queue[priority].pop();
		}

		PacketTransaction* transaction = state->m_currentTransaction;
		if (transaction == NULL)
			break;

		// acquire next packet from transaction
		PacketSend* packet = transaction->front();
		transaction->pop();

		// if the transaction is now empty we can clear it now so we can move
		// on to the next transaction later
		if (transaction->empty())
		{
			state->m_currentTransaction = NULL;
			delete transaction;
		}

		// check if the packet is allowed this packet
		if (state->canReceive(packet) == false || packet->onSend(client) == false)
		{
			delete packet;
			continue;
		}

		EXC_TRY("processPacketQueue");
		lengthProcessed += packet->getLength();
		packetsProcessed++;

		EXC_SET("sending");
		if (sendPacket(state, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
			break;
		}

		client->m_timeLastSend = time;

		EXC_CATCH;
		EXC_DEBUG_START;
		g_Log.EventDebug("id='%lx', pri='%u', packet '%" FMTSIZE_T "' of '%" FMTSIZE_T "' to send, length '%" FMTSIZE_T "' of '%" FMTSIZE_T "'\n",
			state->id(), priority, packetsProcessed, maxPacketsToProcess, lengthProcessed, maxLengthToProcess);
		EXC_DEBUG_END;
	}

	if (packetsProcessed >= maxPacketsToProcess)
	{
		DEBUGNETWORK(("Reached maximum packet count limit for this tick (%" FMTSIZE_T "/%" FMTSIZE_T ").\n", packetsProcessed, maxPacketsToProcess));
	}
	if (lengthProcessed >= maxLengthToProcess)
	{
		DEBUGNETWORK(("Reached maximum packet length limit for this tick (%" FMTSIZE_T "/%" FMTSIZE_T ").\n", lengthProcessed, maxLengthToProcess));
	}

	return packetsProcessed;
}

size_t NetworkOutput::processAsyncQueue(NetState* state)
{
	// process a client's async queue
	ADDTOCALLSTACK("NetworkOutput::processAsyncQueue");
	ASSERT(state != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	state->m_asyncQueue.clean();
	if (state->m_asyncQueue.empty() || state->isSendingAsync())
		return 0;

	const CClient* client = state->getClient();
	ASSERT(client != NULL);

	// select the next packet to send
	PacketSend* packet = NULL;
	while (state->m_asyncQueue.empty() == false)
	{
		packet = state->m_asyncQueue.front();
		state->m_asyncQueue.pop();

		// check if the client is allowed this
		if (state->canReceive(packet) && packet->onSend(client))
			break;
			
		// destroy the packet, we aren't going to use it
		if (packet != NULL)
		{
			delete packet;
			packet = NULL;
		}
	}

	if (packet == NULL)
		return 0;

	// start sending the packet we found
	if (sendPacketData(state, packet) == false)
	{
		state->clearQueues();
		state->markWriteClosed();
	}

	return 1;
}

bool NetworkOutput::processByteQueue(NetState* state)
{
	// process a client's byte queue
	ADDTOCALLSTACK("NetworkOutput::processByteQueue");
	ASSERT(state != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	if (state->isWriteClosed() || state->m_byteQueue.GetDataQty() <= 0)
		return false;

	size_t result = sendData(state, state->m_byteQueue.RemoveDataLock(), state->m_byteQueue.GetDataQty());
	if (result == _failed_result())
	{
		// error occurred
		state->clearQueues();
		state->markWriteClosed();
		return false;
	}
		
	if (result > 0)
		state->m_byteQueue.RemoveDataAmount(result);

	return true;
}

bool NetworkOutput::sendPacket(NetState* state, PacketSend* packet)
{
	// send packet to client (can be queued for async operation)
	ADDTOCALLSTACK("NetworkOutput::sendPacket");
	ASSERT(state != NULL);
	ASSERT(packet != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	// check the client is allowed the packet
	if (state->canReceive(packet) == false)
	{
		delete packet;
		return false;
	}

	if (state->isAsyncMode())
	{
		state->m_asyncQueue.push(packet);
		return true;
	}

	return sendPacketData(state, packet);
}

bool NetworkOutput::sendPacketData(NetState* state, PacketSend* packet)
{
	// send packet data to client
	ADDTOCALLSTACK("NetworkOutput::sendPacketData");
	ASSERT(state != NULL);
	ASSERT(packet != NULL);
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	CClient* client = state->getClient();
	ASSERT(client != NULL);

	EXC_TRY("sendPacketData");
	xRecordPacket(client, packet, "server->client");

	EXC_SET("send trigger");
	if (packet->onSend(client) == false)
	{
		delete packet;
		return true;
	}

	EXC_SET("prepare data");
	BYTE* sendBuffer = NULL;
	size_t sendBufferLength = 0;

	if (client->GetConnectType() == CONNECT_GAME)
	{
		// game clients require encryption
		EXC_SET("compress and encrypt");

		// compress
		size_t compressLength = client->xCompress(m_encryptBuffer, packet->getData(), packet->getLength());

		// encrypt
		if (client->m_Crypt.GetEncryptionType() == ENC_TFISH)
			client->m_Crypt.Encrypt(m_encryptBuffer, m_encryptBuffer, compressLength);

		sendBuffer = m_encryptBuffer;
		sendBufferLength = compressLength;
	}
	else
	{
		// other clients expect plain data
		sendBuffer = packet->getData();
		sendBufferLength = packet->getLength();
	}

	// queue packet data
	EXC_SET("queue data");
	state->m_byteQueue.AddNewData(sendBuffer, sendBufferLength);

	// if buffering is disabled then process the queue straight away
	// we need to do this rather than sending the packet data directly, otherwise if
	// the packet does not send all at once we will be stuck with an incomplete data
	// transfer (and no clean way to recover)
	if (g_Cfg.m_fUseExtraBuffer == false)
	{
		EXC_SET("send data");
		processByteQueue(state);
	}
		
	EXC_SET("sent trigger");
	packet->onSent(client);
	delete packet;
	return true;

	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%lx', packet '0x%x', length '%" FMTSIZE_T "'\n",
		state->id(), *packet->getData(), packet->getLength());
	EXC_DEBUG_END;
	return false;
}

size_t NetworkOutput::sendData(NetState* state, const BYTE* data, size_t length)
{
	// send raw data to client
	ADDTOCALLSTACK("NetworkOutput::sendData");
	ASSERT(state != NULL);
	ASSERT(data != NULL);
	ASSERT(length > 0);
	ASSERT(length != _failed_result());
#ifdef MTNETWORK_OUTPUT
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
#endif

	EXC_TRY("SendData");
	size_t result = 0;

	EXC_SET("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = length;
		state->m_bufferWSA.buf = (CHAR*)data;

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, SendCompleted) == 0)
		{
			result = bytesSent;
			state->setSendingAsync(true);
		}
		else
		{
			result = 0;
		}
	}
	else
#endif
	{
		// send via standard api
		int sent = state->m_socket.Send(data, length);
		if (sent > 0)
			result = static_cast<size_t>(sent);
		else
			result = 0;
	}

	// check for error
	if (result <= 0)
	{
		EXC_SET("error parse");
		int errorCode = CGSocket::GetLastError(true);

#ifdef _WIN32
		if (state->isAsyncMode() && errorCode == WSA_IO_PENDING)
		{
			// safe to ignore this, data has actually been sent (or will be)
			result = length;
		}
		else if (state->isAsyncMode() == false && errorCode == WSAEWOULDBLOCK)
#else
		if (errorCode == EAGAIN || errorCode == EWOULDBLOCK)
#endif
		{
			// send failed but it is safe to ignore and try again later
			result = 0;
		}
#ifdef _WIN32
		else if (errorCode == WSAECONNRESET || errorCode == WSAECONNABORTED)
#else
		else if (errorCode == ECONNRESET || errorCode == ECONNABORTED)
#endif
		{
			// connection has been lost, client should be cleared
			result = _failed_result();
		}
		else
		{
			EXC_SET("unexpected connection error");
			if (state->isClosing() == false)
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:TX Error %d\n", state->id(), errorCode);

#ifndef _WIN32
			result = _failed_result();
#else
			result = 0;
#endif
		}
	}

	if (result > 0 && result != _failed_result())
		CurrentProfileData.Count(PROFILE_DATA_TX, result);

	return result;
	EXC_CATCH;
	EXC_DEBUG_START;
	g_Log.EventDebug("id='%lx', packet '0x%x', length '%" FMTSIZE_T "'\n", state->id(), *data, length);
	EXC_DEBUG_END;
	return _failed_result();
}

void NetworkOutput::onAsyncSendComplete(NetState* state, bool success)
{
	// notify that async operation completed
	ADDTOCALLSTACK("NetworkOutput::onAsyncSendComplete");
	ASSERT(state != NULL);
	state->setSendingAsync(false);
	if (success == false)
		return;
		
#ifdef MTNETWORK_OUTPUT
	// we could process another batch of async data right now, but only if we
	// are in the output thread
	// - WinSock calls this from the same thread
	// - LinuxEv calls this from a completely different thread
	if (m_thread->isActive() && m_thread->isCurrentThread())
	{
		if (processAsyncQueue(state) > 0)
			processByteQueue(state);
	}
#endif
}

void NetworkOutput::QueuePacket(PacketSend* packet, bool appendTransaction)
{
	// queue a packet for sending
	ADDTOCALLSTACK("NetworkOutput::QueuePacket");
	ASSERT(packet != NULL);

	// don't bother queuing packets for invalid sockets
	NetState* state = packet->m_target;
	if (state == NULL || state->canReceive(packet) == false)
	{
		delete packet;
		return;
	}

	if (state->m_pendingTransaction != NULL && appendTransaction)
		state->m_pendingTransaction->push_back(packet);
	else
		QueuePacketTransaction(new SimplePacketTransaction(packet));
}

void NetworkOutput::QueuePacketTransaction(PacketTransaction* transaction)
{
	// queue a packet transaction for sending
	ADDTOCALLSTACK("NetworkOutput::QueuePacketTransaction");
	ASSERT(transaction != NULL);

	// don't bother queuing packets for invalid sockets
	NetState* state = transaction->getTarget();
	if (state == NULL || state->isInUse() == false || state->isWriteClosed())
	{
		delete transaction;
		return;
	}

	int priority = transaction->getPriority();
	ASSERT(priority >= PacketSend::PRI_IDLE && priority < PacketSend::PRI_QTY);

	// limit by max number of packets in queue
	size_t maxQueueSize = NETWORK_MAXQUEUESIZE;
	if (maxQueueSize > 0)
	{
		while (priority > PacketSend::PRI_IDLE && state->m_queue[priority].size() >= maxQueueSize)
		{
			priority--;
			transaction->setPriority(priority);
		}
	}

	state->m_queue[priority].push(transaction);

	// notify thread
	NetworkThread* thread = state->getParentThread();
	if (thread != NULL && thread->getPriority() == IThread::Disabled)
		thread->awaken();
}


/***************************************************************************
 *
 *
 *	class NetworkThreadStateIterator		Works as network state iterator getting the states
 *											for a thread, safely.
 *
 *
 ***************************************************************************/
NetworkThreadStateIterator::NetworkThreadStateIterator(const NetworkThread* thread)
{
	ASSERT(thread != NULL);
	m_thread = thread;
	m_nextIndex = 0;
	m_safeAccess = m_thread->isActive() && ! m_thread->isCurrentThread();
}

NetworkThreadStateIterator::~NetworkThreadStateIterator(void)
{
	m_thread = NULL;
}

NetState* NetworkThreadStateIterator::next(void)
{
	if (m_safeAccess == false)
	{
		// current thread, we can use the thread's state list directly
		// find next non-null state
		while (m_nextIndex < m_thread->m_states.size())
		{
			NetState* state = m_thread->m_states.at(m_nextIndex);
			++m_nextIndex;

			if (state != NULL)
				return state;
		}
	}
	else
	{
		// different thread, we have to use the manager's states
		while (m_nextIndex < m_thread->m_manager.m_stateCount)
		{
			NetState* state = m_thread->m_manager.m_states[m_nextIndex];
			++m_nextIndex;

			if (state != NULL && state->getParentThread() == m_thread)
				return state;
		}
	}

	return NULL;
}

#endif
