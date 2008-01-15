 
#ifndef _INC_LINUXEV_H
#define _INC_LINUXEV_H

	#include "../common/libev/ev.h"
	#include "threads.h"
	#include "mutex.h"
	
	class CGSocket;
	class CClient;
		
	class LinuxEv : public AbstractSphereThread
	{
	public:
		enum EventsID
		{
			Undefinied = -1,
			None = 0,
			Read = 1,
			Write = 2,
		
			Error = 0x80000000
		};
	
	private:
		struct ev_loop * m_eventLoop;
		struct ev_io m_watchMainsock;
		bool m_active;
	
	public:
		LinuxEv(void);
		~LinuxEv(void);
	
		virtual void onStart();
		virtual void tick();
		virtual void waitForClose();
		virtual bool shouldExit();
		
	private:
		void forceClientevent(CClient *, EventsID);
		
	public:
		void forceClientread(CClient *);
		void forceClientwrite(CClient *);
		// --------------------------------------	
		void registerClient(CClient *, EventsID);
		void unregisterClient(CClient *);
		// --------------------------------------
		void registerMainsocket();
		void unregisterMainsocket();
	};

#endif
