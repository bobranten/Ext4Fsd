/////////////////////////////////////////////////////////////////////////////
// SystemTray.cpp : implementation file 
//
/////////////////////////////////////////////////////////////////////////////
    
#include "stdafx.h"
#include "SysTray.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSystemTray, CObject)

/////////////////////////////////////////////////////////////////////////////
// CSystemTray construction/creation/destruction

CSystemTray::CSystemTray()
{
    ZeroMemory(&m_tnd, sizeof(m_tnd));
    m_bEnabled = FALSE;
    m_bHidden  = FALSE;
}

CSystemTray::CSystemTray(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szToolTip, 
                     HICON icon, UINT uID)
{
    Create(pWnd, uCallbackMessage, szToolTip, icon, uID);
    m_bHidden = FALSE;
}

BOOL CSystemTray::Create(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szToolTip, 
                       HICON icon, UINT uID)
{
    // this is only for Windows 95 (or higher)
	m_bEnabled = ( GetVersion() & 0xff );
    ASSERT(m_bEnabled >= 4);
    if (!m_bEnabled) 
		return FALSE;

    //Make sure Notification window is valid
    m_bEnabled = (pWnd && ::IsWindow(pWnd->GetSafeHwnd()));
    if (!m_bEnabled) return FALSE;
    
    //Make sure we avoid conflict with other messages
    ASSERT(uCallbackMessage >= WM_USER);

    //Tray only supports tooltip text up to 64 characters
    ASSERT(_tcslen(szToolTip) <= 64);

    // load up the NOTIFYICONDATA structure
    m_tnd.cbSize = sizeof(NOTIFYICONDATA);
    m_tnd.hWnd     = pWnd->GetSafeHwnd();
    m_tnd.uID     = uID ^ GetCurrentProcessId();
    m_tnd.hIcon  = icon;
    m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_tnd.uCallbackMessage = uCallbackMessage;
    strcpy (m_tnd.szTip, szToolTip);

    // Set the tray icon
	m_bEnabled = Shell_NotifyIcon(NIM_ADD, &m_tnd);
    ASSERT(m_bEnabled);
    return m_bEnabled;
}

CSystemTray::~CSystemTray()
{
    RemoveIcon();
}


/////////////////////////////////////////////////////////////////////////////
// CSystemTray icon manipulation

void CSystemTray::MoveToRight()
{
    HideIcon();
    ShowIcon();
}

void CSystemTray::RemoveIcon()
{
    if (!m_bEnabled) return;

    m_tnd.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &m_tnd);
    m_bEnabled = FALSE;
}

void CSystemTray::HideIcon()
{
    if (m_bEnabled && !m_bHidden) {
        m_tnd.uFlags = NIF_ICON;
        Shell_NotifyIcon (NIM_DELETE, &m_tnd);
        m_bHidden = TRUE;
    }
}

void CSystemTray::ShowIcon()
{
    if (m_bEnabled && m_bHidden) {
        m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        Shell_NotifyIcon(NIM_ADD, &m_tnd);
        m_bHidden = FALSE;
    }
}

BOOL CSystemTray::SetIcon(HICON hIcon)
{
    if (!m_bEnabled) return FALSE;

    m_tnd.uFlags = NIF_ICON;
    m_tnd.hIcon = hIcon;

    return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetIcon(LPCTSTR lpszIconName)
{
    HICON hIcon = AfxGetApp()->LoadIcon(lpszIconName);

    return SetIcon(hIcon);
}

BOOL CSystemTray::SetIcon(UINT nIDResource)
{
    HICON hIcon = AfxGetApp()->LoadIcon(nIDResource);

    return SetIcon(hIcon);
}

BOOL CSystemTray::SetStandardIcon(LPCTSTR lpIconName)
{
    HICON hIcon = LoadIcon(NULL, lpIconName);

    return SetIcon(hIcon);
}

BOOL CSystemTray::SetStandardIcon(UINT nIDResource)
{
    HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(nIDResource));

    return SetIcon(hIcon);
}
 
HICON CSystemTray::GetIcon() const
{
    HICON hIcon = NULL;
    if (m_bEnabled)
        hIcon = m_tnd.hIcon;

    return hIcon;
}

/////////////////////////////////////////////////////////////////////////////
// CSystemTray tooltip text manipulation

BOOL CSystemTray::SetTooltipText(LPCTSTR pszTip)
{
    if (!m_bEnabled) return FALSE;

    m_tnd.uFlags = NIF_TIP;
    _tcscpy(m_tnd.szTip, pszTip);

    return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetTooltipText(UINT nID)
{
    CString strText;
    strText.LoadString(nID);

    return SetTooltipText(strText);
}

CString CSystemTray::GetTooltipText() const
{
    CString strText;
    if (m_bEnabled)
        strText = m_tnd.szTip;

    return strText;
}

/////////////////////////////////////////////////////////////////////////////
// CSystemTray notification window stuff

BOOL CSystemTray::SetNotificationWnd(CWnd* pWnd)
{
    if (!m_bEnabled) return FALSE;

    //Make sure Notification window is valid
    ASSERT(pWnd && ::IsWindow(pWnd->GetSafeHwnd()));

    m_tnd.hWnd = pWnd->GetSafeHwnd();
    m_tnd.uFlags = 0;

    return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

CWnd* CSystemTray::GetNotificationWnd() const
{
    return CWnd::FromHandle(m_tnd.hWnd);
}

/////////////////////////////////////////////////////////////////////////////
// CSystemTray implentation of OnTrayNotification

LRESULT CSystemTray::OnTrayNotification(WPARAM wParam, LPARAM lParam) 
{
    //Return quickly if its not for this tray icon
    if (wParam != m_tnd.uID)
        return 0L;

    CMenu menu, *pSubMenu;

    // Clicking with right button brings up a context menu
	switch(LOWORD(lParam))
	{
	case WM_RBUTTONUP:
		{
			if (!menu.LoadMenu(m_tnd.uID ^ GetCurrentProcessId())) 
				return 0;
			pSubMenu = menu.GetSubMenu(0);
			if (!pSubMenu) 
				return 0;

			// Make first menu item the default (bold font)
			::SetMenuDefaultItem(pSubMenu->m_hMenu, 0, TRUE);

			//Display and track the popup menu
			CPoint pos;
			GetCursorPos(&pos);

			::SetForegroundWindow(m_tnd.hWnd);  
			::TrackPopupMenu(pSubMenu->m_hMenu, 0, pos.x, pos.y, 0, m_tnd.hWnd, NULL);

			// BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
			::PostMessage(m_tnd.hWnd, WM_NULL, 0, 0);

			menu.DestroyMenu();
		}
		break;
	case WM_LBUTTONDBLCLK:
		{
			if (!menu.LoadMenu(m_tnd.uID ^ GetCurrentProcessId())) 
				return 0;
			pSubMenu = menu.GetSubMenu(0);
			if (!pSubMenu) 
				return 0;

			// double click received, the default action is to execute first menu item
			::SetForegroundWindow(m_tnd.hWnd);
			::SendMessage(m_tnd.hWnd, WM_COMMAND, pSubMenu->GetMenuItemID(0), 0);

			menu.DestroyMenu();
		}
		break;
	case WM_LBUTTONUP:
        ::SetForegroundWindow(m_tnd.hWnd);
		break;
	}

    return 1;
}