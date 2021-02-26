// Put up a window for data (other than the console)
#ifdef _WIN32

#include "graysvr.h"	// predef header
#include "resource.h"
#include "CNTWindow.h"

#define WM_USER_POST_MSG		(WM_USER+10)
#define WM_USER_TRAY_NOTIFY		(WM_USER+12)
#define IDC_M_LOG	10
#define IDC_M_INPUT 11
#define IDT_ONTICK	1

CNTApp theApp;

//************************************
// -CAboutDlg

bool CNTWindow::CAboutDlg::OnInitDialog()
{
	char *pszBuild = Str_GetTemp();
#if defined(__GITREVISION__) && defined(__GITHASH__)
	snprintf(pszBuild, 80, "Compiled at %s (Build %d / Git hash %s)", g_szCompiledDate, __GITREVISION__, __GITHASH__);
#else
	snprintf(pszBuild, 40, "Compiled at %s", g_szCompiledDate);
#endif

	SetDlgItemText(IDC_ABOUT_VERSION, SPHERE_TITLE_VER " (" SPHERE_VER_ARCH ")");
	SetDlgItemText(IDC_ABOUT_WEBSITE, SPHERE_WEBSITE);
	SetDlgItemText(IDC_ABOUT_COMPILER, pszBuild);
	return false;
}

bool CNTWindow::CAboutDlg::OnCommand( WORD wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	// WM_COMMAND
	switch ( wID )
	{
		case IDOK:
		case IDCANCEL:
			EndDialog( m_hWnd, wID );
			break;
	}
	return( TRUE );
}

BOOL CNTWindow::CAboutDlg::DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_INITDIALOG:
		return( OnInitDialog());
	case WM_COMMAND:
		return( OnCommand(  HIWORD(wParam), LOWORD(wParam), (HWND) lParam ));
	case WM_DESTROY:
		OnDestroy();
		return( TRUE );
	}
	return( FALSE );
}

//************************************
// -CStatusDlg

void CNTWindow::CStatusDlg::FillClients()
{
	if ( m_wndListClients.m_hWnd == NULL )
		return;
	m_wndListClients.ResetContent();
	CNTWindow::CListTextConsole capture( m_wndListClients.m_hWnd );
	g_Serv.ListClients( &capture );
	int iCount = m_wndListClients.GetCount();
	iCount++;
}

void CNTWindow::CStatusDlg::FillStats()
{
	if ( m_wndListStats.m_hWnd == NULL )
		return;

	m_wndListStats.ResetContent();

	CNTWindow::CListTextConsole capture( m_wndListStats.m_hWnd );

	size_t iThreadCount = ThreadHolder::getActiveThreads();
	for ( size_t iThreads = 0; iThreads < iThreadCount; ++iThreads)
	{
		IThread *thrCurrent = ThreadHolder::getThreadAt(iThreads);
		if (thrCurrent == NULL)
			continue;

		const ProfileData &profile = static_cast<AbstractSphereThread *>(thrCurrent)->m_profile;
		if (profile.IsEnabled() == false)
			continue;

		capture.SysMessagef("Thread %u - '%s'\n", thrCurrent->getId(), thrCurrent->getName());

		for (int i = 0; i < PROFILE_QTY; i++)
		{
			if (profile.IsEnabled(static_cast<PROFILE_TYPE>(i)) == false)
				continue;

			capture.SysMessagef("'%-10s' = %s\n", profile.GetName(static_cast<PROFILE_TYPE>(i)), profile.GetDescription(static_cast<PROFILE_TYPE>(i)));
		}
	}
}

bool CNTWindow::CStatusDlg::OnInitDialog()
{
	m_wndListClients.m_hWnd = GetDlgItem(IDC_STAT_CLIENTS);
	FillClients();
	m_wndListStats.m_hWnd = GetDlgItem(IDC_STAT_STATS);
	FillStats();
	return( false );
}

bool CNTWindow::CStatusDlg::OnCommand( WORD wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	// WM_COMMAND
	switch ( wID )
	{
		case IDOK:
		case IDCANCEL:
			DestroyWindow();
			break;
	}
	return( FALSE );
}

BOOL CNTWindow::CStatusDlg::DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	// IDM_STATUS
	switch ( message )
	{
	case WM_INITDIALOG:
		return( OnInitDialog());
	case WM_COMMAND:
		return( OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND) lParam ));
	case WM_DESTROY:
		m_wndListClients.OnDestroy();
		m_wndListStats.OnDestroy();
		OnDestroy();
		return( TRUE );
	}
	return( FALSE );
}

CNTWindow::CNTWindow()
{
	m_iLogTextLen		= 0;
	m_fLogScrollLock	= false;
	m_dwColorNew		= RGB( 0xaf,0xaf,0xaf );
	m_dwColorPrv		= RGB( 0xaf,0xaf,0xaf );
	m_iHeightInput		= 0;
   	m_hLogFont			= NULL;
	m_wndLog.SetSel(0, 0);
	memset(m_zCommands, 0, sizeof(m_zCommands));
}

CNTWindow::~CNTWindow()
{
	NTWindow_DeleteIcon();
	DestroyWindow();
}

void CNTWindow::List_Clear()
{
	m_wndLog.SetWindowText( "");
	m_wndLog.SetSel( 0, 0 );
	m_iLogTextLen = 0;
}

void CNTWindow::List_Add( COLORREF color, LPCTSTR pszText )
{
	int iTextLen = strlen( pszText );
	int iNewLen = m_iLogTextLen + iTextLen;

	if ( iNewLen > (32*1024) )
	{
		int iCut = iNewLen - (32*1024);
		m_wndLog.SetSel( 0, iCut );
		m_wndLog.ReplaceSel( "" );
		m_iLogTextLen = (32*1024);
	}

	m_wndLog.SetSel( m_iLogTextLen, m_iLogTextLen );

	// set the blocks color.
	CHARFORMAT cf;
	memset( &cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;
	m_wndLog.SetSelectionCharFormat( cf );

	m_wndLog.ReplaceSel( pszText );

	m_iLogTextLen += iTextLen;
	m_wndLog.SetSel( m_iLogTextLen, m_iLogTextLen );

	int iSelBegin;
	int iSelEnd;
	m_wndLog.GetSel( iSelBegin, iSelEnd );
	m_iLogTextLen = iSelBegin;	// make sure it's correct.

	// If the select is on screen then keep scrolling.
	if ( ! m_fLogScrollLock && ! GetCapture())
	{
		if ( GRAY_GetOSInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			m_wndLog.Scroll();
		}
	}
}

bool CNTWindow::RegisterClass(char *className)	// static
{
	WNDCLASS wc;
	memset( &wc, 0, sizeof(wc));

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = theApp.m_hInstance;
	wc.hIcon = theApp.LoadIcon( IDR_MAINFRAME );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;

	ATOM frc = ::RegisterClass( &wc );
	if ( !frc )
	{
		return( false );
	}

	TCHAR szLibPath[MAX_PATH];
	GetSystemDirectory(szLibPath, sizeof(szLibPath));
	strncat(szLibPath, "\\riched20.dll", sizeof(szLibPath) - strlen(szLibPath) - 1);

	LoadLibrary(szLibPath);
	return true;
}

int CNTWindow::OnCreate( HWND hWnd, LPCREATESTRUCT lParam )
{
	UNREFERENCED_PARAMETER(lParam);
	CWindow::OnCreate(hWnd);

	m_wndLog.m_hWnd = ::CreateWindow( RICHEDIT_CLASS, NULL,
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY | /* ES_OEMCONVERT | */
		WS_CHILD|WS_VISIBLE|WS_VSCROLL,
		0, 0, 10, 10,
		m_hWnd,
		(HMENU)(UINT) IDC_M_LOG, theApp.m_hInstance, NULL );
	ASSERT( m_wndLog.m_hWnd );


	SetLogFont( "Courier" );

	// TEXTMODE
	m_wndLog.SetBackgroundColor( false, RGB(0,0,0));
	CHARFORMAT cf;
	memset( &cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = m_dwColorPrv;
	cf.bCharSet = ANSI_CHARSET;
	cf.bPitchAndFamily = FF_MODERN | FIXED_PITCH;
	m_wndLog.SetDefaultCharFormat( cf );
	m_wndLog.SetEventMask( ENM_LINK | ENM_MOUSEEVENTS | ENM_KEYEVENTS );

	m_wndInput.m_hWnd = ::CreateWindow("EDIT", NULL,
		ES_LEFT | ES_AUTOHSCROLL | WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP,
		0, 0, 10, 10,
		m_hWnd,
		(HMENU)(UINT) IDC_M_INPUT, theApp.m_hInstance, NULL );
	ASSERT( m_wndInput.m_hWnd );

	if ( GRAY_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		memset(&pnid,0,sizeof(pnid));
		pnid.cbSize = sizeof(NOTIFYICONDATA);
		pnid.hWnd   = m_hWnd;
		pnid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
		pnid.uCallbackMessage = WM_USER_TRAY_NOTIFY;
		pnid.hIcon  = theApp.LoadIcon( IDR_MAINFRAME );
		strcpylen(pnid.szTip, theApp.m_pszAppName, COUNTOF(pnid.szTip)-1);
		Shell_NotifyIcon(NIM_ADD, &pnid);
	}

	NTWindow_SetWindowTitle();

	return( 0 );
}

void CNTWindow::OnDestroy()
{
	m_wndLog.OnDestroy();	// these are automatic.
	m_wndInput.OnDestroy();
	CWindow::OnDestroy();
}

void CNTWindow::OnSetFocus( HWND hWndLoss )
{
	UNREFERENCED_PARAMETER(hWndLoss);
	m_wndInput.SetFocus();
}

LRESULT CNTWindow::OnUserTrayNotify( WPARAM wID, LPARAM lEvent )
{
	UNREFERENCED_PARAMETER(wID);

	// WM_USER_TRAY_NOTIFY
	switch ( lEvent )
	{
	case WM_RBUTTONDOWN:
		// Context menu ?
		{
			HMENU hMenu = theApp.LoadMenu( IDM_POP_TRAY );
			if ( hMenu == NULL )
				break;
			HMENU hMenuPop = GetSubMenu(hMenu,0);
			if ( hMenuPop )
			{
				POINT point;
				if ( GetCursorPos( &point ))
				{
					TrackPopupMenu( hMenuPop, TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL );
				}
			}
			DestroyMenu( hMenu );
		}
		return 1;
	case WM_LBUTTONDBLCLK:
		{
			if ( IsWindowVisible(m_hWnd) )
				ShowWindow(SW_HIDE);
			else
			{
				ShowWindow(SW_NORMAL);
				SetForegroundWindow();
			}
			return 1;
		}
	}
	return 0;	// not handled.
}

void CNTWindow::OnUserPostMessage( COLORREF color, CGString * psMsg )
{
	// WM_USER_POST_MSG
	if ( psMsg )
	{
		List_Add(color, *psMsg);
		delete psMsg;
	}
}

void CNTWindow::OnSize( WPARAM nType, int cx, int cy )
{
	if ( nType != SIZE_MINIMIZED && nType != SIZE_MAXHIDE && m_wndLog.m_hWnd )
	{
		if ( ! m_iHeightInput )
		{
			HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
			if ( !hFont )
				hFont = (HFONT)GetStockObject(SYSTEM_FONT);
			ASSERT(hFont);

			LOGFONT logfont;
			int iRet = ::GetObject(hFont, sizeof(logfont),&logfont );
			ASSERT(iRet==sizeof(logfont));
			UNREFERENCED_PARAMETER(iRet);

			m_iHeightInput = abs( logfont.lfHeight );
			ASSERT(m_iHeightInput);
		}

		m_wndLog.MoveWindow( 0, 0, cx, cy-m_iHeightInput, TRUE );
		m_wndInput.MoveWindow( 0, cy-m_iHeightInput, cx, m_iHeightInput, TRUE );
	}
}

bool CNTWindow::OnClose()
{
	// WM_CLOSE
	if ( g_Serv.m_iExitFlag == 0 )
	{
		int iRet = theApp.m_wndMain.MessageBox("Are you sure you want to close the server?",
			theApp.m_pszAppName, MB_YESNO|MB_ICONQUESTION );
		if ( iRet == IDNO )
			return( false );
	}

	PostQuitMessage(0);
	g_Serv.SetExitFlag( 5 );
	return( true );	// ok to close.
}

bool CNTWindow::OnCommand( WORD wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	// WM_COMMAND
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	switch ( wID )
	{
	case IDC_M_LOG:
		break;
	case IDM_STATUS:
		if ( theApp.m_wndStatus.m_hWnd == NULL )
		{
			theApp.m_wndStatus.m_hWnd = ::CreateDialogParam(
				theApp.m_hInstance,
				MAKEINTRESOURCE(IDM_STATUS),
				HWND_DESKTOP,
				CDialogBase::DialogProc,
				reinterpret_cast<LPARAM>(static_cast <CDialogBase*>(&theApp.m_wndStatus)) );
		}
		theApp.m_wndStatus.SetIcon(theApp.LoadIcon(IDR_MAINFRAME));
		theApp.m_wndStatus.ShowWindow(SW_NORMAL);
		theApp.m_wndStatus.SetForegroundWindow();
		break;
	case IDR_ABOUT_BOX:
		if ( theApp.m_wndAbout.m_hWnd == NULL )
		{
			theApp.m_wndAbout.m_hWnd = ::CreateDialogParam(
				theApp.m_hInstance,
				MAKEINTRESOURCE(IDR_ABOUT_BOX),
				HWND_DESKTOP,
				CDialogBase::DialogProc,
				reinterpret_cast<LPARAM>(static_cast <CDialogBase*>(&theApp.m_wndAbout)) );
		}
		theApp.m_wndAbout.SetIcon(theApp.LoadIcon(IDR_MAINFRAME));
		theApp.m_wndAbout.ShowWindow(SW_NORMAL);
		theApp.m_wndAbout.SetForegroundWindow();
		break;

	case IDM_MINIMIZE:
		// SC_MINIMIZE
	    ShowWindow(SW_HIDE);
		break;
	case IDM_RESTORE:
		// SC_RESTORE
	    ShowWindow(SW_NORMAL);
		SetForegroundWindow();
		break;
	case IDM_EXIT:
		PostMessage( WM_CLOSE );
		break;

	case IDM_RESYNC_PAUSE:
		if ( ! g_Serv.m_fConsoleTextReadyFlag )	// busy ?
		{
			g_Serv.m_sConsoleText = "R";
			g_Serv.m_fConsoleTextReadyFlag = true;
			return( true );
		}
		return( false );

	case IDM_EDIT_COPY:
		m_wndLog.SendMessage( WM_COPY );
		break;

	case IDOK:
		// We just entered the text.

		if ( ! g_Serv.m_fConsoleTextReadyFlag )	// busy ?
		{
			TCHAR szTmp[ MAX_TALK_BUFFER ];
			m_wndInput.GetWindowText( szTmp, sizeof(szTmp));

			for ( int i = 4; i > 0; --i )
			{
				strncpy(theApp.m_wndMain.m_zCommands[i], theApp.m_wndMain.m_zCommands[i - 1], sizeof(theApp.m_wndMain.m_zCommands[i]) - 1);
				theApp.m_wndMain.m_zCommands[i][sizeof(theApp.m_wndMain.m_zCommands[i]) - 1] = '\0';
			}

			strncpy(m_zCommands[0], szTmp, sizeof(m_zCommands[0]) - 1);
			m_zCommands[0][sizeof(m_zCommands[0]) - 1] = '\0';
			m_wndInput.SetWindowText("");
			g_Serv.m_sConsoleText = szTmp;
			g_Serv.m_fConsoleTextReadyFlag = true;
			return( true );
		}
		return( false );
	}
	return( true );
}

bool CNTWindow::OnSysCommand( WPARAM uCmdType, int xPos, int yPos )
{
	// WM_SYSCOMMAND
	// return : 1 = i processed this.
	UNREFERENCED_PARAMETER(xPos);
	UNREFERENCED_PARAMETER(yPos);

	switch ( uCmdType )
	{
		case SC_MINIMIZE:
			if ( GRAY_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
			{
				ShowWindow(SW_HIDE);
				return( true );
			}
			break;
	}
	return( false );
}

void	CNTWindow::SetLogFont( const char * pszFont )
{
	// use an even spaced font
	if ( pszFont == NULL )
	{
		m_hLogFont	= (HFONT) GetStockObject(SYSTEM_FONT);
	}
	else
	{
		LOGFONT logfont;
   		memset( &logfont, 0, sizeof(logfont) );
		strncpy(logfont.lfFaceName, pszFont, sizeof(logfont.lfFaceName) - 1);
		logfont.lfFaceName[sizeof(logfont.lfFaceName) - 1] = '\0';

		// calculate height for a 10pt font, some systems can produce an unreadable
		// font size if we let CreateFontIndirect pick a system default size
		HDC hdc = GetDC(NULL);
		if (hdc != NULL)
		{
			//logfont.lfHeight = MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			logfont.lfHeight = IMULDIV(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			ReleaseDC(NULL, hdc);
		}

		logfont.lfPitchAndFamily = FF_MODERN;
   		m_hLogFont = CreateFontIndirect( &logfont );
	}
   	m_wndLog.SetFont( m_hLogFont, true );
}


LRESULT CNTWindow::OnNotify( int idCtrl, NMHDR * pnmh )
{
	ASSERT(pnmh);
	if ( idCtrl != IDC_M_LOG )
		return 0;

	switch ( pnmh->code )
	{
	case EN_LINK:
		{
			ENLINK * pLink = (ENLINK *)(pnmh);
			if ( pLink->msg == WM_LBUTTONDOWN )
				return 1;
			break;
		}
	case EN_MSGFILTER:
		{
			MSGFILTER	*pMsg = (MSGFILTER *)pnmh;
			ASSERT(pMsg);

			switch ( pMsg->msg )
			{
			case WM_MOUSEMOVE:
				return 0;
			case WM_RBUTTONDOWN:
				{
					HMENU hMenu = theApp.LoadMenu( IDM_POP_LOG );
					if ( !hMenu )
						return 0;
					HMENU hMenuPop = GetSubMenu(hMenu,0);
					if ( hMenuPop )
					{
						POINT point;
						if ( GetCursorPos( &point ))
							TrackPopupMenu( hMenuPop, TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL );
					}
					DestroyMenu(hMenu);
					return 1;
				}
			case WM_LBUTTONDBLCLK:
				{
					TCHAR * zTemp = Str_GetTemp();
					POINT pt;
					pt.x = LOWORD(pMsg->lParam);
					pt.y = HIWORD(pMsg->lParam);

					// get selected line
					LRESULT line = m_wndLog.SendMessage(EM_LINEFROMCHAR, m_wndLog.SendMessage(EM_CHARFROMPOS, 0, reinterpret_cast<LPARAM>(&pt)), 0);

					// get the line text
					reinterpret_cast<WORD*>(zTemp)[0] = SCRIPT_MAX_LINE_LEN - 1; // first WORD is used to indicate the max buffer length
					zTemp[m_wndLog.SendMessage(EM_GETLINE, line, reinterpret_cast<LPARAM>(zTemp))] = '\0';
					if ( *zTemp == '\0' )
						break;

					//	use dclick to open the corresponding script file
					TCHAR * pos = strstr(zTemp, SPHERE_SCRIPT);
					if ( pos != NULL )
					{
						//	use two formats of file names:
						//		Loading filepath/filename/name.scp
						//		ERROR:(filename.scp,line)
						LPCTSTR start = pos;
						TCHAR * end = pos + 4;

						while ( start > zTemp )
						{
							if (( *start == ' ' ) || ( *start == '(' ))
								break;
							start--;
						}
						start++;
						*end = '\0';

						if ( *start != '\0' )
						{
							LPCTSTR filePath = NULL;

							// search script files for a matching name
							size_t i = 0;
							for (const CResourceScript * s = g_Cfg.GetResourceFile(i++); s != NULL; s = g_Cfg.GetResourceFile(i++))
							{
								if ( strstr(s->GetFilePath(), start) == NULL )
									continue;

								filePath = s->GetFilePath();
								break;
							}

							// since certain files aren't listed, handle these separately
							if (filePath == NULL)
							{
								if ( strstr(SPHERE_FILE "tables" SPHERE_SCRIPT, start) )
								{
									TCHAR szBaseDir[MAX_PATH];
									snprintf(szBaseDir, sizeof(szBaseDir), "%s%s", static_cast<LPCTSTR>(g_Cfg.m_sSCPBaseDir), start);
									filePath = szBaseDir;
								}
							}

							if (filePath != NULL)
							{
								TCHAR szApplicationName[MAX_PATH];
								GetSystemDirectory(szApplicationName, sizeof(szApplicationName));
								strncat(szApplicationName, "\\notepad.exe", sizeof(szApplicationName) - strlen(szApplicationName) - 1);

								TCHAR szCommandLine[MAX_PATH];
								snprintf(szCommandLine, sizeof(szCommandLine), " \"%s\"", filePath);

								STARTUPINFO si;
								ZeroMemory(&si, sizeof(si));
								si.cb = sizeof(si);

								PROCESS_INFORMATION pi;
								ZeroMemory(&pi, sizeof(pi));

								if ( CreateProcess(szApplicationName, szCommandLine, NULL, NULL, true, 0, NULL, NULL, &si, &pi) )
								{
									CloseHandle(pi.hProcess);
									CloseHandle(pi.hThread);
									return 1;
								}

								DWORD dwErrorCode = CGFile::GetLastError();
								LPTSTR pszErrorMsg = Str_GetTemp();
								if (CGrayError::GetSystemErrorMessage(dwErrorCode, pszErrorMsg, THREAD_STRING_LENGTH) > 0)
									g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%hu (%s)\n", filePath, dwErrorCode, pszErrorMsg);
								else
									g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%hu\n", filePath, dwErrorCode);
							}
						}
					}
					break;
				}
			case WM_CHAR:
				{
					// We normally have no business typing into this window.
					// Should we allow CTL C etc ?
					if ( pMsg->lParam & (1<<29))	// ALT
						return 0;
					SHORT sState = GetKeyState( VK_CONTROL );
					if ( sState & 0xff00 )
						return 0;
					m_wndInput.SetFocus();
					m_wndInput.PostMessage( WM_CHAR, pMsg->wParam, pMsg->lParam );
					return 1;	// mostly ignored.
				}
			} // pMsg
		} // MSGFILTER
	} // code
	return 0;
}

LRESULT WINAPI CNTWindow::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )	// static
{
	try
	{
		switch( message )
		{
		case WM_CREATE:
			return( theApp.m_wndMain.OnCreate( hWnd, (LPCREATESTRUCT) lParam ));
		case WM_SYSCOMMAND:
			if ( theApp.m_wndMain.OnSysCommand( wParam &~ 0x0f, LOWORD(lParam), HIWORD(lParam)))
				return( 0 );
			break;
		case WM_COMMAND:
			if ( theApp.m_wndMain.OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND) lParam ))
				return( 0 );
			break;
		case WM_CLOSE:
			if ( ! theApp.m_wndMain.OnClose())
				return( false );
			break;
		case WM_ERASEBKGND:	// don't bother with this.
			return 1;
		case WM_SIZE:	// get the client rectangle
			theApp.m_wndMain.OnSize( wParam, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			theApp.m_wndMain.OnDestroy();
			return 0;
		case WM_SETFOCUS:
			theApp.m_wndMain.OnSetFocus( (HWND) wParam );
			return 0;
		case WM_NOTIFY:
			theApp.m_wndMain.OnNotify( (int) wParam, (NMHDR *) lParam );
			return 0;
		case WM_USER_POST_MSG:
			theApp.m_wndMain.OnUserPostMessage( (COLORREF) wParam, reinterpret_cast<CGString*>(lParam) );
			return 1;
		case WM_USER_TRAY_NOTIFY:
			return theApp.m_wndMain.OnUserTrayNotify( wParam, lParam );
		}
	}
	catch (const CGrayError& e)
	{
		g_Log.CatchEvent(&e, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(NULL, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

//************************************

bool NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	theApp.InitInstance(SPHERE_TITLE "Server V" SPHERE_VER_STR_FULL, hInstance, lpCmdLine);

	//	read target window name from the arguments
	char	className[32] = SPHERE_TITLE "Svr";
	TCHAR	*argv[32];
	argv[0] = NULL;
	size_t argc = Str_ParseCmds(lpCmdLine, &argv[1], COUNTOF(argv)-1, " \t") + 1;
	if (( argc > 1 ) && _IS_SWITCH(*argv[1]) )
	{
		if ( argv[1][1] == 'c' )
		{
			if ( argv[1][2] )
			{
				strncpy(className, &argv[1][2], sizeof(className) - 1);
				className[sizeof(className) - 1] = '\0';
			}
		}
	}
	CNTWindow::RegisterClass(className);

	theApp.m_wndMain.m_hWnd = ::CreateWindow(
		className,
		SPHERE_TITLE_VER, // window name
		WS_OVERLAPPEDWINDOW,   // window style
		CW_USEDEFAULT,  // horizontal position of window
		CW_USEDEFAULT,  // vertical position of window
		CW_USEDEFAULT,  // window width
		CW_USEDEFAULT,	// window height
		HWND_DESKTOP,      // handle to parent or owner window
		NULL,          // menu handle or child identifier
		theApp.m_hInstance,  // handle to application instance
		NULL        // window-creation data
		);

	theApp.m_wndMain.ShowWindow(nCmdShow);
	return true;
}

void NTWindow_DeleteIcon()
{
	if ( GRAY_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		theApp.m_wndMain.pnid.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &theApp.m_wndMain.pnid);
	}
}

void NTWindow_Exit()
{
	// Unattach the window.
	if ( g_Serv.m_iExitFlag < 0 )
	{
		TCHAR *pszMsg = Str_GetTemp();
		snprintf(pszMsg, 32, "Server terminated by error %d!", g_Serv.m_iExitFlag);
		theApp.m_wndMain.MessageBox(pszMsg, theApp.m_pszAppName, MB_OK|MB_ICONEXCLAMATION );
		// just sit here for a bit til the user wants to close the window.
		while ( NTWindow_OnTick(500) )
		{
		}
	}
}

void NTWindow_SetWindowTitle( LPCTSTR pszText )
{
	if ( theApp.m_wndMain.m_hWnd == NULL )
		return;
	// set the title to reflect mode.

	LPCTSTR pszMode;
	switch ( g_Serv.m_iModeCode )
	{
	case SERVMODE_RestockAll:	// Major event.
		pszMode = "Restocking";
		break;
	case SERVMODE_Saving:		// Forced save freezes the system.
		pszMode = "Saving";
		break;
	case SERVMODE_Run:			// Game is up and running
		pszMode = "Running";
		break;
	case SERVMODE_Loading:		// Initial load.
		pszMode = "Loading";
		break;
	case SERVMODE_ResyncPause:
		pszMode = "Resync Pause";
		break;
	case SERVMODE_ResyncLoad:	// Loading after resync
		pszMode = "Resync Load";
		break;
	case SERVMODE_Exiting:		// Closing down
		pszMode = "Exiting";
		break;
	default:
		pszMode = "Unknown";
		break;
	}

	char szTitle[MAX_PATH];
	snprintf(szTitle, sizeof(szTitle), "%s - %s (%s) %s", theApp.m_pszAppName, g_Serv.GetName(), pszMode, pszText ? pszText : "");
	theApp.m_wndMain.SetWindowText(szTitle);

	if ( GRAY_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		theApp.m_wndMain.pnid.uFlags = NIF_TIP;
		strcpylen(theApp.m_wndMain.pnid.szTip, szTitle, COUNTOF(theApp.m_wndMain.pnid.szTip) - 1);
		Shell_NotifyIcon(NIM_MODIFY, &theApp.m_wndMain.pnid);
	}
}

bool NTWindow_PostMsgColor( COLORREF color )
{
	// Set the color for the next text.
	if ( theApp.m_wndMain.m_hWnd == NULL )
		return( false );

	if ( ! color )
	{
		// set to default color.
		color = theApp.m_wndMain.m_dwColorPrv;
	}

	theApp.m_wndMain.m_dwColorNew = color;
	return( true );
}

bool NTWindow_PostMsg( LPCTSTR pszMsg )
{
	// Post a message to print out on the main display.
	// If we use AttachThreadInput we don't need to post ?
	// RETURN:
	//  false = post did not work.

	if ( theApp.m_wndMain.m_hWnd == NULL )
		return( false );

	COLORREF color = theApp.m_wndMain.m_dwColorNew;

//	if ( g_Serv.m_dwParentThread != CThread::GetCurrentThreadId())
//	{
//		// A thread safe way to do text.
//		CGString * psMsg = new CGString( pszMsg );
//		ASSERT(psMsg);
//		if ( ! theApp.m_wndMain.PostMessage( WM_USER_POST_MSG, (WPARAM) color, (LPARAM)psMsg ))
//		{
//			delete psMsg;
//			return( false );
//		}
//	}
//	else
//	{
		// just add it now.
		theApp.m_wndMain.List_Add( color, pszMsg );
//	}

	return( true );
}

bool NTWindow_OnTick( int iWaitmSec )
{
	// RETURN: false = exit the app.

#ifdef EXCEPTIONS_DEBUG
	const char *m_sClassName = "NTWindow";
#endif
	if ( iWaitmSec )
	{
		if ( !theApp.m_wndMain.m_hWnd || !theApp.m_wndMain.SetTimer(IDT_ONTICK, iWaitmSec) )
		{
			iWaitmSec = 0;
		}
	}

	// Give the windows message loops a tick.
	for (;;)
	{
		EXC_TRY("Tick");

		MSG msg;

		// any windows messages ? (blocks until a message arrives)
		if ( iWaitmSec )
		{
			if ( ! GetMessage( &msg, NULL, 0, 0 ))
			{
				g_Serv.SetExitFlag( 5 );
				return( false );
			}

			if ( (msg.hwnd == theApp.m_wndMain.m_hWnd) && (msg.message == WM_TIMER) && (msg.wParam == IDT_ONTICK) )
			{
				theApp.m_wndMain.KillTimer( IDT_ONTICK );
				iWaitmSec = 0;	// empty the queue and bail out.
				continue;
			}
		}
		else
		{
			if (! PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
			{
				return true;
			}

			if ( msg.message == WM_QUIT )
			{
				g_Serv.SetExitFlag( 5 );
				return( false );
			}
		}

		//	Got char in edit box
		if ( theApp.m_wndMain.m_wndInput.m_hWnd && (msg.hwnd == theApp.m_wndMain.m_wndInput.m_hWnd) )
		{
			if ( msg.message == WM_CHAR )	//	char to edit box
			{
				if ( msg.wParam == VK_RETURN )	//	ENTER
				{
					if ( theApp.m_wndMain.OnCommand( 0, IDOK, msg.hwnd ))
					{
						return(true);
					}
				}
			}
			else if ( msg.message == WM_KEYUP )	//	key released
			{
				if ( msg.wParam == VK_UP )			//	UP (commands history)
				{
					theApp.m_wndMain.m_wndInput.SetWindowText(theApp.m_wndMain.m_zCommands[0]);

					for ( int i = 0; i < 4; ++i )
					{
						strncpy(theApp.m_wndMain.m_zCommands[i], theApp.m_wndMain.m_zCommands[i + 1], sizeof(theApp.m_wndMain.m_zCommands[i]) - 1);
						theApp.m_wndMain.m_zCommands[i][sizeof(theApp.m_wndMain.m_zCommands[i]) - 1] = '\0';
					}

					theApp.m_wndMain.m_wndInput.GetWindowText(theApp.m_wndMain.m_zCommands[4], sizeof(theApp.m_wndMain.m_zCommands[4]));
				}
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		EXC_CATCH;
	}
}

#endif // _WIN32
