#if !defined(AFX_TREELIST_H__7C424B24_1578_47A5_8134_34901335977B__INCLUDED_)
#define AFX_TREELIST_H__7C424B24_1578_47A5_8134_34901335977B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TreeList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTreeList window

class CTreeList : public CListCtrl
{
// Construction
public:
	CTreeList();

// Attributes
public:

    /* Focus set or lost */
    BOOL     m_bFocus;

    /* single item selection */
    CRect       m_SelectionRect;
    BOOL     m_SelectionFlag;
    CPoint      m_Point;
	int		    m_Rows;
	int         m_Columns;
	int		    m_PrevItem;

    /* bitmap information */
    HBITMAP     m_hBitmap;
    HDC         m_hMemDC;
    HBITMAP     m_hOldBmp;

// Operations
public:

    VOID        Redraw();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeList)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
    afx_msg void MeasureItem ( LPMEASUREITEMSTRUCT lpMeasureItemStruct );

// Implementation
public:
	virtual ~CTreeList();
    int QuerySubItemText(int item, CHAR *Data, int length);

	// Generated message map functions
protected:
	//{{AFX_MSG(CTreeList)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREELIST_H__7C424B24_1578_47A5_8134_34901335977B__INCLUDED_)
