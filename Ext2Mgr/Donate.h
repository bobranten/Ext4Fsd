#if !defined(AFX_DONATE_H__40066CB2_EB28_4B9A_B51A_6CF889E4D5CB__INCLUDED_)
#define AFX_DONATE_H__40066CB2_EB28_4B9A_B51A_6CF889E4D5CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Donate.h : header file
//
#include "HyperLink.h"

/////////////////////////////////////////////////////////////////////////////
// CDonate dialog

class CDonate : public CDialog
{
// Construction
public:
	CDonate(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDonate)
	enum { IDD = IDD_DONATE_DIALOG };
	CMyHyperLink	m_SF;
	CMyHyperLink	m_Paypal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDonate)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDonate)
	virtual BOOL OnInitDialog();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DONATE_H__40066CB2_EB28_4B9A_B51A_6CF889E4D5CB__INCLUDED_)
