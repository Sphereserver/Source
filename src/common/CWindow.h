// CWindow = a base window class for controls.
#ifdef _WIN32
#ifndef _INC_CWINDOW_H
#define _INC_CWINDOW_H
#pragma once

#include "../common/graycom.h"
#include <richedit.h>

class CWindow    // similar to Std MFC class CWnd
{
public:
	static const char *m_sClassName;
	HWND m_hWnd;
	NOTIFYICONDATA m_nid;

public:
	operator HWND () const       // cast as a HWND
	{
		return( m_hWnd );
	}
	CWindow()
	{
		m_hWnd = NULL;
		memset(&m_nid, 0, sizeof(m_nid));
		m_nid.cbSize = sizeof(m_nid);
	}
	virtual ~CWindow()
	{
		DestroyWindow();
	}

private:
	CWindow(const CWindow& copy);
	CWindow& operator=(const CWindow& other);

public:
	// Standard message handlers.
	BOOL OnCreate( HWND hwnd, LPCREATESTRUCT lpCreateStruct = NULL  )
	{
		UNREFERENCED_PARAMETER(lpCreateStruct);
		m_hWnd = hwnd;
		return( TRUE );
	}
	void OnDestroy()
	{
		m_hWnd = NULL;
	}
	void OnDestroy( HWND hwnd )
	{
		UNREFERENCED_PARAMETER(hwnd);
		m_hWnd = NULL;
	}

	// Basic window functions.
	BOOL IsWindow() const
	{
		if ( this == NULL )
			return( false );
		if ( m_hWnd == NULL )
			return( false );
		return( ::IsWindow( m_hWnd ));
	}
	HWND GetParent() const
	{
		ASSERT( m_hWnd );
		return( ::GetParent(m_hWnd));
	}
	LRESULT SendMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		ASSERT( m_hWnd );
		return( ::SendMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	BOOL PostMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		ASSERT( m_hWnd );
		return( ::PostMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	HWND GetDlgItem( int id ) const
	{
		ASSERT(m_hWnd);
		return( ::GetDlgItem( m_hWnd, id ));
	}
	BOOL SetDlgItemText( int nIDDlgItem, LPCTSTR lpString ) const
	{
		ASSERT(m_hWnd);
		return( ::SetDlgItemText( m_hWnd, nIDDlgItem, lpString ));
	}

	// Create/Destroy
	void DestroyWindow()
	{
		if ( m_hWnd == NULL )
			return;
		::DestroyWindow( m_hWnd );
		ASSERT( m_hWnd == NULL );
	}

	// Area and location
	BOOL MoveWindow( int X, int Y, int nWidth, int nHeight, BOOL bRepaint = TRUE )
	{
		return( ::MoveWindow( m_hWnd, X, Y, nWidth, nHeight, bRepaint ));
	}
	BOOL SetForegroundWindow()
	{
		ASSERT( m_hWnd );
		return( ::SetForegroundWindow( m_hWnd ));
	}
	HWND SetFocus()
	{
		ASSERT( m_hWnd );
		return( ::SetFocus( m_hWnd ));
	}
	BOOL ShowWindow( int nCmdShow )
	{
		// SW_SHOW
		return( ::ShowWindow( m_hWnd, nCmdShow ));
	}

	// Standard windows props.
	int GetWindowText( LPTSTR lpszText, int iLen )
	{
		ASSERT( m_hWnd );
		return ::GetWindowText( m_hWnd, lpszText, iLen );
	}
	BOOL SetWindowText( LPCTSTR lpszText )
	{
		ASSERT( m_hWnd );
		return ::SetWindowText( m_hWnd, lpszText );
	}

	void SetFont( HFONT hFont, BOOL fRedraw = false )
	{
		SendMessage( WM_SETFONT, (WPARAM) hFont, MAKELPARAM(fRedraw, 0));
	}
   

	HICON SetIcon( HICON hIcon, BOOL fType = false )
	{
		// ICON_BIG vs ICON_SMALL
		return (HICON)SendMessage(WM_SETICON, (WPARAM)fType, (LPARAM)hIcon);
	}

	UINT_PTR SetTimer( UINT_PTR uTimerID, UINT uWaitMsec )
	{
		ASSERT(m_hWnd);
		return( ::SetTimer( m_hWnd, uTimerID, uWaitMsec, NULL ));
	}
	BOOL KillTimer( UINT_PTR uTimerID )
	{
		ASSERT(m_hWnd);
		return( ::KillTimer( m_hWnd, uTimerID ));
	}
	int MessageBox( LPCTSTR lpszText, LPCTSTR lpszTitle, UINT fuStyle = MB_OK ) const
	{
		// ASSERT( m_hWnd ); ok for this to be NULL !
		return( ::MessageBox( m_hWnd, lpszText, lpszTitle, fuStyle ));
	}
	LONG_PTR SetWindowLongPtr( int nIndex, LONG_PTR dwNewLong )
	{
		ASSERT(m_hWnd);
		return( ::SetWindowLongPtr( m_hWnd, nIndex, dwNewLong ));
	}
	LONG_PTR GetWindowLongPtr( int nIndex ) const
	{
		ASSERT(m_hWnd);
		return( ::GetWindowLongPtr( m_hWnd, nIndex ));
	}

	// Listbox
	void AddString(LPCTSTR lpszString) const
	{
		SendMessage(LB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(lpszString));
	}
};

class CDialogBase : public CWindow
{
public:
	static const char *m_sClassName;
	static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

public:
	virtual ~CDialogBase() { };

public:
	virtual INT_PTR DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER(message);
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);
		return FALSE;
	}
};

class CWindowBase : public CWindow
{
public:
	static const char *m_sClassName;
	static LRESULT WINAPI WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
public:
	static ATOM RegisterClass( WNDCLASS & wc );
	virtual LRESULT DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return ::DefWindowProc( m_hWnd, message, wParam, lParam );
	}
};

class CWinApp	// Similar to MFC type
{
public:
	static const char *m_sClassName;
	LPCTSTR	 	m_pszAppName;	// Specifies the name of the application. (display freindly)
	HINSTANCE 	m_hInstance;	// Identifies the current instance of the application.
	CWindow *	m_pMainWnd;		// Holds a pointer to the application's main window. For an example of how to initialize m_pMainWnd, see InitInstance.
	CGString	m_pszExeName;	// The module name of the application.
	CGString	m_pszProfileName;	// the path to the profile.

public:
	CWinApp()
	{
		m_pszAppName = "";
		m_hInstance = NULL;
		m_pMainWnd = NULL;
	}

	virtual ~CWinApp()
	{
	}

private:
	CWinApp(const CWinApp& copy);
	CWinApp& operator=(const CWinApp& other);

public:
	void InitInstance(LPCTSTR pszAppName, HINSTANCE hInstance)
	{
		m_pszAppName = pszAppName;	// assume this is a static data pointer valid forever.
		m_hInstance	= hInstance;

		char szFileName[ _MAX_PATH ];
		if ( ! GetModuleFileName( m_hInstance, szFileName, sizeof( szFileName )))
			return;
		m_pszExeName = szFileName;

        LPTSTR pszTmp = const_cast<LPTSTR>(strrchr( m_pszExeName, '\\' ));	// Get title
		strncpy(szFileName, pszTmp ? pszTmp + 1 : m_pszExeName, sizeof(szFileName));
		szFileName[sizeof(szFileName) - 1] = '\0';

		pszTmp = strrchr( szFileName, '.' );	// Get extension.
		if ( pszTmp != NULL )
			pszTmp[0] = '\0';
		strncat(szFileName, SPHERE_FILE_EXT_INI, sizeof(szFileName) - strlen(szFileName) - 1);

		OFSTRUCT ofs = { };
		if ( OpenFile( szFileName, &ofs, OF_EXIST ) != HFILE_ERROR)
		{
			m_pszProfileName = ofs.szPathName;
		}
		else
		{
			m_pszProfileName = szFileName;
		}
	}

	HICON LoadIcon( int id ) const
	{
		return( ::LoadIcon( m_hInstance, MAKEINTRESOURCE( id )));
	}
	HMENU LoadMenu( int id ) const
	{
		return( ::LoadMenu( m_hInstance, MAKEINTRESOURCE( id )));
	}
};

#endif	// _INC_CWINDOW_H
#endif	// _WIN32