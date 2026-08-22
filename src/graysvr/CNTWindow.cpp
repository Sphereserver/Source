#ifdef _WIN32
#include "graysvr.h"	// predef header
#include <dwmapi.h>
#include "CNTWindow.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

CNTApp g_NTApp;

////////////////////////////////////////////////////////////
// CNTWindow

CNTWindow::CNTWindow()
{
	SetLogColor(CLog::Color::Default);
	m_iLogTextLen = 0;
	m_iInputHistory = -1;
	memset(m_szInputHistory, 0, sizeof(m_szInputHistory));
	m_hIconBig = NULL;
	m_hIconSmall = NULL;
	m_hFont = NULL;
	m_lFontCharWidth = 0;
	m_lFontCharHeight = 0;
	m_fDarkMode = true;
	m_hBrushDialogDarkBackground = CreateSolidBrush(GetDialogBackgroundColor());
	m_hBrushListBoxDarkBackground = CreateSolidBrush(GetWindowBackgroundColor());
	m_uMsgTaskbarCreated = RegisterWindowMessage("TaskbarCreated");
}

CNTWindow::~CNTWindow()
{
	DestroyWindow();
}

void CNTWindow::MainWindowInit(HINSTANCE hInstance, int iShowCmd)
{
	const char *pszClassName = SPHERE_TITLE "Svr";
	g_NTApp.InitInstance(SPHERE_TITLE "Server V" SPHERE_VER_STR_FULL, hInstance);

	LoadLibraryEx("msftedit.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPHERESVR));
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPHERESVR));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = pszClassName;
	RegisterClassEx(&wc);

	DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD dwExStyle = 0;

	m_hWnd = CreateWindow(pszClassName, SPHERE_TITLE_VER,
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		HWND_DESKTOP, NULL, hInstance, NULL);

	UINT uDpi = GetDpiForWindow(m_hWnd);
	RECT rc = {
		0, 0,
		(m_lFontCharWidth * 131) + GetSystemMetricsForDpi(SM_CXBORDER, uDpi) + GetSystemMetricsForDpi(SM_CXVSCROLL, uDpi),	// (130 chars + left margin) + caret + scroll bar
		m_lFontCharHeight * 31 };	// 30 chars + input line

	AdjustWindowRectExForDpi(&rc, dwStyle, FALSE, dwExStyle, uDpi);
	SetWindowPos(m_hWnd, NULL,
		0, 0,
		rc.right - rc.left,
		rc.bottom - rc.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_HIDEWINDOW);

	ShowWindow(iShowCmd);
	UpdateWindow(m_hWnd);
}

void CNTWindow::MainWindowExit()
{
	if ( g_Serv.m_iExitFlag < 0 )
	{
		WCHAR szTitle[50];
		MultiByteToWideChar(CP_UTF8, 0, g_NTApp.m_pszAppName, -1, szTitle, COUNTOF(szTitle) - 1);	// TaskDialog is unicode, so convert ANSI to unicode
		szTitle[COUNTOF(szTitle) - 1] = '\0';

		WCHAR szContent[50];
		swprintf(szContent, COUNTOF(szContent), L"Server terminated by error %d", g_Serv.m_iExitFlag);

		TaskDialog(m_hWnd, NULL, szTitle, NULL, szContent, TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);

		// Keep the window open until the user close it
		while ( MainWindowTick(500) )
		{
		}
	}
}

bool CNTWindow::MainWindowTick(UINT uWaitMsec)
{
	// RETURN: false = exit the app

	if ( uWaitMsec > 0 )
	{
		if ( !m_hWnd || !SetTimer(IDT_MAIN_TICK, uWaitMsec) )
			uWaitMsec = 0;
	}

	// Give the windows message loops a tick
	for (;;)
	{
		EXC_TRY("Tick");

		// Any windows messages? (blocks until a message arrives)
		MSG msg;
		if ( uWaitMsec > 0 )
		{
			if ( GetMessage(&msg, NULL, 0, 0) <= 0 )
			{
				g_Serv.SetExitFlag(5);	// console window closed
				return false;
			}

			if ( (msg.hwnd == m_hWnd) && (msg.message == WM_TIMER) && (msg.wParam == IDT_MAIN_TICK) )
			{
				// Empty the queue and bail out
				KillTimer(IDT_MAIN_TICK);
				uWaitMsec = 0;
				continue;
			}
		}
		else
		{
			if ( !PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
				return true;

			if ( msg.message == WM_QUIT )
			{
				g_Serv.SetExitFlag(5);	// console window closed
				return false;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		EXC_CATCH;
	}
}

LRESULT CALLBACK CNTWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)	// static
{
	try
	{
		switch ( uMsg )
		{
			case WM_CREATE:
				return g_NTApp.m_wndMain.OnCreate(hWnd, reinterpret_cast<LPCREATESTRUCT>(lParam));
			case WM_DESTROY:
				return g_NTApp.m_wndMain.OnDestroy();
			case WM_SIZE:
				return g_NTApp.m_wndMain.OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
			case WM_SETFOCUS:
				g_NTApp.m_wndMain.m_wndInput.SetFocus();
				return 0;
			case WM_CLOSE:
				return g_NTApp.m_wndMain.OnClose();
			case WM_ERASEBKGND:		// ignore this
				return 1;
			case WM_NOTIFY:
				return g_NTApp.m_wndMain.OnNotify(wParam, reinterpret_cast<LPNMHDR>(lParam));
			case WM_USER_TRAY_NOTIFY:
				return g_NTApp.m_wndMain.OnUserTrayNotify(lParam);
			case WM_COMMAND:
				return g_NTApp.m_wndMain.OnCommand(LOWORD(wParam));
			case WM_SYSCOMMAND:
				if ( !g_NTApp.m_wndMain.OnSysCommand(GET_SC_WPARAM(wParam)) )
					return 0;
				break;
			case WM_SETTINGCHANGE:
				return g_NTApp.m_wndMain.OnSettingChange(reinterpret_cast<LPCTSTR>(lParam));
			case WM_DPICHANGED:
				return g_NTApp.m_wndMain.OnDpiChanged(HIWORD(wParam), reinterpret_cast<LPRECT>(lParam));
		}

		if ( uMsg == g_NTApp.m_wndMain.m_uMsgTaskbarCreated )
		{
			// Windows taskbar clears all tray icons when it restarts, so add the tray icon again
			g_NTApp.m_wndMain.m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
			g_NTApp.m_wndMain.m_nid.uCallbackMessage = WM_USER_TRAY_NOTIFY;
			g_NTApp.m_wndMain.m_nid.hIcon = g_NTApp.m_wndMain.m_hIconSmall;
			Shell_NotifyIcon(NIM_ADD, &g_NTApp.m_wndMain.m_nid);
			return 0;
		}
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )
	{
		g_Log.CatchEvent(NULL, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CNTWindow::InputSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uSubclassID, DWORD_PTR dwRefData)	// static
{
	switch ( uMsg )
	{
		case WM_KEYDOWN:
		{
			CNTWindow *pMainWindow = reinterpret_cast<CNTWindow *>(dwRefData);
			if ( !pMainWindow )
				break;

			switch ( wParam )
			{
				case VK_RETURN:
				{
					// User entered a console command
					if ( g_Serv.m_fConsoleTextReadyFlag )
						break;

					TCHAR szTemp[SCRIPT_MAX_LINE_LEN] = { 0 };
					pMainWindow->m_wndInput.SendMessage(WM_GETTEXT, static_cast<WPARAM>(COUNTOF(szTemp)), reinterpret_cast<LPARAM>(szTemp));
					if ( szTemp[0] == '\0' )
						break;

					for ( int i = COUNTOF(pMainWindow->m_szInputHistory) - 1; i > 0; --i )
					{
						memcpy(pMainWindow->m_szInputHistory[i], pMainWindow->m_szInputHistory[i - 1], sizeof(pMainWindow->m_szInputHistory[i]));
					}
					strncpy(pMainWindow->m_szInputHistory[0], szTemp, COUNTOF(pMainWindow->m_szInputHistory[0]) - 1);
					pMainWindow->m_szInputHistory[0][COUNTOF(pMainWindow->m_szInputHistory[0]) - 1] = '\0';

					g_Serv.m_sConsoleText = szTemp;
					g_Serv.m_fConsoleTextReadyFlag = true;
					pMainWindow->m_iInputHistory = -1;
					pMainWindow->m_wndInput.SendMessage(WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(""));
					return 0;
				}
				case VK_ESCAPE:
					pMainWindow->m_wndInput.SendMessage(WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(""));
					return 0;
				case VK_UP:
					// Move up command history
					if ( pMainWindow->m_iInputHistory < static_cast<int>(COUNTOF(pMainWindow->m_szInputHistory)) - 1 )
					{
						if ( pMainWindow->m_szInputHistory[pMainWindow->m_iInputHistory + 1][0] != '\0' )
						{
							++pMainWindow->m_iInputHistory;
							pMainWindow->m_wndInput.SendMessage(WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(pMainWindow->m_szInputHistory[pMainWindow->m_iInputHistory]));
							pMainWindow->m_wndInput.SendMessage(EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
						}
					}
					return 0;
				case VK_DOWN:
					// Move down command history
					if ( pMainWindow->m_iInputHistory >= 0 )
					{
						--pMainWindow->m_iInputHistory;
						LPCTSTR pszTemp = (pMainWindow->m_iInputHistory >= 0) ? pMainWindow->m_szInputHistory[pMainWindow->m_iInputHistory] : "";
						pMainWindow->m_wndInput.SendMessage(WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(pszTemp));
						pMainWindow->m_wndInput.SendMessage(EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
					}
					return 0;
				case VK_LEFT:
				case VK_BACK:
				{
					// Prevent system beep sound when moving the cursor past the beginning of text
					CHARRANGE cr = { 0, 0 };
					pMainWindow->m_wndInput.SendMessage(EM_EXGETSEL, NULL, reinterpret_cast<LPARAM>(&cr));
					if ( (cr.cpMin == cr.cpMax) && (cr.cpMin == 0) )
						return 0;
					break;
				}
				case VK_RIGHT:
				case VK_DELETE:
				{
					// Prevent system beep sound when moving the cursor past the end of text
					CHARRANGE cr = { 0, 0 };
					pMainWindow->m_wndInput.SendMessage(EM_EXGETSEL, NULL, reinterpret_cast<LPARAM>(&cr));
					if ( (cr.cpMin == cr.cpMax) && (cr.cpMin == pMainWindow->m_wndInput.SendMessage(WM_GETTEXTLENGTH)) )
						return 0;
					break;
				}
			}
			break;
		}
		case WM_NCDESTROY:
		{
			RemoveWindowSubclass(hWnd, InputSubclassProc, uSubclassID);
			break;
		}
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CNTWindow::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	// WM_CREATE
	UNREFERENCED_PARAMETER(lpCreateStruct);
	CWindow::OnCreate(hWnd);

	// Set window theme
	m_fDarkMode = IsSystemDarkMode();
	BOOL fDarkMode = static_cast<BOOL>(m_fDarkMode);
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));

	// Create font
	UINT uDpi = GetDpiForWindow(hWnd);
	m_hFont = CreateFont(-MulDiv(sm_iFontSize, uDpi, 72),
		0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, sm_pszFontFaceName);

	HDC hdc = GetDC(hWnd);
	if ( hdc )
	{
		if ( m_hFont )
		{
			HGDIOBJ hFontPrev = SelectObject(hdc, m_hFont);
			TEXTMETRIC tm;
			if ( GetTextMetrics(hdc, &tm) )
			{
				m_lFontCharWidth = tm.tmAveCharWidth;
				m_lFontCharHeight = tm.tmHeight + tm.tmExternalLeading;
			}
			SelectObject(hdc, hFontPrev);
		}
		ReleaseDC(hWnd, hdc);
	}

	CHARFORMAT cf;
	memset(&cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR | CFM_SIZE;
	cf.yHeight = MulDiv(sm_iFontSize, 1440, 72);
	cf.crTextColor = GetWindowTextColor();

	// Create log window
	m_wndLog.m_hWnd = CreateWindowW(MSFTEDIT_CLASS, NULL,
		ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_CLIPSIBLINGS | ES_READONLY | WS_VSCROLL | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_MAIN_LOG)), g_NTApp.m_hInstance, NULL);
	ASSERT(m_wndLog.m_hWnd);

	m_wndLog.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
	m_wndLog.SendMessage(EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&cf));
	m_wndLog.SendMessage(EM_SETBKGNDCOLOR, FALSE, CLog::Color::Black);
	m_wndLog.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(m_lFontCharWidth, 0));
	m_wndLog.SendMessage(EM_SETEVENTMASK, NULL, ENM_KEYEVENTS | ENM_MOUSEEVENTS);
	SetWindowTheme(m_wndLog.m_hWnd, m_fDarkMode ? L"DarkMode_Explorer" : L"Explorer", NULL);
	m_wndLog.ShowWindow(SW_SHOW);

	// Create input window
	m_wndInput.m_hWnd = CreateWindowW(MSFTEDIT_CLASS, NULL,
		ES_AUTOHSCROLL | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_MAIN_INPUT)), g_NTApp.m_hInstance, NULL);
	ASSERT(m_wndInput.m_hWnd);

	m_wndInput.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
	m_wndInput.SendMessage(EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&cf));
	m_wndInput.SendMessage(EM_SETBKGNDCOLOR, FALSE, GetWindowBackgroundColor());
	m_wndInput.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(m_lFontCharWidth, 0));
	m_wndInput.SendMessage(EM_SETTEXTMODE, TM_PLAINTEXT);
	m_wndInput.SendMessage(EM_EXLIMITTEXT, NULL, COUNTOF(m_szInputHistory[0]) - 1);
	m_wndInput.ShowWindow(SW_SHOW);
	SetWindowSubclass(m_wndInput.m_hWnd, InputSubclassProc, IDC_MAIN_INPUT, reinterpret_cast<DWORD_PTR>(this));

	// Update icon scale
	m_hIconBig = static_cast<HICON>(LoadImage(g_NTApp.m_hInstance, MAKEINTRESOURCE(IDI_SPHERESVR), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXICON, uDpi), GetSystemMetricsForDpi(SM_CYICON, uDpi), LR_DEFAULTCOLOR));
	m_hIconSmall = static_cast<HICON>(LoadImage(g_NTApp.m_hInstance, MAKEINTRESOURCE(IDI_SPHERESVR), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXSMICON, uDpi), GetSystemMetricsForDpi(SM_CYSMICON, uDpi), LR_DEFAULTCOLOR));
	SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
	SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));

	// Add tray icon
	m_nid.hWnd = hWnd;
	m_nid.uID = 1;
	m_nid.uFlags = NIF_MESSAGE | NIF_ICON;
	m_nid.uCallbackMessage = WM_USER_TRAY_NOTIFY;
	m_nid.hIcon = m_hIconSmall;
	Shell_NotifyIcon(NIM_ADD, &m_nid);

	UpdateTitle();
	return 0;
}

LRESULT CNTWindow::OnDestroy()
{
	// WM_DESTROY
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
	if ( m_hIconBig )
	{
		DestroyIcon(m_hIconBig);
		m_hIconBig = NULL;
	}
	if ( m_hIconSmall )
	{
		DestroyIcon(m_hIconSmall);
		m_hIconSmall = NULL;
	}
	if ( m_hFont )
	{
		DeleteObject(m_hFont);
		m_hFont = NULL;
	}
	if ( m_hBrushDialogDarkBackground )
	{
		DeleteObject(m_hBrushDialogDarkBackground);
		m_hBrushDialogDarkBackground = NULL;
	}
	if ( m_hBrushListBoxDarkBackground )
	{
		DeleteObject(m_hBrushListBoxDarkBackground);
		m_hBrushListBoxDarkBackground = NULL;
	}
	CWindow::OnDestroy();
	return 0;
}

LRESULT CNTWindow::OnSize(WPARAM uType, int iWidth, int iHeight)
{
	// WM_SIZE
	if ( (uType == SIZE_MINIMIZED) || (uType == SIZE_MAXHIDE) )
		return 0;

	HDWP hdwp = BeginDeferWindowPos(2);
	if ( hdwp )
		hdwp = DeferWindowPos(hdwp, m_wndLog.m_hWnd, NULL, 0, 0, iWidth, iHeight - m_lFontCharHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	if ( hdwp )
		hdwp = DeferWindowPos(hdwp, m_wndInput.m_hWnd, NULL, 0, iHeight - m_lFontCharHeight, iWidth, m_lFontCharHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	if ( hdwp )
		EndDeferWindowPos(hdwp);
	return 0;
}

LRESULT CNTWindow::OnClose()
{
	// WM_CLOSE
	if ( g_Serv.m_iExitFlag == 0 )
	{
		WCHAR szTitle[50];
		MultiByteToWideChar(CP_UTF8, 0, g_NTApp.m_pszAppName, -1, szTitle, COUNTOF(szTitle) - 1);	// TaskDialog is unicode, so convert ANSI to unicode
		szTitle[COUNTOF(szTitle) - 1] = '\0';

		int iButtonPressed = 0;
		TaskDialog(m_hWnd, NULL, szTitle, NULL, L"Are you sure you want to close the server?", TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &iButtonPressed);
		if ( iButtonPressed != IDYES )
			return 0;
	}

	if ( g_NTApp.m_wndStats.m_hWnd )
		g_NTApp.m_wndStats.DestroyWindow();
	if ( g_NTApp.m_wndAbout.m_hWnd )
		g_NTApp.m_wndAbout.DestroyWindow();

	m_wndLog.DestroyWindow();	// destroy log window to make sure the console will close properly even if stuck
	g_Serv.SetExitFlag(5);		// console window closed
	PostQuitMessage(0);
	return 0;
}

LRESULT CNTWindow::OnNotify(WPARAM uControlID, LPNMHDR pnmh)
{
	// WM_NOTIFY
	ASSERT(pnmh);
	if ( uControlID != IDC_MAIN_LOG )
		return 0;

	switch ( pnmh->code )
	{
		case EN_MSGFILTER:
		{
			MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER *>(pnmh);
			ASSERT(pMsgFilter);

			switch ( pMsgFilter->msg )
			{
				case WM_RBUTTONUP:
				{
					HMENU hMenu = g_NTApp.LoadMenu(IDR_POPUP_CONSOLE);
					if ( hMenu )
					{
						HMENU hMenuPop = GetSubMenu(hMenu, 0);
						if ( hMenuPop )
						{
							POINT pt;
							if ( GetCursorPos(&pt) )
							{
								SetForegroundWindow();
								TrackPopupMenu(hMenuPop, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
								PostMessage(WM_NULL);
							}
						}
						DestroyMenu(hMenu);
					}
					return 1;
				}
				case WM_LBUTTONDBLCLK:
				{
					// Double click on script files should open them
					POINT pt = { 0,0 };
					pt.x = LOWORD(pMsgFilter->lParam);
					pt.y = HIWORD(pMsgFilter->lParam);

					LRESULT lrLine = m_wndLog.SendMessage(EM_LINEFROMCHAR, m_wndLog.SendMessage(EM_CHARFROMPOS, NULL, reinterpret_cast<LPARAM>(&pt)));

					TCHAR *pszTemp = Str_GetTemp();
					*reinterpret_cast<WORD *>(pszTemp) = SCRIPT_MAX_LINE_LEN - 1;		// first WORD is used to indicate the max buffer length

					LRESULT lrLen = m_wndLog.SendMessage(EM_GETLINE, lrLine, reinterpret_cast<LPARAM>(pszTemp));
					if ( lrLen <= 0 )
						break;
					pszTemp[lrLen] = '\0';

					// Search for 2 formats of filename:
					//  Loading path/filename.scp
					//  ERROR:(filename.scp,line)Message
					TCHAR *pszEnd = strstr(pszTemp, SPHERE_FILE_EXT_SCP);
					if ( !pszEnd )
						break;
					pszEnd += COUNTOF(SPHERE_FILE_EXT_SCP) - 1;
					*pszEnd = '\0';

					TCHAR *pszStart = pszEnd;
					while ( pszStart > pszTemp )
					{
						if ( (*(pszStart - 1) == ' ') || (*(pszStart - 1) == '(') )
							break;
						--pszStart;
					}
					if ( *pszStart == '\0' )
						break;

					// Check if this is a valid script file
					LPCTSTR pszFilePath = NULL;
					size_t i = 0;
					for ( const CResourceScript *pScript = g_Cfg.GetResourceFile(i++); pScript != NULL; pScript = g_Cfg.GetResourceFile(i++) )
					{
						if ( !strstr(pScript->GetFilePath(), pszStart) )
							continue;

						pszFilePath = pScript->GetFilePath();
						break;
					}

					TCHAR szBaseDir[_MAX_PATH] = { 0 };
					if ( !pszFilePath && strstr(SPHERE_FILE "tables" SPHERE_FILE_EXT_SCP, pszStart) )
					{
						snprintf(szBaseDir, COUNTOF(szBaseDir), "%s%s", static_cast<LPCTSTR>(g_Cfg.m_sSCPBaseDir), pszStart);
						pszFilePath = szBaseDir;
					}

					// Open the file
					if ( pszFilePath )
					{
						TCHAR szFullPath[_MAX_PATH] = { 0 };
						if ( !_fullpath(szFullPath, pszFilePath, COUNTOF(szFullPath)) )
							break;

						SHELLEXECUTEINFO sei;
						memset(&sei, 0, sizeof(sei));
						sei.cbSize = sizeof(sei);
						sei.fMask = SEE_MASK_FLAG_NO_UI;
						sei.hwnd = m_hWnd;
						sei.lpVerb = "open";
						sei.lpFile = szFullPath;
						sei.nShow = SW_SHOWNORMAL;

						if ( ShellExecuteEx(&sei) )
							return 1;

						DWORD dwErrorCode = GetLastError();
						LPTSTR pszErrorMsg = Str_GetTemp();
						if ( CGrayError::GetSystemErrorMessage(dwErrorCode, pszErrorMsg, THREAD_STRING_LENGTH) > 0 )
							g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%" FMTDWORD " (%s)\n", szFullPath, dwErrorCode, pszErrorMsg);
						else
							g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%" FMTDWORD "\n", szFullPath, dwErrorCode);
					}
					break;
				}
				case WM_CHAR:
				{
					// When try to write on log window, redirect to input window
					if ( (pMsgFilter->lParam & (KF_ALTDOWN << 16)) || (GetKeyState(VK_CONTROL) < 0) )	// allow ALT and CTRL
						break;

					m_wndInput.SetFocus();
					m_wndInput.SendMessage(WM_CHAR, pMsgFilter->wParam, pMsgFilter->lParam);
					return 1;
				}
			}
		}
	}
	return 0;
}

LRESULT CNTWindow::OnUserTrayNotify(LPARAM lEvent)
{
	// WM_USER_TRAY_NOTIFY
	switch ( lEvent )
	{
		case WM_RBUTTONUP:
		{
			HMENU hMenu = g_NTApp.LoadMenu(IDR_POPUP_TRAY);
			if ( hMenu )
			{
				HMENU hMenuPop = GetSubMenu(hMenu, 0);
				if ( hMenuPop )
				{
					POINT pt;
					if ( GetCursorPos(&pt) )
					{
						SetForegroundWindow();
						TrackPopupMenu(hMenuPop, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
						PostMessage(WM_NULL);
					}
				}
				DestroyMenu(hMenu);
			}
			return 0;
		}
		case WM_LBUTTONDBLCLK:
			PostMessage(WM_SYSCOMMAND, IsWindowVisible(m_hWnd) ? SC_MINIMIZE : SC_RESTORE);
			return 0;
	}
	return 1;
}

LRESULT CNTWindow::OnCommand(WORD wCommandID)
{
	// WM_COMMAND
	switch ( wCommandID )
	{
		case IDM_COPY:
			m_wndLog.SendMessage(WM_COPY);
			break;
		case IDM_RESYNC_PAUSE:
			if ( g_Serv.m_fConsoleTextReadyFlag )
				break;

			g_Serv.m_sConsoleText = "R";
			g_Serv.m_fConsoleTextReadyFlag = true;
			m_wndInput.SetFocus();
			break;
		case IDM_SHOW_STATS:
			if ( !g_NTApp.m_wndStats.m_hWnd )
			{
				g_NTApp.m_wndStats.m_hWnd = CreateDialogParam(
					g_NTApp.m_hInstance,
					MAKEINTRESOURCE(IDD_STATS),
					m_hWnd,
					CDialogBase::DialogProc,
					reinterpret_cast<LPARAM>(static_cast<CDialogBase *>(&g_NTApp.m_wndStats)));

				g_NTApp.m_wndStats.SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
				g_NTApp.m_wndStats.SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));
			}
			CenterWindow(g_NTApp.m_wndStats.m_hWnd);
			g_NTApp.m_wndStats.ShowWindow(SW_SHOW);
			g_NTApp.m_wndStats.SetForegroundWindow();
			break;
		case IDM_SHOW_ABOUT:
			if ( !g_NTApp.m_wndAbout.m_hWnd )
			{
				g_NTApp.m_wndAbout.m_hWnd = CreateDialogParam(
					g_NTApp.m_hInstance,
					MAKEINTRESOURCE(IDD_ABOUT),
					m_hWnd,
					CDialogBase::DialogProc,
					reinterpret_cast<LPARAM>(static_cast<CDialogBase *>(&g_NTApp.m_wndAbout)));

				g_NTApp.m_wndAbout.SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
				g_NTApp.m_wndAbout.SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));
			}
			CenterWindow(g_NTApp.m_wndAbout.m_hWnd);
			g_NTApp.m_wndAbout.ShowWindow(SW_SHOW);
			g_NTApp.m_wndAbout.SetForegroundWindow();
			break;
		case IDM_EXIT:
			PostMessage(WM_CLOSE);
			break;
	}
	return 0;
}

LRESULT CNTWindow::OnSysCommand(int iCommandID)
{
	// WM_SYSCOMMAND
	switch ( iCommandID )
	{
		case SC_MINIMIZE:
			ShowWindow(SW_HIDE);
			return 0;
		case SC_RESTORE:
			ShowWindow(SW_RESTORE);
			SetForegroundWindow();
			return 0;
	}
	return 1;
}

LRESULT CNTWindow::OnSettingChange(LPCTSTR pszSetting)
{
	// WM_SETTINGCHANGE
	if ( pszSetting && (strcmpi(pszSetting, "ImmersiveColorSet") == 0) )
	{
		m_fDarkMode = IsSystemDarkMode();
		BOOL fDarkMode = static_cast<BOOL>(m_fDarkMode);
		LPCWSTR pszThemeName = m_fDarkMode ? L"DarkMode_Explorer" : L"Explorer";

		if ( g_NTApp.m_wndMain.m_hWnd )
		{
			CHARFORMAT cf;
			memset(&cf, 0, sizeof(cf));
			cf.cbSize = sizeof(cf);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = GetWindowTextColor();

			g_NTApp.m_wndMain.m_wndInput.SendMessage(EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&cf));
			g_NTApp.m_wndMain.m_wndInput.SendMessage(EM_SETBKGNDCOLOR, FALSE, GetWindowBackgroundColor());

			DwmSetWindowAttribute(g_NTApp.m_wndMain.m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));
			SetWindowTheme(g_NTApp.m_wndMain.m_wndLog.m_hWnd, pszThemeName, NULL);
			InvalidateRect(g_NTApp.m_wndMain.m_hWnd, NULL, TRUE);
		}
		if ( g_NTApp.m_wndStats.m_hWnd )
		{
			DwmSetWindowAttribute(g_NTApp.m_wndStats.m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));
			SetWindowTheme(g_NTApp.m_wndStats.m_wndListStats.m_hWnd, pszThemeName, NULL);
			SetWindowTheme(g_NTApp.m_wndStats.m_wndListClients.m_hWnd, pszThemeName, NULL);
			InvalidateRect(g_NTApp.m_wndStats.m_hWnd, NULL, TRUE);
		}
		if ( g_NTApp.m_wndAbout.m_hWnd )
		{
			DwmSetWindowAttribute(g_NTApp.m_wndAbout.m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));
			InvalidateRect(g_NTApp.m_wndAbout.m_hWnd, NULL, TRUE);
		}
		return 0;
	}
	return 1;
}

LRESULT CNTWindow::OnDpiChanged(UINT uDpi, LPRECT lpRect)
{
	// WM_DPICHANGED
	// Update font metrics
	HDC hdc = GetDC(m_hWnd);
	if ( hdc )
	{
		HFONT hFontTemp = CreateFont(-MulDiv(sm_iFontSize, uDpi, 72),
			0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, sm_pszFontFaceName);

		if ( hFontTemp )
		{
			HGDIOBJ hFontPrev = SelectObject(hdc, hFontTemp);
			TEXTMETRIC tm;
			if ( GetTextMetrics(hdc, &tm) )
			{
				m_lFontCharWidth = tm.tmAveCharWidth;
				m_lFontCharHeight = tm.tmHeight + tm.tmExternalLeading;
			}
			SelectObject(hdc, hFontPrev);
			DeleteObject(hFontTemp);
		}
		ReleaseDC(m_hWnd, hdc);
	}

	// Update window size and position
	SetWindowPos(m_hWnd, NULL,
		lpRect->left,
		lpRect->top,
		lpRect->right - lpRect->left,
		lpRect->bottom - lpRect->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// Update icon scale
	HICON hIconBigPrev = m_hIconBig;
	HICON hIconSmallPrev = m_hIconSmall;
	m_hIconBig = static_cast<HICON>(LoadImage(g_NTApp.m_hInstance, MAKEINTRESOURCE(IDI_SPHERESVR), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXICON, uDpi), GetSystemMetricsForDpi(SM_CYICON, uDpi), LR_DEFAULTCOLOR));
	m_hIconSmall = static_cast<HICON>(LoadImage(g_NTApp.m_hInstance, MAKEINTRESOURCE(IDI_SPHERESVR), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXSMICON, uDpi), GetSystemMetricsForDpi(SM_CYSMICON, uDpi), LR_DEFAULTCOLOR));
	SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
	SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));

	m_nid.uFlags = NIF_ICON;
	m_nid.hIcon = m_hIconSmall;
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);

	if ( hIconBigPrev )
		DestroyIcon(hIconBigPrev);
	if ( hIconSmallPrev )
		DestroyIcon(hIconSmallPrev);

	return 0;
}

BOOL CNTWindow::CenterWindow(HWND hWnd)
{
	RECT rcParent;
	GetWindowRect(m_hWnd, &rcParent);

	RECT rcChild;
	GetWindowRect(hWnd, &rcChild);

	int iPosX = rcParent.left + (((rcParent.right - rcParent.left) - (rcChild.right - rcChild.left)) / 2);
	int iPosY = rcParent.top + (((rcParent.bottom - rcParent.top) - (rcChild.bottom - rcChild.top)) / 2);
	return SetWindowPos(hWnd, NULL, iPosX, iPosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

bool CNTWindow::IsSystemDarkMode()
{
	DWORD dwValue = 1;
	DWORD dwValueSize = sizeof(dwValue);
	RegGetValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", "AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &dwValue, &dwValueSize);
	return (dwValue == 0);
}

void CNTWindow::UpdateTitle(LPCTSTR pszLoadPercent)
{
	TCHAR szTemp[160];
	snprintf(szTemp, COUNTOF(szTemp), "%s - %s (%s) %s", g_NTApp.m_pszAppName, g_Serv.GetName(), ((g_Serv.m_iModeCode < 0) || (static_cast<size_t>(g_Serv.m_iModeCode) >= COUNTOF(g_Serv.sm_szServModes))) ? "" : g_Serv.sm_szServModes[g_Serv.m_iModeCode], pszLoadPercent ? pszLoadPercent : "");

	// Update window title
	SendMessage(WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(szTemp));

	// Update tray title
	m_nid.uFlags = NIF_TIP;
	strncpy(m_nid.szTip, szTemp, COUNTOF(m_nid.szTip) - 1);
	m_nid.szTip[COUNTOF(m_nid.szTip) - 1] = '\0';
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

void CNTWindow::WriteLog(LPCTSTR pszText)
{
	if ( !pszText )
		return;

	// Crop old text
	size_t iTextLen = strlen(pszText);
	static constexpr size_t iLogMaxTextLen = 64 * 1024;	// 64KB
	if ( m_iLogTextLen + iTextLen >= iLogMaxTextLen )
	{
		LRESULT lrCropLine = m_wndLog.SendMessage(EM_LINEINDEX, m_wndLog.SendMessage(EM_LINEFROMCHAR, iLogMaxTextLen / 20));
		m_wndLog.SendMessage(EM_SETSEL, 0, (lrCropLine >= 0) ? lrCropLine : static_cast<LPARAM>(iLogMaxTextLen / 20));
		m_wndLog.SendMessage(EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(""));
		m_iLogTextLen = static_cast<size_t>(m_wndLog.SendMessage(WM_GETTEXTLENGTH));
	}
	m_iLogTextLen += iTextLen;

	// Insert new text
	CHARFORMAT cf;
	memset(&cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = m_crLogColor;

	m_wndLog.SendMessage(EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
	m_wndLog.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
	m_wndLog.SendMessage(EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(pszText));
}

////////////////////////////////////////////////////////////
// CStatsDialog

INT_PTR CNTWindow::CStatsDialog::DefDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch ( uMsg )
	{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_DESTROY:
			m_wndListStats.OnDestroy();
			m_wndListClients.OnDestroy();
			OnDestroy();
			return TRUE;
		case WM_COMMAND:
			return OnCommand(LOWORD(wParam));
		case WM_CTLCOLORLISTBOX:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetTextColor(hdc, g_NTApp.m_wndMain.GetDialogTextColor());
			SetBkMode(hdc, TRANSPARENT);
			return reinterpret_cast<INT_PTR>(g_NTApp.m_wndMain.GetListBoxBackgroundBrush());
		}
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetTextColor(hdc, g_NTApp.m_wndMain.GetDialogTextColor());
			SetBkMode(hdc, TRANSPARENT);
			// fall through
		}
		case WM_CTLCOLORDLG:
			return reinterpret_cast<INT_PTR>(g_NTApp.m_wndMain.GetDialogBackgroundBrush());
	}
	return FALSE;
}

INT_PTR CNTWindow::CStatsDialog::OnInitDialog()
{
	// WM_INITDIALOG
	BOOL fDarkMode = static_cast<BOOL>(g_NTApp.m_wndMain.m_fDarkMode);
	DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));

	LPCWSTR pszThemeName = fDarkMode ? L"DarkMode_Explorer" : L"Explorer";

	// Fill statistics list
	m_wndListStats.m_hWnd = GetDlgItem(IDC_STATS_STATISTICS);
	SetWindowTheme(m_wndListStats.m_hWnd, pszThemeName, NULL);

	CNTWindow::CListTextConsole listStats(m_wndListStats.m_hWnd);
	size_t iThreadCount = ThreadHolder::getActiveThreads();
	for ( size_t i = 0; i < iThreadCount; ++i )
	{
		IThread *pThread = ThreadHolder::getThreadAt(i);
		if ( !pThread )
			continue;

		const ProfileData &profile = static_cast<AbstractSphereThread *>(pThread)->m_profile;
		if ( !profile.IsEnabled() )
			continue;

		listStats.SysMessagef("Thread %zu: %s (ID=%lu, Priority=%d)\n", i + 1, pThread->getName(), pThread->getId(), pThread->getPriority());
		for ( PROFILE_TYPE j = PROFILE_IDLE; j < PROFILE_QTY; j = static_cast<PROFILE_TYPE>(j + 1) )
		{
			if ( !profile.IsEnabled(j) )
				continue;

			listStats.SysMessagef("'%-10s' = %s\n", profile.GetName(j), profile.GetDescription(j));
		}
	}

	// Fill clients list
	m_wndListClients.m_hWnd = GetDlgItem(IDC_STATS_CLIENTS);
	SetWindowTheme(m_wndListClients.m_hWnd, pszThemeName, NULL);

	CNTWindow::CListTextConsole listClients(m_wndListClients.m_hWnd);
	g_Serv.ListClients(&listClients);
	return TRUE;
}

INT_PTR CNTWindow::CStatsDialog::OnCommand(WORD wCommandID)
{
	// WM_COMMAND
	switch ( wCommandID )
	{
		case IDCANCEL:
			DestroyWindow();
			return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////
// CAboutDialog

INT_PTR CNTWindow::CAboutDialog::DefDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_DESTROY:
			OnDestroy();
			return TRUE;
		case WM_COMMAND:
			return OnCommand(LOWORD(wParam));
		case WM_NOTIFY:
			return OnNotify(reinterpret_cast<LPNMHDR>(lParam));
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetTextColor(hdc, g_NTApp.m_wndMain.GetDialogTextColor());
			SetBkMode(hdc, TRANSPARENT);
			// fall through
		}
		case WM_CTLCOLORDLG:
			return reinterpret_cast<INT_PTR>(g_NTApp.m_wndMain.GetDialogBackgroundBrush());
	}
	return FALSE;
}

INT_PTR CNTWindow::CAboutDialog::OnInitDialog()
{
	// WM_INITDIALOG
	BOOL fDarkMode = static_cast<BOOL>(g_NTApp.m_wndMain.m_fDarkMode);
	DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &fDarkMode, sizeof(fDarkMode));

	TCHAR szBuild[80];
#if defined(GIT_COMMIT_COUNT) && defined(GIT_COMMIT_HASH)
	snprintf(szBuild, COUNTOF(szBuild), "Compiled on %s (build %d / Git hash %s)", g_szCompiledDate, GIT_COMMIT_COUNT, GIT_COMMIT_HASH);
#else
	snprintf(szBuild, COUNTOF(szBuild), "Compiled on %s", g_szCompiledDate);
#endif

	SetDlgItemText(IDC_ABOUT_VERSION, SPHERE_TITLE_VER " (" SPHERE_VER_ARCH ")");
	SetDlgItemText(IDC_ABOUT_COMPILER, szBuild);
	SetDlgItemText(IDC_ABOUT_UPDATE, "<a href=\"https://github.com/Sphereserver/Source/releases\">Check for updates on GitHub</a>");
	SetDlgItemText(IDC_ABOUT_WEBSITE, "<a href=\"" SPHERE_WEBSITE "\">" SPHERE_WEBSITE "</a>");
	return TRUE;
}

INT_PTR CNTWindow::CAboutDialog::OnCommand(WORD wCommandID)
{
	// WM_COMMAND
	switch ( wCommandID )
	{
		case IDCANCEL:
			DestroyWindow();
			return TRUE;
	}
	return FALSE;
}

INT_PTR CNTWindow::CAboutDialog::OnNotify(LPNMHDR pnmh)
{
	// WM_NOTIFY
	switch ( pnmh->code )
	{
		case NM_CLICK:
		{
			switch ( pnmh->idFrom )
			{
				case IDC_ABOUT_UPDATE:
				case IDC_ABOUT_WEBSITE:
				{
					PNMLINK pnmLink = reinterpret_cast<PNMLINK>(pnmh);
					ShellExecuteW(NULL, L"open", pnmLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

#endif // _WIN32
