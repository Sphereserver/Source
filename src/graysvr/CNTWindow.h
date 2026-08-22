#ifdef _WIN32
#ifndef _INC_CNTWINDOW_H
#define _INC_CNTWINDOW_H

#include "../common/CWindow.h"

class CNTWindow : public CWindow
{
public:
	static const char *m_sClassName;

	CNTWindow();
	virtual ~CNTWindow();

private:
	CWindow m_wndLog;
	COLORREF m_crLogColor;
	size_t m_iLogTextLen;

	CWindow m_wndInput;
	int m_iInputHistory;
	TCHAR m_szInputHistory[10][SCRIPT_MAX_LINE_LEN];

	HICON m_hIconBig;
	HICON m_hIconSmall;

	static constexpr LPCTSTR sm_pszFontFaceName = "Consolas";
	static constexpr int sm_iFontSize = 10;
	HFONT m_hFont;
	LONG m_lFontCharWidth;
	LONG m_lFontCharHeight;

	bool m_fDarkMode;
	HBRUSH m_hBrushDialogDarkBackground;
	HBRUSH m_hBrushListBoxDarkBackground;

	UINT m_uMsgTaskbarCreated;		// OS broadcasts this registered message ID to all applications when the taskbar restarts

public:
	void MainWindowInit(HINSTANCE hInstance, int iShowCmd);
	void MainWindowExit();
	bool MainWindowTick(UINT uWaitMsec);

private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK InputSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uSubclassID, DWORD_PTR dwRefData);
	LRESULT OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
	LRESULT OnDestroy();
	LRESULT OnSize(WPARAM uType, int iWidth, int iHeight);
	LRESULT OnClose();
	LRESULT OnNotify(WPARAM uControlID, LPNMHDR pnmh);
	LRESULT OnUserTrayNotify(LPARAM lEvent);
	LRESULT OnCommand(WORD wCommandID);
	LRESULT OnSysCommand(int iCommandID);
	LRESULT OnSettingChange(LPCTSTR pszSetting);
	LRESULT OnDpiChanged(UINT uDpi, LPRECT lpRect);

	BOOL CenterWindow(HWND hWnd);
	bool IsSystemDarkMode();

public:
	void UpdateTitle(LPCTSTR pszLoadPercent = NULL);

	void WriteLog(LPCTSTR pszText);
	void SetLogColor(COLORREF color)
	{
		m_crLogColor = color;
	}

private:
	// System has colors for light mode, but not for dark mode, so define them here
	inline COLORREF GetWindowTextColor()
	{
		return m_fDarkMode ? RGB(240, 240, 240) : GetSysColor(COLOR_WINDOWTEXT);
	}
	inline COLORREF GetWindowBackgroundColor()
	{
		return m_fDarkMode ? RGB(26, 26, 26) : GetSysColor(COLOR_WINDOW);
	}

	inline COLORREF GetDialogTextColor()
	{
		return m_fDarkMode ? RGB(178, 178, 178) : GetSysColor(COLOR_BTNTEXT);
	}
	inline COLORREF GetDialogBackgroundColor()
	{
		return m_fDarkMode ? RGB(32, 32, 32) : GetSysColor(COLOR_BTNFACE);
	}
	inline HBRUSH GetDialogBackgroundBrush()
	{
		return m_fDarkMode ? m_hBrushDialogDarkBackground : GetSysColorBrush(COLOR_BTNFACE);
	}

	inline HBRUSH GetListBoxBackgroundBrush()
	{
		return m_fDarkMode ? m_hBrushListBoxDarkBackground : GetSysColorBrush(COLOR_WINDOW);
	}

public:
	class CStatsDialog : public CDialogBase
	{
	public:
		CWindow m_wndListStats;
		CWindow m_wndListClients;

	private:
		virtual INT_PTR DefDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnInitDialog();
		INT_PTR OnCommand(WORD wCommandID);
	};

	class CAboutDialog : public CDialogBase
	{
	private:
		virtual INT_PTR DefDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnInitDialog();
		INT_PTR OnCommand(WORD wCommandID);
		INT_PTR OnNotify(LPNMHDR pnmh);
	};

	class CListTextConsole : public CTextConsole
	{
	public:
		CListTextConsole(HWND hWnd)
		{
			m_wndList.m_hWnd = hWnd;
		}
		~CListTextConsole()
		{
			m_wndList.OnDestroy();
		}

	public:
		CWindow m_wndList;

	public:
		virtual LPCTSTR GetName() const
		{
			return "Stats";
		}
		virtual PLEVEL_TYPE GetPrivLevel() const
		{
			return PLEVEL_QTY;
		}
		virtual void SysMessage(LPCTSTR pszText) const
		{
			if ( !pszText || ISINTRESOURCE(pszText) )
				return;

			TCHAR *ppMessages[100];
			size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszText), ppMessages, COUNTOF(ppMessages), "\n");
			for ( size_t i = 0; i < iQty; ++i )
			{
				if ( *ppMessages[i] )
					m_wndList.AddString(ppMessages[i]);
			}
		}
	};
};

class CNTApp : public CWinApp
{
public:
	static const char *m_sClassName;

public:
	CNTWindow m_wndMain;
	CNTWindow::CStatsDialog m_wndStats;
	CNTWindow::CAboutDialog m_wndAbout;
};

extern CNTApp g_NTApp;

#endif // _INC_CNTWINDOW_H
#endif // _WIN32
