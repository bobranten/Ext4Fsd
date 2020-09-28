#if !defined(AFX_PART_BOX_H_)
#define AFX_PART_BOX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PartBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPartBox window

class CPartBox : public CButton
{
// Construction
public:
	CPartBox();

// Operations
public:

    void SetListboxHeight(HWND hWnd);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPartBox)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPartBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPartBox)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSelectChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

    int         m_nID;
    int         m_nLeft;
    int         m_nSize;

    CComboBox   m_ComboBox;

private:

	int AssemblingTitle();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
