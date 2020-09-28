#if !defined(AFX_DISK_BOX_H_)
#define AFX_DISK_BOX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiskBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiskBox window

class CDiskBox : public CButton
{
// Construction
public:
	CDiskBox();

// Operations
public:
    void SetListboxHeight(HWND);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiskBox)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDiskBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDiskBox)
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
