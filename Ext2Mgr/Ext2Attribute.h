#if !defined(AFX_EXT2ATTRIBUTE_H__59ED803F_08E4_4ADD_BDAE_E86EF7708DEF__INCLUDED_)
#define AFX_EXT2ATTRIBUTE_H__59ED803F_08E4_4ADD_BDAE_E86EF7708DEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Ext2Attribute.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExt2Attribute dialog

class CExt2Attribute : public CDialog
{
// Construction
public:
	CExt2Attribute(CWnd* pParent = NULL);   // standard constructor

    CWnd *  m_MainDlg;
    PEXT2_VOLUME_PROPERTY3 m_EVP;
    CString m_DevName;
    BOOL m_bCdrom;
    CHAR    m_autoDrv;
    CHAR    m_fixDrv;

// Dialog Data
	//{{AFX_DATA(CExt2Attribute)
	enum { IDD = IDD_EXT2_ATTR };
	CString	m_Codepage;
	BOOL	m_bReadonly;
	CString	m_FixedLetter;
	CString	m_sPrefix;
	CString	m_sSuffix;
	BOOL	m_bAutoMount;
	BOOL	m_bFixMount;
	CString	m_AutoLetter;
    CString m_sUID;
    CString m_sGID;
    CString m_sEUID;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExt2Attribute)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExt2Attribute)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnAutomount();
	afx_msg void OnFixmount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXT2ATTRIBUTE_H__59ED803F_08E4_4ADD_BDAE_E86EF7708DEF__INCLUDED_)
