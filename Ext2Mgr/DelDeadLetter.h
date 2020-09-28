#if !defined(AFX_DELDEADLETTER_H__289C3418_93AB_43C7_9E64_60C382CCF93B__INCLUDED_)
#define AFX_DELDEADLETTER_H__289C3418_93AB_43C7_9E64_60C382CCF93B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DelDeadLetter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDelDeadLetter dialog

class CDelDeadLetter : public CDialog
{
// Construction
public:
	CDelDeadLetter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDelDeadLetter)
	enum { IDD = IDD_REMOVE_DEADLETTER };
	CString		m_sDrvLetter;
	BOOL	m_bAutoRemoval;
	BOOL	m_bKeepIt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDelDeadLetter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    VOID UpdateDeadLetterList();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDelDeadLetter)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnReloadDl();
	afx_msg void OnAutoRemoval();
	afx_msg void OnAutoremovaltext();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DELDEADLETTER_H__289C3418_93AB_43C7_9E64_60C382CCF93B__INCLUDED_)
