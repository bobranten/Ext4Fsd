#if !defined(AFX_MOUNTPOINTS_H__83141F47_96A4_4335_B42C_F2F7EB6F0ADD__INCLUDED_)
#define AFX_MOUNTPOINTS_H__83141F47_96A4_4335_B42C_F2F7EB6F0ADD__INCLUDED_

#include "SelectDrvLetter.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MountPoints.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMountPoints dialog

class CMountPoints : public CDialog
{
// Construction
public:
	CMountPoints(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMountPoints)
	enum { IDD = IDD_CHANGE_MOUNTPINT };
	CListCtrl	m_drvList;
	//}}AFX_DATA

    void InitializeList(ULONGLONG letters);

// Attributes
public: 

    PEXT2_CDROM     m_Cdrom;
    PEXT2_VOLUME    m_Volume;
    PEXT2_PARTITION m_Part;
    CString         m_Letter;
    BOOL         m_bUpdated;
    BOOL         m_bMgrNoted;

    CWnd *          m_MainDlg;

BOOL
RemoveMountPoint(CHAR drvChar);

BOOL
AddMountPoint(CHAR drvChar, BOOL bRegistry, BOOL bMountMgr);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMountPoints)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMountPoints)
	afx_msg void OnClickDrvLetterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAddMountpoint();
	afx_msg void OnChangeMountpoint();
	afx_msg void OnRemoveMountpoint();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOUNTPOINTS_H__83141F47_96A4_4335_B42C_F2F7EB6F0ADD__INCLUDED_)
