#if !defined(AFX_PROPERTIES_H__859C5D61_7EA7_4CC1_B69A_200D175C4ECE__INCLUDED_)
#define AFX_PROPERTIES_H__859C5D61_7EA7_4CC1_B69A_200D175C4ECE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Properties.h : header file
//

#include "DiskBox.h"
#include "PartBox.h"

/////////////////////////////////////////////////////////////////////////////
// CProperties dialog

class CProperties : public CDialog
{
// Construction
public:
	CProperties(CWnd* pParent = NULL);   // standard constructor

    BOOL m_bdisk;
    ULONG   m_type;
    PVOID   m_sdev;

// Dialog Data
	//{{AFX_DATA(CProperties)
	enum { IDD = IDD_PROPERTY_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    PEXT2_VOLUME    m_volume;
    PEXT2_CDROM     m_cdrom;
    PEXT2_DISK      m_disk;
    PEXT2_PARTITION m_part;

    CDiskBox    m_DiskBox;
    CPartBox    m_PartBox;

    CComboBox *cbDiskBox;
    CComboBox *cbPartBox;

	// Generated message map functions
	//{{AFX_MSG(CProperties)
	afx_msg void OnSdevChangeMp();
	afx_msg void OnSdevQuickMount();
	afx_msg void OnSdevExt2Info();
	virtual BOOL OnInitDialog();
    LRESULT OnGroupBoxUpdated(WPARAM wParam,LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void ResetDiskGroup();
    void ResetPartGroup();

    void SetDisk(PEXT2_DISK disk);
    void SetCdrom(PEXT2_CDROM cdrom);
    void SetVolume(PEXT2_VOLUME volume);
    void SetPartition(PEXT2_PARTITION part);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPERTIES_H__859C5D61_7EA7_4CC1_B69A_200D175C4ECE__INCLUDED_)
