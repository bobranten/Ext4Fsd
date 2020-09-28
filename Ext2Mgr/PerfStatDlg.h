#if !defined(AFX_PERFSTATDLG_H__E75F834E_0A81_4B82_BA17_D204EA1EC094__INCLUDED_)
#define AFX_PERFSTATDLG_H__E75F834E_0A81_4B82_BA17_D204EA1EC094__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PerfStatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPerfStatDlg dialog

class CPerfStatDlg : public CDialog
{
// Construction
public:
	CPerfStatDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPerfStatDlg)
	enum { IDD = IDD_PERFSTAT_DIALOG };
	int		m_Interval;
	//}}AFX_DATA

	CListCtrl *         m_IrpList;
	CListCtrl *         m_MemList;

    HANDLE               m_Handle;
    EXT2_QUERY_PERFSTAT  m_PerfStat;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerfStatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPerfStatDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnChangePerfstatInterval();
	virtual void OnOK();
	virtual void OnCancel();

    afx_msg void OnQueryPerf();

    void RefreshPerfStat();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFSTATDLG_H__E75F834E_0A81_4B82_BA17_D204EA1EC094__INCLUDED_)
