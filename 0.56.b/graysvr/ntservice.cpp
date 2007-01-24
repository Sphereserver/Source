// ntservice.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Stuff for making this application run as an NT service

#ifdef _WIN32
#include "graysvr.h"
#include "../common/grayver.h"
#include "ntservice.h"
#include <direct.h>

CNTService g_Service;

CNTService::CNTService()
{
	m_hStatusHandle = NULL;
	m_fIsNTService = false;
}

// Try to create the registry key containing the working directory for the application
static void ExtractPath(LPTSTR szPath)
{
	TCHAR	*pszPath = strrchr(szPath, '\\');
	if ( pszPath )
		*pszPath = 0;
}

static LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize)
{
	// Check CGrayError.
	//	PURPOSE:  copies error message text to a string
	//
	//	PARAMETERS:
	//		lpszBuf - destination buffer
	//		dwSize - size of buffer
	//
	//	RETURN VALUE:
	//		destination buffer

	int nChars = CGrayError::GetSystemErrorMessage( GetLastError(), lpszBuf, dwSize );
	sprintf( lpszBuf+nChars, " (ox%x)", GetLastError());
	return lpszBuf;
}

/////////////////////////////////////////////////////////////////////////////////////

//	PURPOSE:  Allows any thread to log a message to the NT Event Log
void CNTService::ReportEvent( WORD wType, DWORD dwEventID, LPCTSTR lpszMsg, LPCTSTR lpszArgs )
{
	g_Log.Event(LOGM_INIT|(( wType == EVENTLOG_INFORMATION_TYPE ) ? LOGL_EVENT : LOGL_ERROR), "%s %s\n", lpszMsg, lpszArgs);
}

// RETURN: false = exit app.
bool CNTService::OnTick()
{
	if (( !m_fIsNTService ) || ( m_sStatus.dwCurrentState != SERVICE_STOP_PENDING ) )
		return true;

	// Let the SCM know we aren't ignoring it
	SetServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);
	g_Serv.SetExitFlag(4);
	return false;
}

//	PURPOSE:  Sets the current status of the service and reports it to the Service Control Manager
BOOL CNTService::SetServiceStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint )
{
	if ( dwCurrentState == SERVICE_START_PENDING )
		m_sStatus.dwControlsAccepted = 0;
	else
		m_sStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	m_sStatus.dwCurrentState = dwCurrentState;
	m_sStatus.dwWin32ExitCode = dwWin32ExitCode;
	m_sStatus.dwWaitHint = dwWaitHint;

	static DWORD dwCheckPoint = 1;
	if (( dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		m_sStatus.dwCheckPoint = 0;
	else
		m_sStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the Service Control Manager
	BOOL fResult = ::SetServiceStatus(m_hStatusHandle, &m_sStatus);
	if ( !fResult && m_fIsNTService )
	{
		// if fResult is not 0, then an error occurred.  Throw this in the event log.
		char szErr[256];
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "SetServiceStatus", GetLastErrorText(szErr, sizeof(szErr)));
	}
	return fResult;
}

//	PURPOSE:  Handles console control events.  This application only handles Ctrl-C and Ctrl-Break events.
BOOL WINAPI CNTService::ControlHandler(DWORD dwCtrlType)	// static
{
	if (( dwCtrlType == CTRL_BREAK_EVENT ) || ( dwCtrlType == CTRL_C_EVENT ))
	{
		g_Service.ServiceStop();
		return TRUE;
	}
	g_Service.SetServiceStatus(g_Service.m_sStatus.dwCurrentState, NO_ERROR, 3000);
	return FALSE;
}

//	PURPOSE:  This function is called by the SCM whenever ControlService() is called on this service.  The
//		SCM does not start the service through this function.
void WINAPI CNTService::service_ctrl(DWORD dwCtrlCode) // static
{
	if ( dwCtrlCode == SERVICE_CONTROL_STOP )
		g_Service.ServiceStop();
	g_Service.SetServiceStatus(g_Service.m_sStatus.dwCurrentState, NO_ERROR, 0);
}

//	PURPOSE: is called by the SCM, and takes care of some initialization and calls ServiceStart().
void WINAPI CNTService::service_main(DWORD dwArgc, LPTSTR *lpszArgv) // static
{
	g_Service.ServiceStartMain(dwArgc, lpszArgv);
}

//	PURPOSE:  starts the service. (synchronous)
void CNTService::ServiceStartMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	TCHAR *pszMsg = Str_GetTemp();

	sprintf(pszMsg, GRAY_TITLE " V" GRAY_VERSION " - %s", g_Serv.GetName());

	m_hStatusHandle = RegisterServiceCtrlHandler(pszMsg, service_ctrl);
	if ( !m_hStatusHandle )	// Not much we can do about this.
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "RegisterServiceCtrlHandler failed");
		return;
	}

	// SERVICE_STATUS members that don't change
	m_sStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_sStatus.dwServiceSpecificExitCode = 0;

	// report the status to the service control manager.
	if ( SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		ServiceStart( dwArgc, lpszArgv);

	SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

//	PURPOSE:  starts the service. (synchronous)
int CNTService::ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv)
{
	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Service start pending.");

	// Service initialization report status to the service control manager.
	int rc = -1;
	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		return rc;

	// Create the event object.  The control handler function signals this event when it receives a "stop" control code
	m_hServerStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( !m_hServerStopEvent )
		return rc;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
	{
bailout1:
		CloseHandle(m_hServerStopEvent);
		return rc;
	}

	// Create the event object used in overlapped i/o
	HANDLE hEvents[2] = {NULL, NULL};
	hEvents[0] = m_hServerStopEvent;
	hEvents[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( !hEvents[1] )
		goto bailout1;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
	{
bailout2:
		CloseHandle(hEvents[1]);
		goto bailout1;
	}

	// Create a security descriptor that allows anyone to write to the pipe
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	if ( !pSD )
		goto bailout2;

	if ( !InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) )
	{
bailout3:
		free(pSD);
		goto bailout2;
	}

	// Add a NULL descriptor ACL to the security descriptor
	if ( !SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE) )
		goto bailout3;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	if ( !SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000) )
		goto bailout3;

	// Set the name of the named pipe this application uses.  We create a named pipe to ensure that
	// only one instance of this application runs on the machine at a time.  If there is an instance
	// running, it will own this pipe, and any further attempts to create the same named pipe will fail.
	char lpszPipeName[80];
	sprintf(lpszPipeName, "\\\\.\\pipe\\" GRAY_FILE "svr\\%s", g_Serv.GetName());

	char szErr[256];

	// Open the named pipe
	HANDLE hPipe = CreateNamedPipe(lpszPipeName,
		FILE_FLAG_OVERLAPPED|PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
		1, 0, 0, 1000, &sa);
	if ( hPipe == INVALID_HANDLE_VALUE )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Can't create named pipe. Check multi-instance?", GetLastErrorText(szErr, sizeof(szErr)));
		goto bailout3;
	}

	if ( SetServiceStatus(SERVICE_RUNNING, NO_ERROR, 0) )
	{
		rc = Sphere_MainEntryPoint(dwArgc, lpszArgv);

		if ( !rc )
			ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "service stopped", GetLastErrorText(szErr, sizeof(szErr)));
		else
		{
			char szMessage[80];
			sprintf(szMessage, "%ld.", rc);
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "service stopped abnormally", szMessage);
		}
	}
	else
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "ServiceStart failed.");

	CloseHandle(hPipe);
	goto bailout3;
}

//	PURPOSE:  reports that the service is attempting to stop
void CNTService::ServiceStop()
{
	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Attempting to stop service");

	m_sStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(m_sStatus.dwCurrentState, NO_ERROR, 3000);

	if (m_hServerStopEvent)
		SetEvent(m_hServerStopEvent);
}

//	PURPOSE:  Installs the service on the local machine
void CNTService::CmdInstallService()
{
	char	szPath[_MAX_PATH * 2];
	char	szErr[256];

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Installing Service.");

	// Try to determine the name and path of this application.
	if ( !GetModuleFileName(NULL, szPath, sizeof(szPath)) )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install GetModuleFileName", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to open the Service Control Manager
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if ( !schSCManager )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install OpenSCManager", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to create the service
	char szInternalName[MAX_PATH];
	sprintf(szInternalName, GRAY_TITLE " - %s", g_Serv.GetName());

	SC_HANDLE schService = CreateService(
		schSCManager,					// handle of the Service Control Manager
		szInternalName,				// Internal name of the service (used when controlling the service using "net start" or "netsvc")
		szInternalName,			// Display name of the service (displayed in the Control Panel | Services page)
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,				// Start automatically when the OS starts
		SERVICE_ERROR_NORMAL,
		szPath,							// Path and filename of this executable
		NULL, NULL, NULL, NULL, NULL
	);
	if ( !schService )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install CreateService", GetLastErrorText(szErr, sizeof(szErr)));
bailout1:
		CloseServiceHandle(schSCManager);
		return;
	}

	HKEY	hKey;
	char	szKey[MAX_PATH];

	// Register the application for event logging
	DWORD dwData;
	// Try to create the registry key containing information about this application
	strcpy(szKey, "System\\CurrentControlSet\\Services\\EventLog\\Application\\" GRAY_FILE "svr");

	if (RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey))
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegCreateKey", GetLastErrorText(szErr, sizeof(szErr)));
	else
	{
		// Try to create the registry key containing the name of the EventMessageFile
		//  Replace the name of the exe with the name of the dll in the szPath variable
		if (RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE) szPath, strlen(szPath) + 1))
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));
		else
		{
			// Try to create the registry key containing the types of errors this application will generate
			dwData = EVENTLOG_ERROR_TYPE|EVENTLOG_INFORMATION_TYPE|EVENTLOG_WARNING_TYPE;
			if ( RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD)) )
				ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));
		}
		RegCloseKey(hKey);
	}

	// Set the working path for the application
	sprintf(szKey, "System\\CurrentControlSet\\Services\\" GRAY_TITLE " - %s\\Parameters", g_Serv.GetName());
	if ( RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegCreateKey", GetLastErrorText(szErr, sizeof(szErr)));
bailout2:
		CloseServiceHandle(schService);
		goto bailout1;
	}
	ExtractPath(szPath);

	if ( RegSetValueEx(hKey, "WorkingPath", 0, REG_SZ, (const unsigned char *) &szPath[0], strlen(szPath)) )
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Install RegSetValueEx", GetLastErrorText(szErr, sizeof(szErr)));

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Install OK", g_Serv.GetName());
	goto bailout2;
}

//	PURPOSE:  Stops and removes the service
void CNTService::CmdRemoveService()
{
	char szErr[256];

	ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Removing Service.");

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if ( !schSCManager )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove OpenSCManager failed", GetLastErrorText(szErr, sizeof(szErr)));
		return;
	}

	// Try to obtain the handle of this service
	char szInternalName[MAX_PATH];
	sprintf(szInternalName, GRAY_TITLE " - %s", g_Serv.GetName());

	SC_HANDLE schService = OpenService(schSCManager, szInternalName, SERVICE_ALL_ACCESS);
	if ( !schService )
	{
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove OpenService failed", GetLastErrorText(szErr, sizeof(szErr)));

		CloseServiceHandle(schSCManager);
		return;
	}

	// Check to see if the service is started, if so try to stop it.
	if ( ControlService(schService, SERVICE_CONTROL_STOP, &m_sStatus) )
	{
		ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Stopping");

		Sleep(1000);
		while ( QueryServiceStatus(schService, &m_sStatus) )	// wait the service to stop
		{
			if ( m_sStatus.dwCurrentState == SERVICE_STOP_PENDING )
				Sleep(1000);
			else
				break;
		}

		if (m_sStatus.dwCurrentState == SERVICE_STOPPED)
			ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Stopped");
		else
			ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Failed to Stop");
	}

	// Remove the service
	if ( DeleteService(schService) )
		ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Removed OK");
	else
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "Remove Fail", GetLastErrorText(szErr, sizeof(szErr)));

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void CNTService::CmdMainStart()
{
	m_fIsNTService = true;

	char szTmp[256];
	sprintf(szTmp, GRAY_TITLE " - %s", g_Serv.GetName());
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ szTmp, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL },
	};

	if ( !StartServiceCtrlDispatcher(dispatchTable) )
		ReportEvent(EVENTLOG_ERROR_TYPE, 0, "StartServiceCtrlDispatcher", GetLastErrorText(szTmp, sizeof(szTmp)));
}

/////////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: main()
//
/////////////////////////////////////////////////////////////////////////////////////

extern void testThreads();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if( strstr(lpCmdLine, "testThreads") ) {
		testThreads();
		return 0;
	}

	TCHAR	*argv[32];
	argv[0] = NULL;
	int argc = Str_ParseCmds(lpCmdLine, &argv[1], COUNTOF(argv)-1, " \t") + 1;

	if ( GRAY_GetOSInfo()->dwPlatformId != VER_PLATFORM_WIN32_NT )
	{
		// We are running Win9x - So we are not an NT service.
do_not_nt_service:
		NTWindow_Init(hInstance, lpCmdLine, nCmdShow);
		int iRet = Sphere_MainEntryPoint(argc, argv);
		NTWindow_Exit();
		return iRet;
	}

	// We need to find out what the server name is....look it up in the .ini file
	if ( !g_Cfg.LoadIni(true) )
	{
		// Try to determine the name and path of this application.
		char szPath[_MAX_PATH];

		GetModuleFileName(NULL, szPath, sizeof(szPath));
		
		if ( !szPath[0] )
			return -2;

		ExtractPath(szPath);
		_chdir(szPath);

		if ( !g_Cfg.LoadIni(false) )
			return -2;
	}

	if ( !g_Cfg.m_fUseNTService )	// since there is no way to detect how did we start, use config for that
		goto do_not_nt_service;

	g_Service.SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 5000);

	// process the command line arguments...
	if (( argc > 1 ) && _IS_SWITCH(*argv[1]) )
	{
		if ( argv[1][1] == 'k' )		// service control
		{
			if ( argc < 2 )
				printf("Use \"-k command\" with operation to proceed (install/remove)\n");
			else if ( !strcmp(argv[2], "install") )
			{
				g_Service.CmdInstallService();
			}
			else if ( !strcmp(argv[2], "remove") )
			{
				g_Service.CmdRemoveService();
			}
			return 0;
		}
	}

	// If the argument does not match any of the above parameters, the Service Control Manager (SCM) may
	// be attempting to start the service, so we must call StartServiceCtrlDispatcher.
	g_Service.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, "Starting Service.");

	g_Service.CmdMainStart();
	g_Service.SetServiceStatus(SERVICE_STOPPED, -1, 0);
	return -1;
}

#endif

