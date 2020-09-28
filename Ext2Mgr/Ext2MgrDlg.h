// Ext2MgrDlg.h : header file
//

#if !defined(AFX_EXT2MGRDLG_H__EACC693E_C531_48BA_A0FD_4AFB090CCB29__INCLUDED_)
#define AFX_EXT2MGRDLG_H__EACC693E_C531_48BA_A0FD_4AFB090CCB29__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SysTray.h"
#include "Toolbar.h"
#include "splash.h"
#include "donate.h"
#include "TreeList.h"
#include "HyperLink.h"
#include "Mountpoints.h"
#include "Properties.h"
#include "ServiceManage.h"
#include "Ext2Attribute.h"
#include "PerfStatDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CExt2List window

class CExt2List : public CListCtrl
{
// Construction
public:
	CExt2List();

// Attributes
public:

    CPoint m_Point;

// Operations
public:

    int QuerySubItemText(int item, CHAR *Data, int length);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExt2List)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CExt2List();

	// Generated message map functions
protected:
	//{{AFX_MSG(CExt2List)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrDlg dialog

class CDlgView;

class CExt2MgrDlg : public CDialog
{
// Construction
public:
	CExt2MgrDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CExt2MgrDlg)
	enum { IDD = IDD_EXT2MGR_DIALOG };
	CTreeList	m_DiskView;
	CExt2List	m_VolumeList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExt2MgrDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

public:

    /* routines */
    PVOID QuerySelectedItem(PBOOL bIsExt2);
    VOID  DriversChangeNotify(ULONG, BOOL);
    VOID  DriverLetterChangeNotify(CHAR, BOOL);

    VOID  DriverChangeNotify(PEXT2_LETTER, BOOL);

    VOID  UpdateVolume(PEXT2_VOLUME volume);
    VOID  UpdateCdrom(PEXT2_CDROM cdrom) ;
    VOID  UpdatePartition(PEXT2_PARTITION part);
    VOID  RegisterDeviceInterface(GUID, PHDEVNOTIFY);
    VOID  OnFlush();
    VOID  OnPartType();
    VOID  OnRemoveDeadLetter();

    VOID OnKeyupVolumeList();
    VOID OnKeyupDiskList();

    ULONG   m_type;
    PVOID   m_sdev;

    /* attributes */
    CSplash  *  m_splash;
    CMenu       m_Menu;

    CFont    m_MSSanS;
    BOOL     m_bHide;
    BOOL     m_bQuiet;
    BOOL     m_bService;
    BOOL     m_bStat;

    CPerfStatDlg * m_PerfDlg;

// Implementation
protected:

	HICON       m_hIcon;

	CSystemTray  m_Tray;
    CStatusBar   m_bar;
    CImageList   m_ImageList;

    BOOL      m_bFsStarted;
    BOOL      m_bHandleChange;
    BOOL      m_bFocusVolume;
    BOOL      m_bFocusDisk;
    LONG         m_IndexVolume;
    LONG         m_IndexDisk;

    HACCEL       m_hAccel;
    HDEVNOTIFY   m_hUsbNotify;

    /* global parameters */
    ULONG   m_nStartmode;
	CString	m_Codepage;
	BOOL	m_bExt3Writable;
	BOOL	m_bReadonly;
	CString	m_srvStatus;
	CString	m_sPrefix;
	CString	m_sSuffix;
	BOOL	m_bAutoMount;

	// Generated message map functions
	//{{AFX_MSG(CExt2MgrDlg)
	virtual BOOL OnInitDialog();
    virtual void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnChangeProperty();
	afx_msg void OnRefresh();
	afx_msg void OnFormat();
	afx_msg void OnService();
	virtual void OnCancel();
	afx_msg void OnAbout();
	afx_msg void OnExit();
	virtual void OnOK();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDblclkDiskList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusDiskList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickDiskList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickDiskList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusDiskList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProperty();
	afx_msg void OnDonate();
	afx_msg void OnCopy();
	afx_msg void OnInstallService();
	afx_msg void OnRemoveService();
    afx_msg void OnEnableAutorun(); 
    afx_msg void OnDisableAutorun();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDrvLetter();
	afx_msg void OnDrvQuickMount();
	afx_msg void OnShowMain();
	afx_msg void OnHelp();
	afx_msg void OnPerfStat();
	afx_msg void OnPerfStop();
	afx_msg void OnCopyAll();
	//}}AFX_MSG
	afx_msg LRESULT OnTrayNotification(WPARAM wParam=0,LPARAM lParam=0);
	afx_msg LRESULT OnTerminate(WPARAM wParam=0,LPARAM lParam=0);
	afx_msg LRESULT OnMountPointNotify(WPARAM wParam=0,LPARAM lParam=0);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXT2MGRDLG_H__EACC693E_C531_48BA_A0FD_4AFB090CCB29__INCLUDED_)
