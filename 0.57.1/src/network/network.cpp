#include "network.h"
#include "receive.h"
#include "send.h"

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
	clear();
}

NetState::~NetState()
{
}

long NetState::id()
{
	return m_id;
}

void NetState::clear()
{
	if ( m_client )
	{
		g_Serv.StatDec(SERV_STAT_CLIENTS);
		g_Log.Event(LOGM_CLIENTS_LOG, "%x:Client disconnected [Total:%d] ('%s')\n",
			m_id, g_Serv.StatGet(SERV_STAT_CLIENTS), m_peerAddr.GetAddrStr());

	}
	for ( int i = 0; i < PacketSend::PRI_QTY; i++ )
	{
		while ( m_queue[i].size() )
		{
			delete m_queue[i].front();
			m_queue[i].pop();
		}
	}
	m_sequence = 0;
	m_seeded = false;
	m_seed = 0;
	m_sock.Close();
	m_clientVersion = 0;
	m_toClose = false;
	m_client = NULL;
}

void NetState::init(SOCKET sock, CSocketAddress addr)
{
	m_peerAddr = addr;
	if ( m_client )
	{
		//	record the client reference to the garbage collection to be deleted on it's time
		g_World.m_ObjDelete.InsertHead(m_client);
	}
	clear();
	g_Serv.StatInc(SERV_STAT_CLIENTS);
	m_sock = sock;
	m_client = new CClient(this);
	m_sock.SetNonBlocking();
}

bool NetState::isValid(CClient *client)
{
	if ( client == NULL )
		return ( m_sock.IsOpen() && !m_toClose );
	else
		return ( m_sock.IsOpen() && ( m_client == client ));
}

void NetState::markClose()
{
	m_toClose = true;
}

bool NetState::toClose()
{
	return m_toClose;
}

/***************************************************************************
 *
 *
 *	class ClientIterator		Works as client iterator getting the clients
 *
 *
 ***************************************************************************/

ClientIterator::ClientIterator()
{
	m_id = -1;
	m_max = g_Network ? config.get("network.client.max") : -1;
}

ClientIterator::~ClientIterator()
{
}

CClient *ClientIterator::next()
{
	while ( ++m_id < m_max )
	{
		NetState *state = g_Network->m_states[m_id];
		if ( state->isValid() && state->m_client )
			return state->m_client;
	}
	return NULL;
}

/***************************************************************************
 *
 *
 *	class Network::HistoryIP	Simple interface for IP history maintainese
 *
 *
 ***************************************************************************/

void NetworkIn::HistoryIP::update()
{
	m_ttl = config.get("network.iphist.ttl");
}

bool NetworkIn::HistoryIP::checkPing()
{
	update();

	return ( m_blocked || ( m_pings++ >= config.get("network.iphist.pings") ));
}

/***************************************************************************
 *
 *
 *	class NetworkIn				Networking thread used for receiving client data
 *
 *
 ***************************************************************************/

NetworkIn *g_Network;
const char *NetworkIn::m_sClassName = "NetworkIn";

NetworkIn::NetworkIn()
{
	m_lastGivenSlot = -1;

	memset(m_handlers, 0, sizeof(m_handlers));
	memset(m_extended, 0, sizeof(m_extended));
	memset(m_encoded, 0, sizeof(m_encoded));

#ifdef DEBUGPACKETS
	g_Log.Debug("Registering packets...\n");
#endif
	registerPacket(0x02, new PacketMovementReq());
	registerPacket(0x13, new PacketEquipReq());
	registerPacket(0x22, new PacketResynchronize());
	registerPacket(0x34, new PacketCharStatusReq());
	registerPacket(0x73, new PacketPingReq());
	registerPacket(0xa4, new PacketDummy(0x95));
	registerPacket(0xd1, new PacketLogoutReq());
	registerPacket(0xd9, new PacketDummy(-1));
	/*
	registerPacket(0x00, new CreateCharacter());
	registerPacket(0x03, new AsciiSpeech());
	registerPacket(0x05, new AttackReq());
	registerPacket(0x06, new UseReq());
	registerPacket(0x07, new LiftReq());
	registerPacket(0x08, new DropReq());
	registerPacket(0x09, new LookReq());
	registerPacket(0x0A, new Edit());
	registerPacket(0x12, new TextCommand());
	registerPacket(0x14, new ChangeZ());
	registerPacket(0x2C, new DeathStatusResponse());
	registerPacket(0x3A, new ChangeSkillLock());
	registerPacket(0x47, new NewTerrain());
	registerPacket(0x48, new NewAnimData());
	registerPacket(0x58, new NewRegion());
	registerPacket(0x5D, new PlayCharacter());
	registerPacket(0x61, new DeleteStatic());
	registerPacket(0x6C, new TargetResponse());
	registerPacket(0x6F, new SecureTrade());
	registerPacket(0x72, new SetWarMode());
	registerPacket(0x75, new RenameRequest());
	registerPacket(0x79, new ResourceQuery());
	registerPacket(0x7D, new MenuResponse());
	registerPacket(0x80, new AccountLogin());
	registerPacket(0x83, new DeleteCharacter());
	registerPacket(0x91, new GameLogin());
	registerPacket(0x95, new HuePickerResponse());
	registerPacket(0x98, new MobileNameRequest());
	registerPacket(0x9A, new AsciiPromptResponse());
	registerPacket(0x9B, new HelpRequest());
	registerPacket(0x9D, new GMSingle());
	registerPacket(0xA0, new PlayServer());
	registerPacket(0xA7, new RequestScrollWindow());
	registerPacket(0xAD, new UnicodeSpeech());
	registerPacket(0xB1, new DisplayGumpResponse());
	registerPacket(0xB5, new ChatRequest());
	registerPacket(0xB6, new ObjectHelpRequest());
	registerPacket(0xB8, new ProfileReq());
	registerPacket(0xBB, new AccountID());
	registerPacket(0xBD, new ClientVersion());
	registerPacket(0xBE, new AssistVersion());
	registerPacket(0xBF, new ExtendedCommand());
	registerPacket(0xC2, new UnicodePromptResponse());
	registerPacket(0xC8, new SetUpdateRange());
	registerPacket(0xC9, new TripTime());
	registerPacket(0xCA, new UTripTime());
	registerPacket(0xCF, new AccountLogin());
	registerPacket(0xD0, new ConfigurationFile());
	registerPacket(0xD6, new BatchQueryProperties());
	registerPacket(0xD7, new EncodedCommand());
	
	registerExtended(0x05, new ScreenSize());
	registerExtended(0x06, new PartyMessage());
	registerExtended(0x07, new QuestArrow());
	registerExtended(0x09, new DisarmRequest());
	registerExtended(0x0A, new StunRequest());
	registerExtended(0x0B, new Language());
	registerExtended(0x0C, new CloseStatus());
	registerExtended(0x0E, new Animate());
	registerExtended(0x0F, new Empty()); // What's this?
	registerExtended(0x10, new QueryProperties());
	registerExtended(0x13, new ContextMenuRequest());
	registerExtended(0x15, new ContextMenuResponse());
	registerExtended(0x1A, new StatLockChange());
	registerExtended(0x1C, new CastSpell());
	registerExtended(0x24, new UnhandledBF());
	  
	registerEncoded(0x19, new SetAbility());
	registerEncoded(0x28, new GuildGumpRequest());
	*/

	m_states = (NetState**)malloc(config.get("network.client.max") * sizeof(NetState*));
	for ( long l = 0; l < config.get("network.client.max"); l++ )
	{
		m_states[l] = new NetState(l);
	}

#ifdef DEBUGPACKETS
	g_Log.Debug("done\n");
#endif
}

long NetworkIn::getStateSlot(long startFrom)
{
	if ( startFrom == -1 )
		startFrom = m_lastGivenSlot+1;

	//	give ordered slot number, each time incrementing by 1 for easier log view
	long max = config.get("network.client.max");
	for ( long l = startFrom; l < max; l++ )
	{
		if ( !m_states[l]->isValid() )
		{
			return ( m_lastGivenSlot = l );
		}
	}

	//	we did not find empty slots till the end, try rescan from beginning
	if ( startFrom > 0 )
		return getStateSlot(0);

	//	no empty slots exists, arbitrary too many clients
	return -1;
}

#ifdef _WIN32
#define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); closesocket(_x_); }
#else
#define CLOSESOCKET(_x_)	{ shutdown(_x_, 2); close(_x_); }
#endif

#define DROPCLIENT			{ client->markClose(); continue; }

#define MOVEBUF(_x_)		{ buffer += _x_; received -= _x_; }

THREAD_ENTRY_RET _cdecl NetworkIn::EntryProc(void *lpThreadParameter)
{
	CThread *thread = (CThread *)lpThreadParameter;
	
	thread->OnCreate();
	long l = 0;
	int deadSock;
	BYTE *buf = (BYTE *)malloc(0xf000);
	BYTE *decryptBuf = (BYTE *)malloc(0xf000);
	long max = config.get("network.client.max");
	while ( !g_Serv.m_iExitFlag )
	{
		thread->OnTick(config.get("network.recv.sleep"));

		if ( g_Serv.m_iModeCode != SERVMODE_Run )
			continue;
		
		EXC_TRY("Entry");

		if ( l > 20 )	// approx once per second
			l = 0;

		if ( l == 0 )
		{
			EXC_SET("pereodic");

			//	from time to time rebuild constants
			deadSock = config.get("network.client.dead");

			//	and do maintainer's actions
			g_Network->pereodically();
		}

		//	check for incoming packets
		EXC_SET("select");
		fd_set readfds;
		g_Network->checkForData(&readfds);
		
		EXC_SET("new connection");
		if ( FD_ISSET(g_Serv.m_SocketMain.GetSocket(), &readfds) )
			g_Network->acceptConnection();
		
		EXC_SET("messages");
		for ( long i = 0; i < max; i++ )
		{
			NetState *client = g_Network->m_states[i];

			if ( !client->m_client )
				continue;

			if ( !FD_ISSET(client->m_sock.GetSocket(), &readfds) )
			{
				if ( client->m_client->GetConnectType() != CONNECT_TELNET )
				{
					//	clients are pinged from time to time
					//	if no event happened for some time, assume the client being hang
					int sinceLastEvent = -g_World.GetTimeDiff(client->m_client->m_timeLastEvent);

					if ( deadSock && ( sinceLastEvent > deadSock ))
					{
						g_Log.Error("%x:Frozen client connection disconnected.\n", client->m_id);
						client->markClose();
					}
				}
				continue;
			}

			//	read data from the client
			BYTE *buffer = buf;
			int received = client->m_sock.Receive(buf, 0xf000, 0);
			if ( received <= 0 )
				DROPCLIENT;

			if ( client->m_client->GetConnectType() == CONNECT_UNK )
			{
			
				if ( !client->m_seeded )
				{
					//	seed the login connection
					if ( received >= 4 )
					{
						if ( !memcmp(buf, "GET /", 5) )						//	HTTP
						{
							if ( g_Cfg.m_fUseHTTP != 2 )
								DROPCLIENT;
							
							client->m_client->SetConnectType(CONNECT_HTTP);
							
							if ( !client->m_client->OnRxWebPageRequest(buf, received) )
								DROPCLIENT;
							continue;
						}
						DWORD seed = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
						if ( !seed )
						{
							g_Log.Error("Invalid client %d detected, disconnecting", client->id());
							DROPCLIENT;
						}
						client->m_seeded = true;
						client->m_seed = seed;
						MOVEBUF(4);
					}
					//	some sort of special connection?
					else if ( received == 1 )
					{
						if (( *buffer == '\x1' ) || ( *buffer == ' ' ))		//	TELNET
						{
							client->m_client->SetConnectType(CONNECT_TELNET);
							client->m_client->m_zLogin[0] = 0;
							client->m_client->SysMessagef("Welcome to %s Administrative Telnet\n", g_Serv.GetName());

							if ( client->m_peerAddr.IsLocalAddr() )
							{
								CAccountRef pAccount = g_Accounts.Account_Find("Administrator");
								if ( !pAccount )
									pAccount = g_Accounts.Account_Find("RemoteAdmin");
								if ( pAccount )
								{
									CGString sMsg;
									BYTE lErr = client->m_client->LogIn(pAccount, sMsg);
									if ( lErr != PacketLoginError::Success )
									{
										if ( lErr != PacketLoginError::Invalid )
											client->m_client->SysMessage(sMsg);
										DROPCLIENT;
									}
									if ( client->m_client->OnRxConsoleLoginComplete() )
										DROPCLIENT;
								}
							}

							client->m_client->SysMessage("Login?:\n");
						}
						else if ( (*buffer == 0xff) || (*buffer == 0x22) )		//	UOG request
						{
							client->m_client->SetConnectType(CONNECT_TELNET);
							g_Log.Event(LOGM_CLIENTS_LOG|LOGL_EVENT, "%x:UOG Status request from %s\n",
								client->m_id,
								client->m_peerAddr.GetAddrStr());
							client->m_client->SysMessage(g_Serv.GetStatusString(0x22));
							client->m_client->SetConnectType(CONNECT_UNK);
						}
						continue;
					}
					else
						DROPCLIENT;
				}

				if ( !received )
					continue;

				//	log in the client
				client->m_client->SetConnectType(CONNECT_CRYPT);
				client->m_client->xProcessClientSetup((CEvent*)buffer, received);

				continue;
			}

			client->m_client->m_timeLastEvent = CServTime::GetCurrentTime();

			CEvent	event;
			memcpy(&event, buf, sizeof(event));

			//	first data on a new connection -> find out what should come next
			if ( !client->m_client->m_Crypt.IsInit() )
			{
				switch ( client->m_client->GetConnectType() )
				{
				case CONNECT_CRYPT:
					client->m_client->xProcessClientSetup(&event, received);
					break;
					
				case CONNECT_HTTP:
					if ( !client->m_client->OnRxWebPageRequest(event.m_Raw, received) )
						DROPCLIENT;
					break;
					
				case CONNECT_TELNET:
					if ( !client->m_client->OnRxConsole(event.m_Raw, received) )
						DROPCLIENT;
					break;
					
				default:
					g_Log.Event(LOGM_CLIENTS_LOG,"%x:Junk messages with no crypt\n", client->m_id);
					DROPCLIENT;
				}
				continue;
			}
			
			//	decrypt the client data and add it to queue
			client->m_client->m_Crypt.Decrypt(decryptBuf, buffer, received);
			Packet *packet = new Packet(decryptBuf, received);

			//	process the message
			EXC_TRYSUB("ProcessMessage");

			long len = packet->length();
			long offset = 0;
			while (( len > 0 ) && !client->toClose() )
			{
				long packetID = packet->data()[offset];
				Packet *handler = g_Network->m_handlers[packetID];

				//	Packet filtering - check if any function triggering is installed
				//		allow skipping the packet which we do not wish to get
				if ( g_Serv.m_PacketFilter[packetID][0] )
				{
					BYTE *data = packet->data() + offset;
					long bytes = packet->length() - offset;
					CScriptTriggerArgs args(data[0] + offset);
					enum TRIGRET_TYPE tr;
					char idx[5];

					TEMPSTRING(tempBuf);
					memcpy(tempBuf, data, bytes);
					tempBuf[bytes] = 0;

					args.m_s1 = client->m_peerAddr.GetAddrStr();
					args.m_VarsLocal.SetNum("NUM", bytes);
					args.m_VarsLocal.SetStr("STR", false, tempBuf);
					if ( client->m_client->GetAccount() )
						args.m_VarsLocal.SetStr("ACCOUNT", false, client->m_client->GetAccount()->GetName());
					if ( client->m_client->GetChar() )
						args.m_VarsLocal.SetNum("CHAR", false, client->m_client->GetChar()->uid());

					for ( int i = 0; i < bytes; i++ )
						args.m_VarsLocal.SetNum(ITOA(i, idx), (int)data[i]);

					if ( g_Serv.r_Call(g_Serv.m_PacketFilter[packetID], &g_Serv, &args, NULL, &tr) )
					{
						if ( tr == TRIGRET_RET_TRUE )
							break;
					}
				}

#ifdef DEBUGPACKETS
				g_Log.Debug("TX RECV %s", packet->dump());
#endif
				if ( !handler )
				{
					Packet *oldPacket = new Packet(packet->data() + offset, len);
					long curPktLen = client->m_client->xDispatchMsg(oldPacket);
					len -= curPktLen;
					offset += curPktLen;
					delete oldPacket;
				}
				else
				{
					long packetLen = packet->getValidLength();
#ifdef DEBUGPACKETS
					g_Log.Debug("Checking length: counted %d.\n", packetLen);
#endif
					//	fall back and delete the packet
					if ( !packetLen )
						break;

					len -= packetLen;
					offset += packetLen;

					handler->seek();
					for ( int i = 0; i < packetLen; i++ )
					{
						BYTE next = packet->readBYTE();
						handler->write(next);
					}
					handler->seek(1);
					handler->onReceive(client);
				}
			}

			EXC_CATCHSUB("Network");
			EXC_DEBUGSUB_START;
			g_Log.Debug("Parsing %s", packet->dump());
			EXC_DEBUGSUB_END;
			delete packet;
		}
		EXC_CATCH;
		l++;
	}
	thread->OnClose();

	g_Serv.m_SocketMain.Close();
	free(buf);
	free(decryptBuf);
}

void NetworkIn::Start()
{
	m_name = "NetworkIn Task";
	CThread::Create(EntryProc);
}

void NetworkIn::checkForData(fd_set *storage)
{
//	Berkeley sockets needs nfds to be updated that while in Windows that's ignored
#ifdef _WIN32
#define ADDTOSELECT(_x_)	FD_SET(_x_, storage)
#else
#define ADDTOSELECT(_x_)	{ FD_SET(_x_, storage); if ( _x_ > nfds ) nfds = _x_; }
#endif

	int nfds = 0;
		
	FD_ZERO(storage);
	ADDTOSELECT(g_Serv.m_SocketMain.GetSocket());
	long max = config.get("network.client.max");
	for ( long l = 0; l < max; l++ )
	{
		NetState *state = m_states[l];
		if ( !state->m_sock.IsOpen() )
			continue;

		if ( state->toClose() )
		{
#ifdef DEBUGPACKETS
			g_Log.Debug("Client '%x' is gonna being cleared since marked to close.\n",
				state->id());
#endif
			state->clear();
			continue;
		}
		ADDTOSELECT(state->m_sock.GetSocket());
	}

	timeval Timeout;	// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000
	select(nfds+1, storage, NULL, NULL, &Timeout);

#undef ADDTOSELECT
}

void NetworkIn::acceptConnection()
{
	EXC_TRY("acceptConnection");
	CSocketAddress client_addr;

	EXC_SET("accept");
	SOCKET h = g_Serv.m_SocketMain.Accept(client_addr);
	if (( h >= 0 ) && ( h != INVALID_SOCKET ))
	{
		EXC_SET("ip history");
		HistoryIP *ip = &getHistoryForIP(client_addr);
		long maxIp = config.get("network.conn.maxip");
		long climaxIp = config.get("network.client.maxip");

#ifdef DEBUGPACKETS 
		g_Log.Debug("Incoming connection from '%s' [blocked=%d, ttl=%d, pings=%d, connecting=%d, connected=%d]\n",
			ip->m_ip.GetAddrStr(), ip->m_blocked, ip->m_ttl, ip->m_pings, ip->m_connecting, ip->m_connected);
#endif
		//	ip is blocked
		if ( ip->checkPing() ||
			// or too much connect tries from this ip
			( maxIp && ( ip->m_connecting > maxIp )) ||
			// or too much clients from this ip
			( climaxIp && ( ip->m_connected > climaxIp ))
			)
		{
			EXC_SET("rejecting");
#ifdef DEBUGPACKETS 
			g_Log.Debug("Closing incoming connection [max ip=%d, clients max ip=%d).\n",
				maxIp, climaxIp);
#endif
			CLOSESOCKET(h);
		}
		else
		{
			EXC_SET("detecting slot");
			long slot = getStateSlot();
			if ( slot == -1 )			// we do not have enough empty slots for clients
			{
#ifdef DEBUGPACKETS 
				g_Log.Debug("Unable to allocate new slot for client, too many clients already.\n");
#endif
				CLOSESOCKET(h);
			}
			else
			{
				m_states[slot]->init(h, client_addr);
			}
		}
	}
	EXC_CATCH;
}

void NetworkIn::registerPacket(int packetId, Packet *handler)
{
	if (( packetId >= 0 ) && ( packetId < 0x100 ))
		m_handlers[packetId] = handler;
}

void NetworkIn::registerExtended(int packetId, Packet *handler)
{
	if (( packetId >= 0 ) && ( packetId < 0x100 ))
		m_extended[packetId] = handler;
}

void NetworkIn::registerEncoded(int packetId, Packet *handler)
{
	if (( packetId >= 0 ) && ( packetId < 0x100 ))
		m_encoded[packetId] = handler;
}

NetworkIn::HistoryIP &NetworkIn::getHistoryForIP(CSocketAddressIP ip)
{
	vector<HistoryIP>::iterator it;

	for ( it = m_ips.begin(); it != m_ips.end(); it++ )
	{
		if ( (*it).m_ip == ip )
			return *it;
	}

	HistoryIP hist;
	memset(&hist, 0, sizeof(hist));
	hist.m_ip = ip;
	hist.update();

	m_ips.push_back(hist);
	return getHistoryForIP(ip);
}

NetworkIn::HistoryIP &NetworkIn::getHistoryForIP(char *ip)
{
	CSocketAddressIP	me(ip);

	return getHistoryForIP(me);
}

void NetworkIn::pereodically()
{
	//	Periodic network tasks happen here
	CClient *client;
	int		connecting = 0;
	int		connectingMax = config.get("network.conn.max");
	long	decaystart = config.get("network.iphist.pings")/2;
	long	decayttl = config.get("network.iphist.ttl")/2;
	int		i;

	EXC_TRY("pereodically");
	//	Check if max connecting limit is obeyed
	if ( connectingMax )
	{
		EXC_SET("limiting connecting clients");
		ClientIterator clients;
		while ( client = clients.next() )
		{
			if ( client->IsConnecting() )
			{
				connecting++;
				if ( connecting > connectingMax )
				{
#ifdef DEBUGPACKETS
					g_Log.Debug("Closing client '%x' since '%d' connecting overlaps '%d'\n",
						client->m_net->id(), connecting, connectingMax);
#endif
					client->m_net->markClose();
				}
			}
		}
		if ( connecting > connectingMax )
		{
			g_Log.Warn("%d clients in connect mode (max %d), closing %d\n",
				connecting, connectingMax, connecting - connectingMax);
		}
	}

	//	Tick the ip history, maybe remove some from the list
	EXC_SET("ticking history");
	for ( i = 0; i < m_ips.size(); i++ )
	{
		HistoryIP *ip = &m_ips[i];
		if ( ip->m_blocked )		// blocked IP does not decay
			continue;

		ip->m_ttl--;
		//	decay pings history with time
		if (( ip->m_pings > decaystart ) && ( ip->m_ttl < decayttl ))
			ip->m_pings--;
	}

	//	Clear old IP history (decaying 1 ip per time is ok enough for us)
	EXC_SET("clearing history");
	for ( vector<HistoryIP>::iterator it = m_ips.begin(); it != m_ips.end(); it++ )
	{
		if ( it->m_ttl < 0 )
		{
			m_ips.erase(it);
			break;
		}
	}
	EXC_CATCH;
}

Packet *NetworkIn::getHandler(BYTE packetID)
{
	return m_handlers[packetID];
}

/***************************************************************************
 *
 *
 *	class NetworkOut			Networking thread used for queued sending outgoing packets
 *
 *
 ***************************************************************************/

const char *NetworkOut::m_sClassName = "NetworkOut";
NetworkOut *g_Sender;

NetworkOut::NetworkOut()
{
}

void NetworkOut::schedule(PacketSend *packet)
{
	scheduleOnce(new PacketSend(packet));
}

void NetworkOut::scheduleOnce(PacketSend *packet)
{
	NetState *state = packet->m_target;
	long priority = packet->getPriority();

	//	limit by number of packets to be in queue
	if ( priority > PacketSend::PRI_IDLE )
	{
		long maxClientPackets = config.get("network.packet.max");

		if ( state->m_queue[priority].size() >= maxClientPackets )
		{
#ifdef DEBUGPACKETS
			g_Log.Debug("%x:Packet decreased priority due to overal amount %d overlapping %d.\n",
				state->id(), state->m_queue[priority].size(), maxClientPackets);
#endif
			packet->m_priority = priority-1;
			scheduleOnce(packet);
			return;
		}
	}

	state->m_queue[priority].push(packet);
}

THREAD_ENTRY_RET _cdecl NetworkOut::EntryProc(void *lpThreadParameter)
{
	CThread *thread = (CThread *)lpThreadParameter;
	
	thread->OnCreate();
	long l = 0;
	while ( !g_Serv.m_iExitFlag )
	{
		thread->OnTick(config.get("network.send.sleep"));

		if ( g_Serv.m_iModeCode != SERVMODE_Run )
			continue;
		
		EXC_TRY("Entry");
		l++;

		//	HIGHEST		each tick (Xms)
		EXC_SET("highest");
		g_Sender->proceedQueue(PacketSend::PRI_HIGHEST);

		//	HIGH		each 1/2 tick (X*2ms)
		EXC_SET("high");
		if ( (l % 2) == 1 )
			g_Sender->proceedQueue(PacketSend::PRI_HIGH);

		//	NORMAL		each 1/4 tick (X*4ms)
		EXC_SET("normal");
		if ( (l % 4) == 3 )
			g_Sender->proceedQueue(PacketSend::PRI_NORMAL);

		//	LOW			each 1/8 tick (X*8ms)
		EXC_SET("low");
		if ( (l % 8) == 7 )
			g_Sender->proceedQueue(PacketSend::PRI_LOW);

		//	IDLE		each 1/16 tick (X*16ms)
		EXC_SET("idle");
		if ( (l % 16) == 15 )
			g_Sender->proceedQueue(PacketSend::PRI_IDLE);

		EXC_CATCH;
	}
	thread->OnClose();
}

void NetworkOut::Start()
{
	m_name = "NetworkOut Task";
	CThread::Create(EntryProc);
}

void NetworkOut::proceedQueue(long priority)
{
	long maxClientPackets = config.get("network.packet.max");
	long maxClientLength = config.get("network.packet.limit");
	CServTime time = CServTime::GetCurrentTime();

	ClientIterator clients;
	while ( CClient *client = clients.next() )
	{
		NetState *state = client->m_net;
		long packets = state->m_queue[priority].size();
		long length = 0;

		if ( !packets )
			continue;

		if ( packets > maxClientPackets )
			packets = maxClientPackets;

		// send N packets from the queue
		for ( int i = 0; i < packets; i++ )
		{
			PacketSend *packet = state->m_queue[priority].front();
			if ( !packet || !packet->m_target->m_client )
				break;

			EXC_TRY("proceedQueue");
			// remove early to get loop of stuck on exceptions
			state->m_queue[priority].pop();
			length += packet->length();

			EXC_SET("send trigger");
			if ( packet->onSend(client) )
			{
				int ret = 0;
				if ( !state->m_client )
				{
#ifdef DEBUGPACKETS
					g_Log.Debug("%x:Sending packet to closed client?\n", state->id());
#endif
					ret = state->m_sock.Send(packet->m_buf, packet->m_len);
				}
				else
				{
					client->m_timeLastSend = time;

					bool compressed = false;
					//	game packets require packet encrypting
					if ( client->GetConnectType() == CONNECT_GAME )
					{
						EXC_SET("compress and encrypt");
						ret = client->xCompress(client->sm_xCompress_Buffer, packet->m_buf, packet->m_len);
						compressed = true;
						if ( client->m_Crypt.GetEncryptionType() == ENC_TFISH )
						{
							client->m_Crypt.Encrypt(client->sm_xCompress_Buffer, client->sm_xCompress_Buffer, ret);
						}
					}

					EXC_SET("sending");
					if ( compressed )
						ret = state->m_sock.Send(client->sm_xCompress_Buffer, ret);
					else
						ret = state->m_sock.Send(packet->m_buf, packet->m_len);
				}

				if ( !ret )
				{
					EXC_SET("error parse");
					int errCode = CGSocket::GetLastError();
#ifdef _WIN32
					if (( errCode == WSAECONNRESET ) || ( errCode == WSAECONNABORTED ))
						state->markClose();
#endif
					g_Log.Warn("%x:TX Error %d\n", state->m_id, errCode);
				}
			}

			EXC_SET("deleting");
			delete packet;

			EXC_SET("check length");
			if ( length > maxClientLength )
			{
#ifdef DEBUGPACKETS
				g_Log.Debug("%x:Packets sending stopped at %d packet due to overal length %d overlapping %d.\n",
					state->id(), i, length, maxClientLength);
#endif
				break;
			}

			EXC_CATCH;
			EXC_DEBUG_START;
			g_Log.Debug("id='%x', pri='%d', packet '%d' of '%d' to send, length '%d' of '%d'\n",
				state->id(), priority, i, packets, length, maxClientLength);
			EXC_DEBUG_END;
		}
	}
}
