#if !defined(AFX_SERVICEMANAGE_H__1B7348C0_0EC3_43D1_8E39_25D5F1113B8E__INCLUDED_)
#define AFX_SERVICEMANAGE_H__1B7348C0_0EC3_43D1_8E39_25D5F1113B8E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServiceManage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServiceManage dialog

class CServiceManage : public CDialog
{
// Construction
public:
	CServiceManage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceManage)
	enum { IDD = IDD_SERVICE_MANAGE };
	CString	m_Codepage;
	BOOL	m_bExt3Writable;
	BOOL	m_bReadonly;
	CString	m_srvStatus;
	CString	m_sPrefix;
	CString	m_sSuffix;
	BOOL	m_bAutoMount;
	//}}AFX_DATA
    ULONG   m_nStartmode;
    BOOL    m_bInited;
    BOOL    m_bStarted;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceManage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServiceManage)
	afx_msg void OnReadOnly();
	afx_msg void OnExt3Writable();
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnStartService();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICEMANAGE_H__1B7348C0_0EC3_43D1_8E39_25D5F1113B8E__INCLUDED_)
