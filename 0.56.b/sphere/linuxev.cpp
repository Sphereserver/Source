#ifndef _WIN32

#include "linuxev.h"
#include "../graysvr/graysvr.h"
#include "../network/network.h"

LinuxEv g_NetworkEvent;

static void socketmain_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{
			// warning: accepting a new connection here can result in a threading issue,
			// where the main thread can clear the connection before it has been fully
			// initialised
			g_NetworkIn.acceptConnection();
		}
	}
	
	ev_io_start(loop, w);
}

static void socketslave_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	CClient * theClient = reinterpret_cast<CClient *>( w->data );
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			// g_NetworkOut.onAsyncSendComplete(theClient);
		}		
		else if ( revents & EV_WRITE )
		{	
			g_NetworkOut.onAsyncSendComplete(theClient);
		}
	}
	
	if ( theClient->GetNetState()->isSendingAsync() )
	{
		ev_io_start(loop, w);
	}
}

LinuxEv::LinuxEv(void) : AbstractSphereThread("NetworkEvents", IThread::High), m_active(true)
{
	m_eventLoop = ev_loop_new(EV_BACKEND_LIST);
	ev_set_io_collect_interval(m_eventLoop, 0.01);
	
	memset(&m_watchMainsock, 0, sizeof(struct ev_io));
}

LinuxEv::~LinuxEv(void)
{
	ev_loop_destroy(m_eventLoop);
}

void LinuxEv::onStart()
{
	// g_Log.Event(LOGM_CLIENTS_LOG, "Event start backend 0x%x\n", ev_backend(m_eventLoop));
}
	
void LinuxEv::tick()
{	
	ev_loop(m_eventLoop, 0);
}

void LinuxEv::waitForClose()
{
	m_active = false;
	ev_unloop(m_eventLoop, EVUNLOOP_ALL);
}

bool LinuxEv::shouldExit()
{
	return !m_active;
}

void LinuxEv::registerClient(CClient * theClient, EventsID eventCheck)
{
	NetState* state = theClient->GetNetState();
	
	memset(state->iocb(), 0, sizeof(struct ev_io));
		
	ev_io_init(state->iocb(), socketslave_cb, state->m_socket.GetSocket(), (int)eventCheck);
	state->iocb()->data = theClient;
	state->setSendingAsync(true);
	
    ev_io_start(m_eventLoop, state->iocb());	
}

void LinuxEv::unregisterClient(CClient * theClient)
{
	NetState* state = theClient->GetNetState();

	state->setSendingAsync(false);
	
	ev_io_stop(m_eventLoop, state->iocb());
}

void LinuxEv::forceClientevent(CClient * theClient, EventsID eventForce)
{
	NetState* state = theClient->GetNetState();
	ev_invoke(m_eventLoop, state->iocb(), (int)eventForce);
}

void LinuxEv::forceClientread(CClient * theClient)
{
	forceClientevent(theClient, LinuxEv::Read);
}

void LinuxEv::forceClientwrite(CClient * theClient)
{
	forceClientevent(theClient, LinuxEv::Write);
}

void LinuxEv::registerMainsocket()
{
	ev_io_init(&m_watchMainsock, socketmain_cb, g_Serv.m_SocketMain.GetSocket(), EV_READ);
    ev_io_start(m_eventLoop, &m_watchMainsock);		
}

void LinuxEv::unregisterMainsocket()
{
	ev_io_stop(m_eventLoop, &m_watchMainsock);
}

#endif
