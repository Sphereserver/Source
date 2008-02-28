#ifndef _WIN32

#include "linuxev.h"
#include "../graysvr/graysvr.h"

LinuxEv g_NetworkEvent;

static void socketmain_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			g_Serv.SocketsReceive( g_Serv.m_SocketMain );
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
			// theClient->xAsyncSendComplete();
		}		
		else if ( revents & EV_WRITE )
		{	
			theClient->xAsyncSendComplete();
		}
	}
	
	if ( theClient->xCanSend() )
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
	memset(theClient->GetIOCB(), 0, sizeof(struct ev_io));
		
	ev_io_init(theClient->GetIOCB(), socketslave_cb, theClient->m_Socket.GetSocket(), (int)eventCheck);
	theClient->GetIOCB()->data = theClient;
	theClient->xSetCanSend(true);
	
    ev_io_start(m_eventLoop, theClient->GetIOCB());	
}

void LinuxEv::unregisterClient(CClient * theClient)
{
	theClient->xSetCanSend(false);
	
	ev_io_stop(m_eventLoop, theClient->GetIOCB());
}

void LinuxEv::forceClientevent(CClient * theClient, EventsID eventForce)
{
	ev_invoke(m_eventLoop, theClient->GetIOCB(), (int)eventForce);
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
