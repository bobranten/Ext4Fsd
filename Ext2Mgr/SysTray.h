/////////////////////////////////////////////////////////////////////////////
// SystemTray.h : header file 
//
// Written by Chris Maunder (Chris.Maunder@cbr.clw.csiro.au)
// Copyright (c) 1998.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. If 
// the source code in  this file is used in any commercial application 
// then acknowledgement must be made to the author of this file 
// (in whatever form you wish).
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to your
// computer, causes your pet cat to fall ill, increases baldness or
// makes you car start emitting strange noises when you start it up.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 

#ifndef _INCLUDED_SYSTEMTRAY_H_
#define _INCLUDED_SYSTEMTRAY_H_

/////////////////////////////////////////////////////////////////////////////
// CSystemTray window

class CSystemTray : public CObject
{
// Construction/destruction
public:
    CSystemTray();
    CSystemTray(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);
    virtual ~CSystemTray();

// Operations
public:
    BOOL Enabled() { return m_bEnabled; }
    BOOL Visible() { return !m_bHidden; }

    //Create the tray icon
    int Create(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);

    //Change or retrieve the Tooltip text
    BOOL    SetTooltipText(LPCTSTR pszTooltipText);
    BOOL    SetTooltipText(UINT nID);
    CString GetTooltipText() const;

    //Change or retrieve the icon displayed
    BOOL  SetIcon(HICON hIcon);
    BOOL  SetIcon(LPCTSTR lpIconName);
    BOOL  SetIcon(UINT nIDResource);
    BOOL  SetStandardIcon(LPCTSTR lpIconName);
    BOOL  SetStandardIcon(UINT nIDResource);
    HICON GetIcon() const;
    void  HideIcon();
    void  ShowIcon();
    void  RemoveIcon();
    void  MoveToRight();

    //Change or retrieve the window to send notification messages to
    BOOL  SetNotificationWnd(CWnd* pNotifyWnd);
    CWnd* GetNotificationWnd() const;

    //Default handler for tray notification message
    virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSystemTray)
    //}}AFX_VIRTUAL

// Implementation
protected:
    BOOL            m_bEnabled;   // does O/S support tray icon?
    BOOL            m_bHidden;    // Has the icon been hidden?
    NOTIFYICONDATA  m_tnd;

    DECLARE_DYNAMIC(CSystemTray)
};


#endif

/////////////////////////////////////////////////////////////////////////////
