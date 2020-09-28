#if !defined(AFX_PARTITIONTYPE_H__7BBDDABB_64CE_4448_92DA_D57973E423A5__INCLUDED_)
#define AFX_PARTITIONTYPE_H__7BBDDABB_64CE_4448_92DA_D57973E423A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PartitionType.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPartitionType dialog

class CPartitionType : public CDialog
{
// Construction
public:
	CPartitionType(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPartitionType)
	enum { IDD = IDD_PARTITION_TYPE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPartitionType)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

    CString         m_sPartType;

public:

    PEXT2_PARTITION m_Part;
    UCHAR           m_cPartType;
    CString         m_sDevice;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPartitionType)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARTITIONTYPE_H__7BBDDABB_64CE_4448_92DA_D57973E423A5__INCLUDED_)
