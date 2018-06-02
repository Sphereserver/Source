#ifdef _WIN32
#ifndef _INC_NTSERVICE_H
#define _INC_NTSERVICE_H

#ifdef __MINGW32__
#include <excpt.h>
#else //__MINGW32__
#include <eh.h> //	exception handling info.
#endif //__MINGW32__

extern class CNTService
{
private:
	bool m_fIsNTService;	// Are we running an NT service ?

	HANDLE m_hServerStopEvent;
	SERVICE_STATUS_HANDLE m_hStatusHandle;
	SERVICE_STATUS m_sStatus;

private:
	int  ServiceStart(DWORD, LPTSTR *);
	void ServiceStop();
	void ServiceStartMain(DWORD dwArgc, LPTSTR *lpszArgv);

	// Our exported API.
	static void WINAPI service_ctrl(DWORD dwCtrlCode);
	static void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);

public:
	static const char *m_sClassName;
	CNTService();

private:
	CNTService(const CNTService& copy);
	CNTService& operator=(const CNTService& other);

public:
	void ReportEvent( WORD wType, DWORD dwMsgID, LPCTSTR lpszMsg, LPCTSTR lpszArgs = NULL );
	BOOL SetServiceStatus(DWORD, DWORD, DWORD);

	// command line entries.
	void CmdInstallService();
	void CmdRemoveService();
	void CmdMainStart();

	bool OnTick();
	bool IsRunningAsService() const
	{
		return( m_fIsNTService );
	}
} g_Service;

#endif // _INC_NTSERVICE_H
#endif // _WIN32
