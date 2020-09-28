////////////////////////////////////////////////////////
// Class Name : CMyHyperLink
// Written By : Renjith.R
// Email : renjith_sree@hotmail.com
// Details :Derived from MFC CStatic 
// Date :Nov 25 2002
// This can be used as a Hyperlink 
//Feel free to use this class in your project

///////////////////////////////////////////////////////////


#if !defined(AFX_MYHYPERLINK_H__699B2FB4_0C03_4B12_B117_210A97860E0D__INCLUDED_)
#define AFX_MYHYPERLINK_H__699B2FB4_0C03_4B12_B117_210A97860E0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MyHyperLink.h : header file
//

//This is the EventID , Which Will  send to the Parent
//by the hyperlink  control

# define _HYPERLINK_EVENT WM_USER + 101 

/////////////////////////////////////////////////////////////////////////////
// CMyHyperLink window

class CMyHyperLink : public CStatic
{
// Construction
public:
	CMyHyperLink();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyHyperLink)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PreSubclassWindow();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetFireChild(int nFlag);
	CString GetLinkText();
	CString GetLinkUrl();
	
	bool GoToLinkUrl(CString csLink);
	
	void SetHoverColor(COLORREF cHoverColor);
	void SetVisitedColor(COLORREF sVisitedColor);
	void SetLinkUrl(CString csUrl);
	void SetToolTipBgColor(COLORREF sToolTipBgColor);
	void SetToolTipTextColor(COLORREF sToolTipText);
	void SetLinkText(CString csLinkText);
	void SetTootTipText(LPCSTR szToolTip);
	void ActiveToolTip(int nFlag);
	void SetLinkColor(COLORREF sLinkColor);
	
	virtual ~CMyHyperLink();

	// Generated message map functions
protected:
	bool m_bFireChild;
	
	HCURSOR m_hHyperCursor;	
	
	bool m_bEnableToolTip;
	bool m_bMouseOver;
	bool m_bVisited;

	CFont m_oTextFont;
	CToolTipCtrl m_ToolTip;

	CString m_csToolTipText;
	CString m_csLinkText;
	CString m_csUrl;

	COLORREF m_sHoverColor;
	COLORREF m_sLinkColor;
	COLORREF m_sVisitedColor;

	//{{AFX_MSG(CMyHyperLink)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnClicked();
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYHYPERLINK_H__699B2FB4_0C03_4B12_B117_210A97860E0D__INCLUDED_)
