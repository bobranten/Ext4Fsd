#if !defined(AFX_DLGVIEW1_H__5E7226FD_BC8E_4B9C_9769_80E378804A93__INCLUDED_)
#define AFX_DLGVIEW1_H__5E7226FD_BC8E_4B9C_9769_80E378804A93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgView view

class CDlgView : public CScrollView
{
protected:
	CDlgView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDlgView)

// Attributes
public:

// Operations
public:
   CString m_csText;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDlgView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CDlgView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGVIEW1_H__5E7226FD_BC8E_4B9C_9769_80E378804A93__INCLUDED_)
