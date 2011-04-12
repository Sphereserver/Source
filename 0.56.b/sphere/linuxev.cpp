#if !defined(_WIN32) || defined(_LIBEV)
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
#ifndef _MTNETWORK
			g_NetworkIn.acceptConnection();
#else
			g_NetworkManager.acceptNewConnection();
#endif
		}
	}
	
	ev_io_start(loop, w);
}

static void socketslave_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	NetState* state = reinterpret_cast<NetState *>( w->data );
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			// g_NetworkOut.onAsyncSendComplete(state);
		}		
		else if ( revents & EV_WRITE )
		{
#ifndef _MTNETWORK
			g_NetworkOut.onAsyncSendComplete(state, true);
#else
			NetworkThread* thread = state->getParentThread();
			if (thread != NULL)
				thread->onAsyncSendComplete(state, true);
#endif
		}
	}
	
	if ( state->isSendingAsync() )
	{
		ev_io_start(loop, w);
	}
}

LinuxEv::LinuxEv(void) : AbstractSphereThread("NetworkEvents", IThread::High)
{
	m_eventLoop = ev_loop_new(EV_BACKEND_LIST);
	ev_set_io_collect_interval(m_eventLoop, 0.01);
	
	memset(&m_watchMainsock, 0, sizeof(ev_io));
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
	ev_run(m_eventLoop, EVRUN_NOWAIT);
}

void LinuxEv::waitForClose()
{
	ev_break(m_eventLoop, EVBREAK_ALL);

	AbstractSphereThread::waitForClose();
}

void LinuxEv::registerClient(NetState * state, EventsID eventCheck)
{
	ADDTOCALLSTACK("LinuxEv::registerClient");
	ASSERT(state != NULL);
	
	memset(state->iocb(), 0, sizeof(ev_io));

#ifdef _WIN32
	int fd = EV_WIN32_HANDLE_TO_FD(state->m_socket.GetSocket());
	ev_io_init(state->iocb(), socketslave_cb, fd, (int)eventCheck);
#else
	ev_io_init(state->iocb(), socketslave_cb, state->m_socket.GetSocket(), (int)eventCheck);
#endif
	state->iocb()->data = state;
	state->setSendingAsync(true);
	
    ev_io_start(m_eventLoop, state->iocb());	
}

void LinuxEv::unregisterClient(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::unregisterClient");
	ASSERT(state != NULL);

	state->setSendingAsync(false);
	
	ev_io_stop(m_eventLoop, state->iocb());
}

void LinuxEv::forceClientevent(NetState * state, EventsID eventForce)
{
	ADDTOCALLSTACK("LinuxEv::forceClientevent");
	ASSERT(state != NULL);
	ev_invoke(m_eventLoop, state->iocb(), (int)eventForce);
}

void LinuxEv::forceClientread(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientread");
	forceClientevent(state, LinuxEv::Read);
}

void LinuxEv::forceClientwrite(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientwrite");
	forceClientevent(state, LinuxEv::Write);
}

void LinuxEv::registerMainsocket()
{
#ifdef _WIN32
	int fd = EV_WIN32_HANDLE_TO_FD(g_Serv.m_SocketMain.GetSocket());
	ev_io_init(&m_watchMainsock, socketmain_cb, fd, EV_READ);
#else
	ev_io_init(&m_watchMainsock, socketmain_cb, g_Serv.m_SocketMain.GetSocket(), EV_READ);
#endif
    ev_io_start(m_eventLoop, &m_watchMainsock);		
}

void LinuxEv::unregisterMainsocket()
{
	ev_io_stop(m_eventLoop, &m_watchMainsock);
}

#endif
