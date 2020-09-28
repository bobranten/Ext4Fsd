#if !defined(AFX_SELECTDRVLETTER_H__8FE3FD0E_B375_4956_B36F_733A7EEF2892__INCLUDED_)
#define AFX_SELECTDRVLETTER_H__8FE3FD0E_B375_4956_B36F_733A7EEF2892__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectDrvLetter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectDrvLetter dialog

class CSelectDrvLetter : public CDialog
{
// Construction
public:
	CSelectDrvLetter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectDrvLetter)
	enum { IDD = IDD_NEW_MOUNTPOINT };
	CString	m_DrvLetter;
	//}}AFX_DATA

	BOOL m_bMountMgr;
	BOOL m_bRegistry;
	BOOL m_bDosDev;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectDrvLetter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectDrvLetter)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnDosdevMount();
	afx_msg void OnMMgrMount();
	afx_msg void OnRegistryMount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTDRVLETTER_H__8FE3FD0E_B375_4956_B36F_733A7EEF2892__INCLUDED_)
