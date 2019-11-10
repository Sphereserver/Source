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

NetworkManager g_NetworkManager;

//
// Packet logging
//
#ifdef _DEBUG

void xRecordPacketData(const CClient *client, const BYTE *data, size_t length, LPCTSTR heading)
{
	if ( !(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS) )
		return;

	Packet packet(data, length);
	xRecordPacket(client, &packet, heading);
}

void xRecordPacket(const CClient *client, Packet *packet, LPCTSTR heading)
{
	if ( !(g_Cfg.m_wDebugFlags & DEBUGF_PACKETS) )
		return;

	TemporaryString dump;
	packet->dump(dump);

	// build file name
	TCHAR fname[64];
	strcpy(fname, "packets_");
	if (client->m_pAccount)
		strcat(fname, client->m_pAccount->GetName());
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
		out.Printf("%s %s\n\n", heading, static_cast<LPCTSTR>(dump));
		out.Close();
	}

	// write to console
	g_Log.EventDebug("%lx:%s %s\n", client->GetSocketID(), heading, static_cast<LPCTSTR>(dump));
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
	m_outgoing.currentTransaction = NULL;
	m_outgoing.pendingTransaction = NULL;
	m_incoming.buffer = NULL;
	m_incoming.rawBuffer = NULL;
	m_packetExceptions = 0;
	m_clientType = CLIENTTYPE_2D;
	m_clientVersion = 0;
	m_reportedVersion = 0;
	m_isInUse = false;
	m_parent = NULL;

	clear();
}

NetState::~NetState(void)
{
}

void NetState::clear(void)
{
	ADDTOCALLSTACK("NetState::clear");
	DEBUGNETWORK(("%lx:Clearing client state\n", id()));

	m_isReadClosed = true;
	m_isWriteClosed = true;
	m_needsFlush = false;

	CClient* client = m_client;
	if (client != NULL)
	{
		m_client = NULL;
		CAccount *account = client->m_pAccount;

		g_Serv.StatDec(SERV_STAT_CLIENTS);
		g_Log.Event(LOGM_NOCONTEXT|LOGM_CLIENTS_LOG, "%lx:Client disconnected [Total:%" FMTDWORD "] ('%s', acct='%s')\n", m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_peerAddress.GetAddrStr(), account ? account->GetName() : "<NA>");
		
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

	// empty queues
	clearQueues();

	// clean junk queue entries
	for (size_t i = 0; i < PacketSend::PRI_QTY; ++i)
		m_outgoing.queue[i].clean();
	m_outgoing.asyncQueue.clean();
	m_incoming.rawPackets.clean();

	if (m_outgoing.currentTransaction != NULL)
	{
		delete m_outgoing.currentTransaction;
		m_outgoing.currentTransaction = NULL;
	}

	if (m_outgoing.pendingTransaction != NULL)
	{
		delete m_outgoing.pendingTransaction;
		m_outgoing.pendingTransaction = NULL;
	}

	if (m_incoming.buffer != NULL)
	{
		delete m_incoming.buffer;
		m_incoming.buffer = NULL;
	}
	
	if (m_incoming.rawBuffer != NULL)
	{
		delete m_incoming.rawBuffer;
		m_incoming.rawBuffer = NULL;
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
	for (size_t i = 0; i < PacketSend::PRI_QTY; ++i)
	{
		while (m_outgoing.queue[i].empty() == false)
		{
			delete m_outgoing.queue[i].front();
			m_outgoing.queue[i].pop();
		}
	}

	// clear async queue
	while (m_outgoing.asyncQueue.empty() == false)
	{
		delete m_outgoing.asyncQueue.front();
		m_outgoing.asyncQueue.pop();
	}

	// clear byte queue
	m_outgoing.bytes.Empty();
	
	// clear received queue
	while (m_incoming.rawPackets.empty() == false)
	{
		delete m_incoming.rawPackets.front();
		m_incoming.rawPackets.pop();
	}
}

void NetState::init(SOCKET socket, CSocketAddress addr)
{
	ADDTOCALLSTACK("NetState::init");

	clear();

	DEBUGNETWORK(("%lx:Initializing client\n", id()));
	m_peerAddress = addr;
	m_socket.SetSocket(socket);
	if ( m_socket.SetNonBlocking() == SOCKET_ERROR )
		g_Log.Event(LOGL_CRIT, "Unable to set listen socket nonblocking mode\n");

	// disable NAGLE algorythm for data compression/coalescing.
	// Send as fast as we can. we handle packing ourselves.
	BOOL nbool = true;
	if ( m_socket.SetSockOpt(TCP_NODELAY, &nbool, sizeof(BOOL), IPPROTO_TCP) == SOCKET_ERROR )
		g_Log.Event(LOGL_CRIT, "Unable to set listen socket option TCP_NODELAY\n");

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
	ADDTOCALLSTACK("NetState::markReadClosed");

	DEBUGNETWORK(("%lx:Client being closed by read-thread\n", m_id));
	m_isReadClosed = true;
	if (m_parent != NULL && m_parent->getPriority() == IThread::Disabled)
		m_parent->awaken();
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
	//bool wasAsync = isAsyncMode();

	// Disabled because of unstability.
	setAsyncMode(false);
	return;
	// is async mode enabled?
/*	if ( !g_Cfg.m_fUseAsyncNetwork || !isInUse() )
		setAsyncMode(false);

	// if the version mod flag is not set, always use async mode
	else if ( g_Cfg.m_fUseAsyncNetwork != 2 )
		setAsyncMode(true);

	// http clients do not want to be using async networking unless they have keep-alive set
	else if (m_client && m_client->GetConnectType() == CONNECT_HTTP)
		setAsyncMode(false);

	// only use async with clients newer than 4.0.0
	// - normally the client version is unknown for the first 1 or 2 packets, so all clients will begin
	//   without async networking (but should switch over as soon as it has been determined)
	// - a minor issue with this is that for clients without encryption we cannot determine their version
	//   until after they have fully logged into the game server and sent a client version packet.
	else if (isClientVersion(MINCLIVER_AOS) || isClientKR() || isClientEnhanced())
		setAsyncMode(true);
	else
		setAsyncMode(false);

	if (wasAsync != isAsyncMode())
		DEBUGNETWORK(("%lx:Switching async mode from %s to %s\n", id(), wasAsync? "1":"0", isAsyncMode()? "1":"0"));*/
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
	for (int i = (isClosing() ? PacketSend::PRI_HIGHEST : PacketSend::PRI_IDLE); i < PacketSend::PRI_QTY; ++i)
	{
		if (m_outgoing.queue[i].empty() == false)
			return true;
	}

	// check async data
	if (isAsyncMode() && m_outgoing.asyncQueue.empty() == false)
		return true;

	// check byte queue
	if (m_outgoing.bytes.GetDataQty() > 0)
		return true;

	// check current transaction
	if (m_outgoing.currentTransaction != NULL)
		return true;

	return false;
}

bool NetState::canReceive(PacketSend* packet) const
{
	if (isInUse() == false || m_socket.IsOpen() == false || packet == NULL)
		return false;

	if (isClosing() && packet->getPriority() < PacketSend::PRI_HIGHEST)
		return false;

	if (packet->getTarget()->m_client == NULL)
		return false;

	return true;
}

void NetState::beginTransaction(long priority)
{
	ADDTOCALLSTACK("NetState::beginTransaction");
	if ( m_outgoing.pendingTransaction )
		DEBUGNETWORK(("%lx:New network transaction started whilst a previous is still open\n", id()));

	// ensure previous transaction is committed
	endTransaction();
	m_outgoing.pendingTransaction = new ExtendedPacketTransaction(this, g_Cfg.m_fUsePacketPriorities? priority : static_cast<long>(PacketSend::PRI_NORMAL));
}

void NetState::endTransaction(void)
{
	ADDTOCALLSTACK("NetState::endTransaction");
	if ( !m_outgoing.pendingTransaction )
		return;

	m_parent->queuePacketTransaction(m_outgoing.pendingTransaction);
	m_outgoing.pendingTransaction = NULL;
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

void HistoryIP::setBlocked(bool fBlock, INT64 iTimeout)
{
	// block ip
	ADDTOCALLSTACK("HistoryIP:setBlocked");
	if ( fBlock )
	{
		CScriptTriggerArgs Args(m_ip.GetAddrStr());
		Args.m_iN1 = iTimeout;
		g_Serv.r_Call("f_onserver_blockip", &g_Serv, &Args);
		iTimeout = Args.m_iN1;
	}

	m_blocked = fBlock;

	if ( fBlock && (iTimeout > 0) )
		m_blockExpire = CServTime::GetCurrentTime() + (iTimeout * TICK_PER_SEC);
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
			if (it->m_blockExpire.IsTimeValid() && (CServTime::GetCurrentTime() > it->m_blockExpire))
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
	memset(static_cast<void *>(&hist), 0, sizeof(hist));
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
		unregisterPacket(static_cast<unsigned int>(i));

	// delete extended packet handlers
	for (size_t i = 0; i < COUNTOF(m_extended); ++i)
		unregisterExtended(static_cast<unsigned int>(i));

	// delete encoded packet handlers
	for (size_t i = 0; i < COUNTOF(m_encoded); ++i)
		unregisterEncoded(static_cast<unsigned int>(i));
}

void PacketManager::registerStandardPackets(void)
{
	ADDTOCALLSTACK("PacketManager::registerStandardPackets");
	// Standard packets
	registerPacket(PACKET_Create, new PacketCreate());							// create character
	registerPacket(PACKET_WalkRequest, new PacketMovementReq());				// movement request
	registerPacket(PACKET_Talk, new PacketSpeakReq());							// speak
	registerPacket(PACKET_Attack, new PacketAttackReq());						// begin attack
	registerPacket(PACKET_DClick, new PacketDoubleClick());						// double click object
	registerPacket(PACKET_ItemPickupReq, new PacketItemPickupReq());			// pick up item
	registerPacket(PACKET_ItemDropReq, new PacketItemDropReq());				// drop item
	registerPacket(PACKET_Click, new PacketSingleClick());						// single click object
	registerPacket(PACKET_ExtCmd, new PacketTextCommand());						// extended text command
	registerPacket(PACKET_ItemEquipReq, new PacketItemEquipReq());				// equip item
	registerPacket(PACKET_WalkAck, new PacketResynchronize());					//
	registerPacket(PACKET_DeathMenu, new PacketDeathStatus());					//
	registerPacket(PACKET_CharStatReq, new PacketCharStatusReq());				// status request
	registerPacket(PACKET_Skill, new PacketSkillLockChange());					// change skill lock
	registerPacket(PACKET_VendorBuy, new PacketVendorBuyReq());					// buy items from vendor
	registerPacket(PACKET_StaticUpdate, new PacketStaticUpdate());				// UltimaLive Packet
	registerPacket(PACKET_MapEdit, new PacketMapEdit());						// edit map pins
	registerPacket(PACKET_CharPlay, new PacketCharPlay());						// select character
	registerPacket(PACKET_BookPage, new PacketBookPageEdit());					// edit book content
	registerPacket(PACKET_Options, new PacketUnknown());						// unused options packet
	registerPacket(PACKET_Target, new PacketTarget());							// target an object
	registerPacket(PACKET_SecureTrade, new PacketSecureTradeReq());				// secure trade action
	registerPacket(PACKET_BBoard, new PacketBulletinBoardReq());				// bulletin board action
	registerPacket(PACKET_War, new PacketWarModeReq());							// toggle war mode
	registerPacket(PACKET_Ping, new PacketPingReq());							// ping request
	registerPacket(PACKET_CharName, new PacketCharRename());					// change character name
	registerPacket(PACKET_MenuChoice, new PacketMenuChoice());					// select menu item
	registerPacket(PACKET_ServersReq, new PacketServersReq());					// request server list
	registerPacket(PACKET_CharDelete, new PacketCharDelete());					// delete character
	registerPacket(PACKET_CreateNew, new PacketCreateNew());					// create character (KR/SA)
	registerPacket(PACKET_CharListReq, new PacketCharListReq());				// request character list
	registerPacket(PACKET_BookOpen, new PacketBookHeaderEdit());				// edit book
	registerPacket(PACKET_DyeVat, new PacketDyeObject());						// colour selection dialog
	registerPacket(PACKET_AllNames3D, new PacketAllNamesReq());					// all names macro (ctrl+shift)
	registerPacket(PACKET_Prompt, new PacketPromptResponse());					// prompt response
	registerPacket(PACKET_HelpPage, new PacketHelpPageReq());					// GM help page request
	registerPacket(PACKET_VendorSell, new PacketVendorSellReq());				// sell items to vendor
	registerPacket(PACKET_ServerSelect, new PacketServerSelect());				// select server
	registerPacket(PACKET_Spy, new PacketSystemInfo());							//
	registerPacket(PACKET_Scroll, new PacketUnknown(5));						// scroll closed
	registerPacket(PACKET_TipReq, new PacketTipReq());							// request tip of the day
	registerPacket(PACKET_GumpInpValRet, new PacketGumpValueInputResponse());	// text input dialog
	registerPacket(PACKET_TalkUNICODE, new PacketSpeakReqUNICODE());			// speech (unicode)
	registerPacket(PACKET_GumpDialogRet, new PacketGumpDialogRet());			// dialog response (button press)
	registerPacket(PACKET_ChatText, new PacketChatCommand());					// chat command
	registerPacket(PACKET_Chat, new PacketChatButton());						// chat button
	registerPacket(PACKET_ToolTipReq, new PacketToolTipReq());					// popup help request
	registerPacket(PACKET_CharProfile, new PacketProfileReq());					// profile read/write request
	registerPacket(PACKET_MailMsg, new PacketMailMessage());					//
	registerPacket(PACKET_ClientVersion, new PacketClientVersion());			// client version
	registerPacket(PACKET_ExtendedData, new PacketExtendedCommand());			//
	registerPacket(PACKET_PromptUNICODE, new PacketPromptResponseUnicode());	// prompt response (unicode)
	registerPacket(PACKET_ViewRange, new PacketViewRange());					//
	registerPacket(PACKET_ConfigFile, new PacketUnknown());						//
	registerPacket(PACKET_LogoutStatus, new PacketLogout());					//
	registerPacket(PACKET_AOSBookPage, new PacketBookHeaderEditNew());			// edit book
	registerPacket(PACKET_AOSTooltip, new PacketAOSTooltipReq());				// request tooltip data
	registerPacket(PACKET_EncodedData, new PacketEncodedCommand());				//
	registerPacket(PACKET_Spy2, new PacketHardwareInfo());						// client hardware info
	registerPacket(PACKET_BugReport, new PacketBugReport());					// bug report
	registerPacket(PACKET_KRClientType, new PacketClientType());				// report client type (2d/kr/sa)
	registerPacket(PACKET_HighlightUIRemove, new PacketRemoveUIHighlight());	//
	registerPacket(PACKET_UseHotbar, new PacketUseHotbar());					//
	registerPacket(PACKET_MacroEquipItem, new PacketEquipItemMacro());			// equip item(s) macro (KR)
	registerPacket(PACKET_MacroUnEquipItem, new PacketUnEquipItemMacro());		// unequip item(s) macro (KR)
	registerPacket(PACKET_WalkRequestNew, new PacketMovementReqNew());			// new movement request (KR/SA)
	registerPacket(PACKET_TimeSyncRequest, new PacketTimeSyncRequest());		// time sync request (KR/SA)
	registerPacket(PACKET_CrashReport, new PacketCrashReport());				//
	registerPacket(PACKET_CreateHS, new PacketCreateHS());						// create character (HS)
	registerPacket(PACKET_UltimaStoreButton, new PacketUltimaStoreButton());	// ultima store button
	registerPacket(PACKET_GlobalChat, new PacketGlobalChatReq());				//
	registerPacket(PACKET_MultiSight, new PacketMultiSight());					// toggle OSI multi sight

	// Extended packets (0xBF)
	registerExtended(PACKETEXT_ScreenSize, new PacketScreenSize());				// client screen size
	registerExtended(PACKETEXT_Party_Msg, new PacketPartyMessage());			// party command
	registerExtended(PACKETEXT_QuestArrow_Click, new PacketArrowClick());		// click quest arrow
	registerExtended(PACKETEXT_Wrestle_Disarm, new PacketWrestleDisarm());		// wrestling disarm macro (obsolete, removed since AOS)
	registerExtended(PACKETEXT_Wrestle_Stun, new PacketWrestleStun());			// wrestling stun macro (obsolete, removed since AOS)
	registerExtended(PACKETEXT_Language, new PacketLanguage());					// client language
	registerExtended(PACKETEXT_StatusClose, new PacketStatusClose());			// status window closed
	registerExtended(PACKETEXT_AnimationReq, new PacketAnimationReq());			// play animation
	registerExtended(PACKETEXT_ClientInfo, new PacketClientInfo());				// client information
	registerExtended(PACKETEXT_OldAOSTooltipInfo, new PacketAosTooltipInfo());	//
	registerExtended(PACKETEXT_Popup_Request, new PacketPopupReq());			// request popup menu
	registerExtended(PACKETEXT_Popup_Select, new PacketPopupSelect());			// select popup option
	registerExtended(PACKETEXT_StatLock, new PacketChangeStatLock());			// change stat lock
	registerExtended(PACKETEXT_CastSpell, new PacketSpellSelect());				//
	registerExtended(PACKETEXT_HouseDesignReq, new PacketHouseDesignReq());		// house design request
	registerExtended(PACKETEXT_AntiCheat, new PacketAntiCheat());				// anti-cheat / unknown (sent by SE clients, every second or so)
	registerExtended(PACKETEXT_BandageMacro, new PacketBandageMacro());			//
	registerExtended(PACKETEXT_TargetedSpell, new PacketTargetedSpell());		// use targeted skill
	registerExtended(PACKETEXT_TargetedSkill, new PacketTargetedSkill());		// use targeted skill
	registerExtended(PACKETEXT_TargetedResource, new PacketTargetedResource());	//
	registerExtended(PACKETEXT_GargoyleFly, new PacketGargoyleFly());			// gargoyle flying action
	registerExtended(PACKETEXT_WheelBoatMove, new PacketWheelBoatMove());		// wheel boat movement

	// Encoded packets (0xD7)
	registerEncoded(PACKETENC_HouseDesign_Backup, new PacketHouseDesignBackup());		// house design - backup
	registerEncoded(PACKETENC_HouseDesign_Restore, new PacketHouseDesignRestore());		// house design - restore
	registerEncoded(PACKETENC_HouseDesign_Commit, new PacketHouseDesignCommit());		// house design - commit
	registerEncoded(PACKETENC_HouseDesign_RemItem, new PacketHouseDesignDestroyItem());	// house design - remove item
	registerEncoded(PACKETENC_HouseDesign_AddItem, new PacketHouseDesignPlaceItem());	// house design - place item
	registerEncoded(PACKETENC_HouseDesign_Exit, new PacketHouseDesignExit());			// house design - exit designer
	registerEncoded(PACKETENC_HouseDesign_AddStair, new PacketHouseDesignPlaceStair());	// house design - place stairs
	registerEncoded(PACKETENC_HouseDesign_Sync, new PacketHouseDesignSync());			// house design - synchronise
	registerEncoded(PACKETENC_HouseDesign_Clear, new PacketHouseDesignClear());			// house design - clear
	registerEncoded(PACKETENC_HouseDesign_Switch, new PacketHouseDesignSwitch());		// house design - change floor
	registerEncoded(PACKETENC_HouseDesign_AddRoof, new PacketHouseDesignPlaceRoof());	// house design - place roof
	registerEncoded(PACKETENC_HouseDesign_RemRoof, new PacketHouseDesignDestroyRoof());	// house design - remove roof
	registerEncoded(PACKETENC_SpecialMoveSelect, new PacketSpecialMoveSelect());		//
	registerEncoded(PACKETENC_HouseDesign_Revert, new PacketHouseDesignRevert());		// house design - revert
	registerEncoded(PACKETENC_EquipLastWeapon, new PacketEquipLastWeapon());			//
	registerEncoded(PACKETENC_GuildButton, new PacketGuildButton());					// guild button press
	registerEncoded(PACKETENC_QuestButton, new PacketQuestButton());					// quest button press
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

ClientIterator::ClientIterator(const NetworkManager* network)
{
	m_network = (network == NULL? &g_NetworkManager : network);
	m_nextClient = static_cast<CClient*> (m_network->m_clients.GetHead());
}

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
		if (!current->m_NetState || !current->m_NetState->isInUse(current) || current->m_NetState->isClosed())
			continue;

		// skip clients whose connection is being closed
		if (!includeClosing && current->m_NetState->isClosing())
			continue;

		m_nextClient = current->GetNext();
		return current;
	}

	return NULL;
}

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
		DEBUGNETWORK(("Async i/o operation completed without client context\n"));
		return;
	}

	NetworkThread* thread = state->getParentThread();
	if (thread == NULL)
	{
		DEBUGNETWORK(("%lx:Async i/o operation completed\n", state->id()));
		return;
	}

	if (dwError != 0)
		DEBUGNETWORK(("%lx:Async i/o operation completed with error code 0x%x, %ld bytes sent\n", state->id(), dwError, cbTransferred));

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
		g_Log.Event(LOGL_WARN, "Too many network threads requested. Reducing to %" FMTSIZE_T "\n", maxThreads);
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
	DEBUGNETWORK(("Searching for a suitable thread to handle a new client...\n"));

	// search for quietest thread
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
	{
		if ((*it)->getClientCount() < bestThreadSize)
		{
			bestThread = *it;
			bestThreadSize = bestThread->getClientCount();
		}
	}

	if ( bestThread )
		DEBUGNETWORK(("Selected thread #%" FMTSIZE_T "\n", bestThread->id()));
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

	DEBUGNETWORK(("Receiving new connection\n"));

	// accept socket connection
	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (h == INVALID_SOCKET)
		return;

	// check ip history
	EXC_SET("ip history");

	DEBUGNETWORK(("Retrieving IP history for '%s'\n", client_addr.GetAddrStr()));
	HistoryIP& ip = m_ips.getHistoryForIP(client_addr);
	long maxIp = g_Cfg.m_iConnectingMaxIP;
	long climaxIp = g_Cfg.m_iClientsMaxIP;

	DEBUGNETWORK(("Incoming connection from '%s' [blocked=%d, ttl=%ld, pings=%ld, connecting=%ld, connected=%ld]\n", ip.m_ip.GetAddrStr(), ip.m_blocked, ip.m_ttl, ip.m_pings, ip.m_connecting, ip.m_connected));

	// check if ip is allowed to connect
	if ( ip.checkPing() ||									// check for ip ban
			( maxIp > 0 && ip.m_connecting > maxIp ) ||		// check for too many connecting
			( climaxIp > 0 && ip.m_connected > climaxIp ) )	// check for too many connected
	{
		EXC_SET("rejected");
		DEBUGNETWORK(("Closing incoming connection [max ip=%ld, clients max ip=%ld]\n", maxIp, climaxIp));
		CLOSESOCKET(h);

		if ( g_Log.GetLogMask() & LOGM_CLIENTS_LOG )
		{
			if ( ip.m_blocked )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "Connection from %s rejected (Blocked IP)\n", client_addr.GetAddrStr());
			else if ( maxIp && (ip.m_connecting > maxIp) )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "Connection from %s rejected (CONNECTINGMAXIP reached %ld/%ld)\n", client_addr.GetAddrStr(), ip.m_connecting, maxIp);
			else if ( climaxIp && (ip.m_connected > climaxIp) )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "Connection from %s rejected (CLIENTMAXIP reached %ld/%ld)\n", client_addr.GetAddrStr(), ip.m_connected, climaxIp);
			else if ( ip.m_pings >= NETHISTORY_MAXPINGS )
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "Connection from %s rejected (MAXPINGS reached %ld/%d)\n", client_addr.GetAddrStr(), ip.m_pings, NETHISTORY_MAXPINGS);
			else
				g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "Connection from %s rejected\n", client_addr.GetAddrStr());
		}
		return;
	}

	// select an empty slot
	EXC_SET("detecting slot");
	NetState* state = findFreeSlot();
	if (state == NULL)
	{
		// not enough empty slots
		EXC_SET("no slot available");
		DEBUGNETWORK(("Unable to allocate new slot for client, too many clients already connected\n"));
		CLOSESOCKET(h);

		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_ERROR, "Connection from %s rejected (CLIENTMAX reached)\n", client_addr.GetAddrStr());
		return;
	}

	DEBUGNETWORK(("%lx:Allocated slot for client (%lu)\n", state->id(), static_cast<unsigned long>(h)));

	// assign slot
	EXC_SET("assigning slot");
	state->init(h, client_addr);

	DEBUGNETWORK(("%lx:State initialized, registering client instance\n", state->id()));

	EXC_SET("recording client");
	if (state->m_client)
		m_clients.InsertHead(state->m_client);

	EXC_SET("assigning thread");
	DEBUGNETWORK(("%lx:Selecting a thread to assign to\n", state->id()));
	assignNetworkState(state);

	DEBUGNETWORK(("%lx:Client successfully initialized\n", state->id()));

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
	for (unsigned int i = 0; i < g_Cfg.m_iClientsMax; ++i)
		m_states[i] = new NetState(static_cast<long>(i));
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

		DEBUGNETWORK(("Started %" FMTSIZE_T " network threads\n", m_threads.size()));
	}
	else
	{
		// Initialize network threads
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
			state->m_outgoing.queue[priority].clean();
		state->m_outgoing.asyncQueue.clean();

		EXC_SET("check closing");
		if (state->isClosing() == false)
		{
			// if data is queued whilst the thread is in the middle of processing then the signal
			// sent from NetworkOutput::QueuePacketTransaction can be ignored
			// the safest solution to this is to send additional signals from here
			NetworkThread* thread = state->getParentThread();
			if (thread != NULL && state->hasPendingData() && thread->getPriority() == IThread::Disabled)
				thread->awaken();
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
					DEBUGNETWORK(("%lx:Flushing data for client\n", state->id()));
					NetworkThread * thread = state->getParentThread();
					if (thread != NULL)
						thread->flush(state);
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
			DEBUGNETWORK(("%lx:Client is being cleared since marked to close\n", state->id()));
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

	// force each thread to process input (NOT THREADSAFE)
	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->processInput();
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

	for (NetworkThreadList::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
		(*it)->flushAllClients();
}

size_t NetworkManager::flush(NetState * state)
{
	// flush data for a single client
	ADDTOCALLSTACK("NetworkManager::flush");
	ASSERT(state != NULL);
	NetworkThread * thread = state->getParentThread();
	if (thread != NULL)
		return thread->flush(state);

	return 0;
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
	m_profile.EnableProfile(PROFILE_NETWORK_RX);
	m_profile.EnableProfile(PROFILE_DATA_RX);
	m_profile.EnableProfile(PROFILE_NETWORK_TX);
	m_profile.EnableProfile(PROFILE_DATA_TX);
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

	processInput();
	processOutput();

	// we're active, take priority
	setPriority(static_cast<IThread::Priority>(g_Cfg.m_iNetworkThreadPriority));
}

void NetworkThread::flushAllClients(void)
{
	ADDTOCALLSTACK("NetworkThread::flushAllClients");
	NetworkThreadStateIterator states(this);
	while (NetState* state = states.next())
		m_output.flush(state);
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

	// when called from within the thread's context, we just receive data
	if (!m_thread->isActive() || m_thread->isCurrentThread())
		receiveData();

	// when called from outside the thread's context, we process data
	if (!m_thread->isActive() || !m_thread->isCurrentThread())
	{
		// if the thread does not receive ticks, we must perform a quick select to see if we should
		// wake up the thread
		if (m_thread->isActive() && m_thread->getPriority() == IThread::Disabled)
		{
			fd_set fds;
			if (checkForData(fds))
				m_thread->awaken();
		}

		processData();
	}
	return true;
}

void NetworkInput::receiveData()
{
	ADDTOCALLSTACK("NetworkInput::receiveData");
	ASSERT(m_thread != NULL);
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
	EXC_TRY("ReceiveData");

	// check for incoming data
	EXC_SET("select");
	fd_set fds;
	if (checkForData(fds) == false)
		return;

	EXC_SET("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);
		if ( ! FD_ISSET(state->m_socket.GetSocket(), &fds))
		{
			state->m_incoming.rawPackets.clean();
			continue;
		}
			
		// receive data
		EXC_SET("messages - receive");
		int received = state->m_socket.Receive(m_receiveBuffer, NETWORK_BUFFERSIZE, 0);
		if (received <= 0 || received > NETWORK_BUFFERSIZE)
		{
			state->markReadClosed();
			EXC_SET("next state");
			continue;
		}

		EXC_SET("start client profile");
		CurrentProfileData.Count(PROFILE_DATA_RX, received);

		EXC_SET("messages - parse");

		// our objective here is to take the received data and separate it into packets to
		// be stored in NetState::m_incoming.rawPackets
		BYTE* buffer = m_receiveBuffer;
		while (received > 0)
		{
			// currently we just take the data and push it into a queue for the main thread
			// to parse into actual packets
			// todo: if possible, it would be useful to be able to perform that separation here,
			// but this is made difficult due to the variety of client types and encryptions that
			// may be connecting
			size_t length = received;

			Packet* packet = new Packet(buffer, length);
			state->m_incoming.rawPackets.push(packet);
			buffer += length;
			received -= static_cast<int>(length);
		}
	}

	EXC_CATCH;
}

void NetworkInput::processData()
{
	ADDTOCALLSTACK("NetworkInput::processData");
	ASSERT(m_thread != NULL);
	ASSERT(!m_thread->isActive() || !m_thread->isCurrentThread());
	EXC_TRY("ProcessData");

	// check which states have data
	EXC_SET("messages");
	NetworkThreadStateIterator states(m_thread);
	while (NetState* state = states.next())
	{
		EXC_SET("check socket");
		if (state->isReadClosed())
			continue;

		EXC_SET("start network profile");
		ProfileTask networkTask(PROFILE_NETWORK_RX);

		const CClient* client = state->m_client;
		ASSERT(client != NULL);

		EXC_SET("check message");
		if (state->m_incoming.rawPackets.empty())
		{
			if ((client->GetConnectType() != CONNECT_TELNET) && (client->GetConnectType() != CONNECT_AXIS))
			{
				// check for timeout
				EXC_SET("check frozen");
				INT64 iLastEventDiff = -g_World.GetTimeDiff( client->m_timeLastEvent );
				if ( g_Cfg.m_iDeadSocketTime > 0 && iLastEventDiff > g_Cfg.m_iDeadSocketTime )
				{
					g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Frozen client disconnected\n", state->id());
					state->m_client->addLoginErr( PacketLoginError::Other );		//state->markReadClosed();
				}
			}

			if (state->m_incoming.rawBuffer == NULL)
			{
				EXC_SET("next state");
				continue;
			}
		}

		EXC_SET("messages - process");
		// we've already received some raw data, we just need to add it to any existing data we have
		while (state->m_incoming.rawPackets.empty() == false)
		{
			Packet* packet = state->m_incoming.rawPackets.front();
			state->m_incoming.rawPackets.pop();
			ASSERT(packet != NULL);

			EXC_SET("packet - queue data");
			if (state->m_incoming.rawBuffer == NULL)
			{
				// create new buffer
				state->m_incoming.rawBuffer = new Packet(packet->getData(), packet->getLength());
			}
			else
			{
				// append to buffer
				size_t pos = state->m_incoming.rawBuffer->getPosition();
				state->m_incoming.rawBuffer->seek(state->m_incoming.rawBuffer->getLength());
				state->m_incoming.rawBuffer->writeData(packet->getData(), packet->getLength());
				state->m_incoming.rawBuffer->seek(pos);
			}

			delete packet;
		}

		if (g_Serv.IsLoading() == false)
		{
			EXC_SET("start client profile");
			ProfileTask clientTask(PROFILE_CLIENTS);

			EXC_SET("packets - process");
			Packet* buffer = state->m_incoming.rawBuffer;
			if (buffer != NULL)
			{
				// we have a buffer of raw bytes, we need to go through them all and process as much as we can
				while (state->isReadClosed() == false && buffer->getRemainingLength() > 0)
				{
					if (processData(state, buffer))
						continue;

					// processData didn't want to use any data, which means we probably
					// received some invalid data or that the packet was malformed
					// best course of action right now is to close the connection
					state->markReadClosed();
					break;
				}

				if (buffer->getRemainingLength() <= 0)
				{
					EXC_SET("packets - clear buffer");
					delete buffer;
					state->m_incoming.rawBuffer = NULL;
				}
			}
		}
		EXC_SET("next state");
	}

	EXC_CATCH;
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
		if (state->isReadClosed())
			continue;

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
	return false;
}

bool NetworkInput::processData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processData");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->m_client;
	ASSERT(client != NULL);

	if (client->GetConnectType() == CONNECT_UNK)
		return processUnknownClientData(state, buffer);
		
	client->m_timeLastEvent = CServTime::GetCurrentTime();

	if ( client->m_Crypt.IsInit() == false )
		return processOtherClientData(state, buffer);

	return processGameClientData(state, buffer);
}

bool NetworkInput::processGameClientData(NetState* state, Packet* buffer)
{
	ADDTOCALLSTACK("NetworkInput::processGameClientData");
	EXC_TRY("ProcessGameData");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->m_client;
	ASSERT(client != NULL);

	EXC_SET("decrypt message");
	client->m_Crypt.Decrypt(m_decryptBuffer, buffer->getRemainingData(), buffer->getRemainingLength());

	if (state->m_incoming.buffer == NULL)
	{
		// create new buffer
		state->m_incoming.buffer = new Packet(m_decryptBuffer, buffer->getRemainingLength());
	}
	else
	{
		// append to buffer
		size_t pos = state->m_incoming.buffer->getPosition();
		state->m_incoming.buffer->seek(state->m_incoming.buffer->getLength());
		state->m_incoming.buffer->writeData(m_decryptBuffer, buffer->getRemainingLength());
		state->m_incoming.buffer->seek(pos);
	}

	Packet* packet = state->m_incoming.buffer;
	size_t remainingLength = packet->getRemainingLength();

	EXC_SET("record message");
#ifdef _DEBUG
	xRecordPacket(client, packet, "client->server");
#endif

	// process the message
	EXC_TRYSUB("ProcessMessage");

	while (remainingLength > 0 && state->isClosing() == false)
	{
		ASSERT(remainingLength == packet->getRemainingLength());
		BYTE packetId = packet->getRemainingData()[0];
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
			if (client->xPacketFilter(packet->getRemainingData(), packetLength))
			{
				packet->skip(static_cast<long>(packetLength));
				continue;
			}

			// copy data to handler
			handler->seek();
			handler->writeData(packet->getRemainingData(), packetLength);
			packet->skip(static_cast<long>(packetLength));

			// move to position 1 (no need for id) and fire onReceive()
			handler->resize(packetLength);
			handler->seek(1);
			handler->onReceive(state);
		}
		else
		{
			// Packet filtering - check if any function trigger is installed
			//  allow skipping the packet which we do not wish to get
			if (client->xPacketFilter(packet->getRemainingData(), remainingLength))
			{
				// todo: adjust packet filter to specify size!
				// packet has been handled by filter but we don't know how big the packet
				// actually is.. we can only assume the entire buffer is used.
				packet->skip(static_cast<long>(remainingLength));
				remainingLength = 0;
				break;
			}

			// unknown packet.. we could skip 1 byte at a time but this can produce
			// strange behaviours (it's unlikely that only 1 byte is incorrect), so
			// it's best to just discard everything we have
			g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Unknown game packet (0x%x) received\n", state->id(), packetId);
			packet->skip(static_cast<long>(remainingLength));
			remainingLength = 0;
		}
	}
		
	EXC_CATCHSUB("Message");
	EXC_DEBUGSUB_START;
	TemporaryString dump;
	packet->dump(dump);

	g_Log.EventDebug("%lx:Parsing %s", state->id(), static_cast<LPCTSTR>(dump));

	++state->m_packetExceptions;
	if (state->m_packetExceptions > 10)
	{
		g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN, "%lx:Disconnecting client from account '%s' since it is causing exceptions problems\n", state->id(), client && client->m_pAccount ? client->m_pAccount->GetName() : "");
		if (client != NULL)
			client->addKick(&g_Serv, false);
		else
			state->markReadClosed();
	}

	EXC_DEBUGSUB_END;

	// delete the buffer once it has been exhausted
	if (remainingLength <= 0)
	{
		state->m_incoming.buffer = NULL;
		delete packet;
	}

	buffer->seek(buffer->getLength());
	return true;

	EXC_CATCH;
	return false;
}

bool NetworkInput::processOtherClientData(NetState* state, Packet* buffer)
{
	// process data from a non-game client
	ADDTOCALLSTACK("NetworkInput::processOtherClientData");
	EXC_TRY("ProcessOtherClientData");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->m_client;
	ASSERT(client != NULL);

	switch (client->GetConnectType())
	{
		case CONNECT_CRYPT:
			if (buffer->getRemainingLength() < 5)
			{
				// not enough data to be a real client
				EXC_SET("ping #3");
				client->SetConnectType(CONNECT_UNK);
				if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
					return false;
				break;
			}

			// first real data from client which we can use to log in
			EXC_SET("encryption setup");
			ASSERT(buffer->getRemainingLength() <= sizeof(CEvent));

			CEvent evt;
			memcpy(&evt, buffer->getRemainingData(), buffer->getRemainingLength());

			if ((evt.Default.m_Cmd == PACKET_EncryptionReply) && state->isClientKR())
			{
				EXC_SET("encryption reply");

				// receiving response to 0xE3 packet
				size_t iEncKrLen = evt.EncryptionReply.m_len;
				if (buffer->getRemainingLength() < iEncKrLen)
					return false; // need more data

				buffer->skip(static_cast<long>(iEncKrLen));
				return true;
			}

			client->xProcessClientSetup(&evt, buffer->getRemainingLength());
			break;

		case CONNECT_HTTP:
			EXC_SET("http message");
			if (client->OnRxWebPageRequest(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;
			break;

		case CONNECT_TELNET:
			EXC_SET("telnet message");
			if (client->OnRxConsole(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;
			break;
		case CONNECT_AXIS:
			EXC_SET("Axis message");
			if (client->OnRxAxis(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;
			break;

		default:
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Junk messages with no crypt\n", state->id());
			return false;
	}

	buffer->seek(buffer->getLength());
	return true;
	EXC_CATCH;
	return false;
}

bool NetworkInput::processUnknownClientData(NetState* state, Packet* buffer)
{
	// process data from an unknown client type
	ADDTOCALLSTACK("NetworkInput::processUnknownClientData");
	EXC_TRY("ProcessNewClient");
	ASSERT(state != NULL);
	ASSERT(buffer != NULL);
	CClient* client = state->m_client;
	ASSERT(client != NULL);

	if ( buffer->getRemainingLength() > 1024 )
	{
		DEBUGNETWORK(("%lx:Client connected with a seed length of %" FMTSIZE_T " exceeding max length limit of %d, disconnecting\n", state->id(), buffer->getRemainingLength(), 1024));
		return false;
	}

	if (state->m_seeded == false)
	{
		// check for HTTP connection
		if ((buffer->getRemainingLength() >= 5 && memcmp(buffer->getRemainingData(), "GET /", 5) == 0) ||
			(buffer->getRemainingLength() >= 6 && memcmp(buffer->getRemainingData(), "POST /", 6) == 0))
		{
			EXC_SET("http request");
			if (g_Cfg.m_fUseHTTP != 2)
				return false;

			client->SetConnectType(CONNECT_HTTP);
			if (client->OnRxWebPageRequest(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				return false;

			buffer->seek(buffer->getLength());
			return true;
		}

		// check for new seed (sometimes it's received on its own)
		else if ((buffer->getRemainingLength() == 1) && (buffer->getRemainingData()[0] == PACKET_NewSeed))
		{
			state->m_newseed = true;
			buffer->skip(1);
			return true;
		}

		// check for ping data
		else if (buffer->getRemainingLength() < 4)
		{
			EXC_SET("ping #1");
			if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
				state->markReadClosed();
			
			buffer->seek(buffer->getLength());
			return true;
		}

		// at least 4 bytes and not a web request, so must assume
		// it is a game client seed
		EXC_SET("game client seed");
		DWORD seed = 0;

		DEBUGNETWORK(("%lx:Client connected with a seed length of %" FMTSIZE_T " ([0]=0x%x)\n", state->id(), buffer->getRemainingLength(), static_cast<int>(buffer->getRemainingData()[0])));
		if (state->m_newseed || ((buffer->getRemainingData()[0] == PACKET_NewSeed) && (buffer->getRemainingLength() >= NETWORK_SEEDLEN_NEW)))
		{
			DEBUGNETWORK(("%lx:Receiving new client login handshake\n", state->id()));

			if (state->m_newseed == false)
			{
				// we don't need the command byte
				state->m_newseed = true;
				buffer->skip(1);
			}
			
			if (buffer->getRemainingLength() >= (NETWORK_SEEDLEN_NEW - 1))
			{
				seed = buffer->readInt32();
				DWORD versionMajor = buffer->readInt32();
				DWORD versionMinor = buffer->readInt32();
				DWORD versionRevision = buffer->readInt32();
				DWORD versionPatch = buffer->readInt32();

				DEBUG_WARN(("%lx:New Login Handshake Detected. Client Version: %" FMTDWORD ".%" FMTDWORD ".%" FMTDWORD ".%" FMTDWORD "\n", state->id(), versionMajor, versionMinor, versionRevision, versionPatch));
				state->m_reportedVersion = CCrypt::GetVerFromNumber(versionMajor, versionMinor, versionRevision, versionPatch);
			}
			else
			{
				DEBUGNETWORK(("%lx:Not enough data received to be a valid handshake (%" FMTSIZE_T ")\n", state->id(), buffer->getRemainingLength()));
			}
		}
		else if ((buffer->getRemainingData()[0] == PACKET_UOGRequest) && (buffer->getRemainingLength() == 8))
		{
			DEBUGNETWORK(("%lx:Receiving new UOG status request\n", state->id()));
			buffer->skip(7);
			buffer->getRemainingData()[0] = 0x7F;
			return true;
		}
		else
		{
			// assume it's a normal client login
			DEBUGNETWORK(("%lx:Receiving old client login handshake\n", state->id()));

			seed = buffer->readInt32();
		}

		DEBUGNETWORK(("%lx:Client connected with a seed of 0x" FMTDWORDH " (new handshake=%d, version=%" FMTDWORD ")\n", state->id(), seed, state->m_newseed ? 1 : 0, state->m_reportedVersion));

		if (seed == 0)
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%lx:Invalid client detected, disconnecting\n", state->id());
			return false;
		}

		state->m_seeded = true;
		state->m_seed = seed;
			
		if (buffer->getRemainingLength() <= 0 && state->m_seed == 0xFFFFFFFF)
		{
			// UO:KR Client opens connection with 255.255.255.255 and waits for the
			// server to send a packet before continuing
			EXC_SET("KR client seed");

			DEBUG_WARN(("UO:KR Client Detected\n"));
			client->SetConnectType(CONNECT_CRYPT);
			state->m_clientType = CLIENTTYPE_KR;
			new PacketKREncryption(client);
		}

		return true;
	}

	if (buffer->getRemainingLength() < 5)
	{
		// client has a seed assigned but hasn't send enough data to be anything useful,
		// some programs (ConnectUO) send a fake seed when really they're just sending
		// ping data
		EXC_SET("ping #2");
		if (client->OnRxPing(buffer->getRemainingData(), buffer->getRemainingLength()) == false)
			return false;

		buffer->seek(buffer->getLength());
		return true;
	}

	// process login packet for client
	EXC_SET("messages  - setup");
	client->SetConnectType(CONNECT_CRYPT);
	client->xProcessClientSetup(reinterpret_cast<CEvent*>(buffer->getRemainingData()), buffer->getRemainingLength());
	buffer->seek(buffer->getLength());
	return true;

	EXC_CATCH;
	return false;
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
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	ProfileTask networkTask(PROFILE_NETWORK_TX);
		
	static unsigned char tick = 0;
	EXC_TRY("NetworkOutput");
	++tick;

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
			++packetsSent;
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
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());
	
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
	ADDTOCALLSTACK("NetworkOutput::flush");
	ASSERT(state != NULL);

	if (state->isInUse() == false || state->isClosed())
		return 0;

	if (m_thread->isActive() && m_thread->isCurrentThread() == false)
	{
		// when this isn't the active thread, all we can do is raise a request to flush this
		// client later
		state->markFlush(true);
		if (m_thread->getPriority() == IThread::Disabled)
			m_thread->awaken();

		return 0;
	}
	
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
		++packetsSent;

	state->markFlush(false);
	return packetsSent;
}

size_t NetworkOutput::processPacketQueue(NetState* state, unsigned int priority)
{
	// process a client's packet queue
	ADDTOCALLSTACK("NetworkOutput::processPacketQueue");
	ASSERT(state != NULL);
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	if (state->isWriteClosed() ||
		(state->m_outgoing.queue[priority].empty() && state->m_outgoing.currentTransaction == NULL))
		return 0;

	CClient* client = state->m_client;
	ASSERT(client != NULL);

	size_t maxPacketsToProcess = NETWORK_MAXPACKETS;
	size_t maxLengthToProcess = NETWORK_MAXPACKETLEN;
	size_t packetsProcessed = 0;
	size_t lengthProcessed = 0;

	while (packetsProcessed < maxPacketsToProcess && lengthProcessed < maxLengthToProcess)
	{
		// select next transaction
		while (state->m_outgoing.currentTransaction == NULL)
		{
			if (state->m_outgoing.queue[priority].empty())
				break;
				
			state->m_outgoing.currentTransaction = state->m_outgoing.queue[priority].front();
			state->m_outgoing.queue[priority].pop();
		}

		PacketTransaction* transaction = state->m_outgoing.currentTransaction;
		if (transaction == NULL)
			break;

		// acquire next packet from transaction
		PacketSend* packet = transaction->front();
		transaction->pop();

		// if the transaction is now empty we can clear it now so we can move
		// on to the next transaction later
		if (transaction->empty())
		{
			state->m_outgoing.currentTransaction = NULL;
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
		++packetsProcessed;

		EXC_SET("sending");
		if (sendPacket(state, packet) == false)
		{
			state->clearQueues();
			state->markWriteClosed();
			break;
		}

		EXC_CATCH;
		EXC_DEBUG_START;
		g_Log.EventDebug("id='%lx', pri='%u', packet '%" FMTSIZE_T "' of '%" FMTSIZE_T "' to send, length '%" FMTSIZE_T "' of '%" FMTSIZE_T "'\n", state->id(), priority, packetsProcessed, maxPacketsToProcess, lengthProcessed, maxLengthToProcess);
		EXC_DEBUG_END;
	}

	if ( packetsProcessed >= maxPacketsToProcess )
		DEBUGNETWORK(("Reached maximum packet count limit for this tick (%" FMTSIZE_T "/%" FMTSIZE_T ")\n", packetsProcessed, maxPacketsToProcess));
	if ( lengthProcessed >= maxLengthToProcess )
		DEBUGNETWORK(("Reached maximum packet length limit for this tick (%" FMTSIZE_T "/%" FMTSIZE_T ")\n", lengthProcessed, maxLengthToProcess));

	return packetsProcessed;
}

size_t NetworkOutput::processAsyncQueue(NetState* state)
{
	// process a client's async queue
	ADDTOCALLSTACK("NetworkOutput::processAsyncQueue");
	ASSERT(state != NULL);
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	if (state->isWriteClosed() || state->isAsyncMode() == false)
		return 0;

	if (state->m_outgoing.asyncQueue.empty() || state->isSendingAsync())
		return 0;

	const CClient* client = state->m_client;
	ASSERT(client != NULL);

	// select the next packet to send
	PacketSend* packet = NULL;
	while (state->m_outgoing.asyncQueue.empty() == false)
	{
		packet = state->m_outgoing.asyncQueue.front();
		state->m_outgoing.asyncQueue.pop();

		if (packet != NULL)
		{
			// check if the client is allowed this
			if (state->canReceive(packet) && packet->onSend(client))
				break;

			// destroy the packet, we aren't going to use it
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
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	if (state->isWriteClosed() || state->m_outgoing.bytes.GetDataQty() <= 0)
		return false;

	size_t result = sendData(state, state->m_outgoing.bytes.RemoveDataLock(), state->m_outgoing.bytes.GetDataQty());
	if (result == _failed_result())
	{
		// error occurred
		state->clearQueues();
		state->markWriteClosed();
		return false;
	}
		
	if (result > 0)
		state->m_outgoing.bytes.RemoveDataAmount(result);

	return true;
}

bool NetworkOutput::sendPacket(NetState* state, PacketSend* packet)
{
	// send packet to client (can be queued for async operation)
	ADDTOCALLSTACK("NetworkOutput::sendPacket");
	ASSERT(state != NULL);
	ASSERT(packet != NULL);
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	// check the client is allowed the packet
	if (state->canReceive(packet) == false)
	{
		delete packet;
		return false;
	}

	if (state->isAsyncMode())
	{
		state->m_outgoing.asyncQueue.push(packet);
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
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	CClient* client = state->m_client;
	ASSERT(client != NULL);

	EXC_TRY("sendPacketData");
#ifdef _DEBUG
	xRecordPacket(client, packet, "server->client");
#endif

	EXC_SET("send trigger");
	if (packet->onSend(client) == false)
	{
		delete packet;
		return true;
	}

	if (client->xOutPacketFilter(packet->getData(), packet->getLength()) == true)
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
	state->m_outgoing.bytes.AddNewData(sendBuffer, sendBufferLength);

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
	g_Log.EventDebug("id='%lx', packet '0x%x', length '%" FMTSIZE_T "'\n", state->id(), *packet->getData(), packet->getLength());
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
	ASSERT(!m_thread->isActive() || m_thread->isCurrentThread());

	EXC_TRY("sendData");
	size_t result = 0;

	EXC_SET("sending");

#if defined(_WIN32) && !defined(_LIBEV)
	if (state->isAsyncMode())
	{
		// send via async winsock
		ZeroMemory(&state->m_overlapped, sizeof(WSAOVERLAPPED));
		state->m_overlapped.hEvent = state;
		state->m_bufferWSA.len = static_cast<ULONG>(length);
		state->m_bufferWSA.buf = reinterpret_cast<CHAR *>(const_cast<BYTE *>(data));

		DWORD bytesSent;
		if (state->m_socket.SendAsync(&state->m_bufferWSA, 1, &bytesSent, 0, &state->m_overlapped, SendCompleted) != SOCKET_ERROR)
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
		int sent = state->m_socket.Send(data, static_cast<int>(length));
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

			// Connection error should clear the client too
			result = _failed_result();
		}
	}

	if (result > 0 && result != _failed_result())
		CurrentProfileData.Count(PROFILE_DATA_TX, static_cast<DWORD>(result));

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
		
	// we could process another batch of async data right now, but only if we
	// are in the output thread
	// - WinSock calls this from the same thread
	// - LinuxEv calls this from a completely different thread
	if (m_thread->isActive() && m_thread->isCurrentThread())
	{
		if (processAsyncQueue(state) > 0)
			processByteQueue(state);
	}
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

	if (state->m_outgoing.pendingTransaction != NULL && appendTransaction)
		state->m_outgoing.pendingTransaction->push_back(packet);
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
		while (priority > PacketSend::PRI_IDLE && state->m_outgoing.queue[priority].size() >= maxQueueSize)
		{
			transaction->setPriority(priority--);
		}
	}

	state->m_outgoing.queue[priority].push(transaction);

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
