#ifdef _WIN32
#ifndef _INC_CNTWINDOW_H
#define _INC_CNTWINDOW_H

#include "../common/CWindow.h"

class CNTWindow : public CWindow
{
public:
	CNTWindow();
	virtual ~CNTWindow();

	static const char *m_sClassName;

public:
	int m_iLogTextLen;
	bool m_fLogScrollLock;		// lock with the rolling text?
	COLORREF m_dwColorNew;		// setthe color for the next block written
	COLORREF m_dwColorPrv;
	int m_iHeightInput;
	HFONT m_hLogFont;
	CRichEditCtrl m_wndLog;
	CEdit m_wndInput;			// the text input portion at the bottom
	char m_zCommands[5][256];

public:
	void List_Clear();
	void List_Add(COLORREF color, LPCTSTR pszText);
	bool OnCommand(WORD wNotifyCode, INT_PTR wID, HWND hwndCtl);

	static bool RegisterClass(char *className);
	static LRESULT WINAPI WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	int OnCreate(HWND hWnd, LPCREATESTRUCT lParam);
	void OnDestroy();
	void OnSetFocus(HWND hWndLoss);
	LRESULT OnUserTrayNotify(WPARAM wID, LPARAM lEvent);
	void OnUserPostMessage(COLORREF color, CGString *psMsg);
	void OnSize(WPARAM nType, int cx, int cy);
	bool OnClose();
	bool OnSysCommand(WPARAM uCmdType, int xPos, int yPos);
	void SetLogFont(const char *pszFont);
	LRESULT OnNotify(int idCtrl, NMHDR *pnmh);

public:
	class CAboutDlg : public CDialogBase
	{
	private:
		bool OnInitDialog();
		bool OnCommand(WORD wNotifyCode, INT_PTR wID, HWND hwndCtl);

	public:
		virtual BOOL DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam);
	};

	class CStatusDlg : public CDialogBase
	{
	public:
		CListbox m_wndListClients;
		CListbox m_wndListStats;

	private:
		bool OnInitDialog();
		bool OnCommand(WORD wNotifyCode, INT_PTR wID, HWND hwndCtl);

	public:
		void FillClients();
		void FillStats();

		virtual BOOL DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam);
	};

	class CListTextConsole : public CTextConsole
	{
	public:
		CListTextConsole(HWND hWndList)
		{
			m_wndList.m_hWnd = hWndList;
		}
		~CListTextConsole()
		{
			m_wndList.OnDestroy();
		}

	public:
		CListbox m_wndList;

	public:
		virtual LPCTSTR GetName() const
		{
			return "Stats";
		}
		virtual PLEVEL_TYPE GetPrivLevel() const
		{
			return PLEVEL_QTY;
		}
		virtual void SysMessage(LPCTSTR pszMessage) const
		{
			if ( !pszMessage || ISINTRESOURCE(pszMessage) )
				return;

			TCHAR *ppMessages[255];
			size_t iQty = Str_ParseCmds(const_cast<TCHAR *>(pszMessage), ppMessages, COUNTOF(ppMessages), "\n");
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
	CNTWindow::CAboutDlg m_wndAbout;
	CNTWindow::CStatusDlg m_wndStatus;
};

#endif // _INC_CNTWINDOW_H
#endif // _WIN32
