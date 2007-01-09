#ifndef _INC_CWINDOW_H
#define _INC_CWINDOW_H
#pragma once

#include "cstring.h"
#include <RICHEDIT.H>	// CRichEditCtrl

class CWindow    // similar to Std MFC class CWnd
{
public:
	static const char *m_sClassName;
	HWND m_hWnd;
public:
	operator HWND () const       // cast as a HWND
	{
		return( m_hWnd );
	}
	CWindow()
	{
		m_hWnd = NULL;
	}
	~CWindow()
	{
		DestroyWindow();
	}

	// Standard message handlers.
	BOOL OnCreate( HWND hwnd, LPCREATESTRUCT lpCreateStruct = NULL  )
	{
		m_hWnd = hwnd;
		return true;
	}
	void OnDestroy()
	{
		m_hWnd = NULL;
	}
	void OnDestroy( HWND hwnd )
	{
		m_hWnd = NULL;
	}

	// Basic window functions.
	BOOL IsWindow() const
	{
		if ( this == NULL )
			return false;
		if ( m_hWnd == NULL )
			return false;
		return( ::IsWindow( m_hWnd ));
	}
	HWND GetParent() const
	{
		return( ::GetParent(m_hWnd));
	}
	LRESULT SendMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		return( ::SendMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	BOOL PostMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		return( ::PostMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	HWND GetDlgItem( int id ) const
	{
		return( ::GetDlgItem( m_hWnd, id ));
	}
	BOOL SetDlgItemText( int nIDDlgItem, LPCSTR lpString )
	{
		return( ::SetDlgItemText( m_hWnd, nIDDlgItem, lpString ));
	}

	// Create/Destroy
	void DestroyWindow()
	{
		if ( m_hWnd == NULL )
			return;
		::DestroyWindow( m_hWnd );
	}

	// Area and location
	BOOL MoveWindow( int X, int Y, int nWidth, int nHeight, BOOL bRepaint = TRUE )
	{
		return( ::MoveWindow( m_hWnd, X, Y, nWidth, nHeight, bRepaint ));
	}
	BOOL SetForegroundWindow()
	{
		return( ::SetForegroundWindow( m_hWnd ));
	}
	HWND SetFocus()
	{
		return( ::SetFocus( m_hWnd ));
	}
	BOOL ShowWindow( int nCmdShow )
	{
		// SW_SHOW
		return( ::ShowWindow( m_hWnd, nCmdShow ));
	}

	// Standard windows props.
	int GetWindowText( LPSTR lpszText, int iLen )
	{
		return ::GetWindowText( m_hWnd, lpszText, iLen );
	}
	BOOL SetWindowText( LPCSTR lpszText )
	{
		return ::SetWindowText( m_hWnd, lpszText );
	}

	void SetFont( HFONT hFont, BOOL fRedraw = false )
	{
		SendMessage( WM_SETFONT, (WPARAM) hFont, MAKELPARAM(fRedraw, 0));
	}
   

	HICON SetIcon( HICON hIcon, BOOL fType = false )
	{
		// ICON_BIG vs ICON_SMALL
		return( (HICON)(DWORD) SendMessage( WM_SETICON, (WPARAM)fType, (LPARAM) hIcon ));
	}

	UINT SetTimer( UINT uTimerID, UINT uWaitmSec )
	{
		return( ::SetTimer( m_hWnd, uTimerID, uWaitmSec, NULL ));
	}
	BOOL KillTimer( UINT uTimerID )
	{
		return( ::KillTimer( m_hWnd, uTimerID ));
	}
	int MessageBox( LPCSTR lpszText, LPCSTR lpszTitle, UINT fuStyle = MB_OK	) const
	{
		return( ::MessageBox( m_hWnd, lpszText, lpszTitle, fuStyle ));
	}
	LONG SetWindowLong( int nIndex, LONG dwNewLong )
	{
		return( ::SetWindowLong( m_hWnd, nIndex, dwNewLong ));
	}
	LONG GetWindowLong( int nIndex ) const
	{
		return( ::GetWindowLong( m_hWnd, nIndex ));
	}

	int SetDlgItemText( int ID, LPCSTR lpszText ) const
	{
		return ::SetDlgItemText(m_hWnd, ID, lpszText);
	}
};

class CDialogBase : public CWindow
{
public:
	static const char *m_sClassName;
	static BOOL CALLBACK DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
public:
	virtual BOOL DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return FALSE;
	}
};

class CWindowBase : public CWindow
{
public:
	static const char *m_sClassName;
	static LRESULT WINAPI WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
public:
	virtual LRESULT DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return ::DefWindowProc( m_hWnd, message, wParam, lParam );
	}
};

class CScrollBar : public CWindow
{
// Constructors
public:
	static const char *m_sClassName;
	CScrollBar::CScrollBar() 
	{
	}

// Attributes
	void CScrollBar::GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const
	{
		::GetScrollRange(m_hWnd, SB_CTL, lpMinPos, lpMaxPos);
	}
	BOOL CScrollBar::GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask)
	{
		lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
		lpScrollInfo->fMask = nMask;
		return ::GetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo);
	}

// Implementation
public:
	virtual ~CScrollBar()
	{
	}
};

class CEdit : public CWindow
{
// Constructors
public:
	static const char *m_sClassName;
	CEdit() {}

// Operations

	void SetSel( DWORD dwSelection, BOOL bNoScroll = FALSE )
	{
		SendMessage( EM_SETSEL, (WPARAM) dwSelection, (LPARAM) dwSelection );
	}
	void SetSel( int nStartChar, int nEndChar, BOOL bNoScroll = FALSE )
	{
		SendMessage( EM_SETSEL, (WPARAM) nStartChar, (LPARAM) nEndChar );
	}
	DWORD GetSel() const
	{
		return((DWORD) SendMessage( EM_GETSEL ));
	}
	void GetSel(int& nStartChar, int& nEndChar) const
	{
		DWORD dwSel = GetSel();
		nStartChar = LOWORD(dwSel);
		nEndChar = HIWORD(dwSel);
	}

	void ReplaceSel( LPCTSTR lpszNewText, BOOL bCanUndo = FALSE )
	{
		SendMessage( EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM) lpszNewText );
	}

// Implementation
public:
	virtual ~CEdit()
	{
	}
};



class CRichEditCtrl : public CEdit
{
public:
	static const char *m_sClassName;
	COLORREF SetBackgroundColor( BOOL bSysColor, COLORREF cr )
	{ 
		return( (COLORREF)(DWORD) SendMessage( EM_SETBKGNDCOLOR, (WPARAM) bSysColor, (LPARAM) cr ));
	}

	void SetSel( int nStartChar, int nEndChar, BOOL bNoScroll = FALSE )
	{
		CHARRANGE range;
		range.cpMin = nStartChar;
		range.cpMax = nEndChar;
		SendMessage( EM_EXSETSEL, 0, (LPARAM) &range );
	}
	void GetSel(int& nStartChar, int& nEndChar) const
	{
		CHARRANGE range;
		SendMessage( EM_EXGETSEL, 0, (LPARAM) &range );
		nStartChar = range.cpMin;
		nEndChar = range.cpMax;
	}

	DWORD Scroll( int iAction = SB_PAGEDOWN )
	{
		return( (DWORD) SendMessage( EM_SCROLL, (WPARAM) iAction ));
	}

	// Formatting.
	BOOL SetDefaultCharFormat( CHARFORMAT& cf )
	{
		return( (BOOL)(DWORD) SendMessage( EM_SETCHARFORMAT, (WPARAM) SCF_DEFAULT, (LPARAM) &cf ));
	}
	BOOL SetSelectionCharFormat( CHARFORMAT& cf )
	{
		return( (BOOL)(DWORD) SendMessage( EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf ));
	}

	// Events.
	long GetEventMask() const
	{
		return( (DWORD) SendMessage( EM_GETEVENTMASK ));
	}
	DWORD SetEventMask( DWORD dwEventMask = ENM_NONE )
	{
		// ENM_NONE = default.
		return( (DWORD) SendMessage( EM_SETEVENTMASK, 0, (LPARAM) dwEventMask ));
	}
};

class CListbox : public CWindow
{
// Constructors
public:
	static const char *m_sClassName;
	CListbox() {}

// Operations

	void ResetContent()
	{
		SendMessage( LB_RESETCONTENT );
	}
	int GetCount() const
	{
		return( (int)(DWORD) SendMessage( LB_GETCOUNT ));
	}
	int AddString( LPCTSTR lpsz ) const
	{
		return( (int)(DWORD) SendMessage( LB_ADDSTRING, 0L, (LPARAM)(lpsz)));
	}

// Implementation
public:
	virtual ~CListbox()
	{
	}
};

#endif
