// SelectDrvLetter.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "SelectDrvLetter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectDrvLetter dialog


CSelectDrvLetter::CSelectDrvLetter(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectDrvLetter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectDrvLetter)
	m_DrvLetter = _T("");
	//}}AFX_DATA_INIT

	m_bMountMgr = TRUE;
	m_bRegistry = FALSE;
	m_bDosDev = FALSE;
}


void CSelectDrvLetter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDrvLetter)
	DDX_CBString(pDX, IDC_DRVLETTERS_LIST, m_DrvLetter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDrvLetter, CDialog)
	//{{AFX_MSG_MAP(CSelectDrvLetter)
	ON_BN_CLICKED(IDC_DOSDEV_MP, OnDosdevMount)
	ON_BN_CLICKED(IDC_PERMANENT_MP, OnMMgrMount)
	ON_BN_CLICKED(IDC_REGISTRY_MP, OnRegistryMount)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDrvLetter message handlers

void CSelectDrvLetter::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

    if (m_DrvLetter.IsEmpty()) {
        AfxMessageBox("You must select a drive letter.", MB_OK|MB_ICONWARNING);
        return;
    }

	CComboBox   *cbDrvLetter = (CComboBox *)GetDlgItem(IDC_DRVLETTERS_LIST);
    if (cbDrvLetter) {
        int rc = cbDrvLetter->FindStringExact(-1, m_DrvLetter);
        if (rc == CB_ERR) {
            AfxMessageBox("Invalid driver letter: "+m_DrvLetter, MB_OK|MB_ICONWARNING);
            return;
        }
    }
	CDialog::OnOK();
}

void CSelectDrvLetter::OnCancel() 
{
	// TODO: Add extra cleanup here
	m_DrvLetter.Empty();
	CDialog::OnCancel();
}

BOOL CSelectDrvLetter::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	CComboBox   *cbDrvLetter = (CComboBox *)GetDlgItem(IDC_DRVLETTERS_LIST);
    if (cbDrvLetter) {
        int i;
        CHAR drvPath[]="A:\0";
        cbDrvLetter->ResetContent();
        for (i=2; i < 26; i++) {
            if (!drvLetters[i].bUsed) {
                drvPath[0] = drvLetters[i].Letter;
                cbDrvLetter->AddString(drvPath);
            }
        }
/*
        for (i=0; i < 10; i++) {
            if (!drvDigits[i].bUsed) {
                drvPath[0] = drvDigits[i].Letter;
                cbDrvLetter->AddString(drvPath);
            }
        }
*/
        cbDrvLetter->SetCurSel(0);
    }
    UpdateData(FALSE);

    SET_CHECK(IDC_DOSDEV_MP,    m_bDosDev);
    SET_WIN(IDC_PERMANENT_MP, FALSE);
    SET_CHECK(IDC_PERMANENT_MP, 0);
    SET_CHECK(IDC_REGISTRY_MP,  m_bRegistry);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectDrvLetter::OnDosdevMount() 
{
    if (!m_bDosDev) {
	    m_bDosDev = !m_bDosDev;
        if (m_bDosDev) {
            m_bMountMgr = FALSE;
            m_bRegistry = FALSE;
        }
    }

    SET_CHECK(IDC_DOSDEV_MP,    m_bDosDev);
    SET_CHECK(IDC_PERMANENT_MP, m_bMountMgr);
    SET_CHECK(IDC_REGISTRY_MP,  m_bRegistry);

    OnOK();
}

void CSelectDrvLetter::OnMMgrMount() 
{
    if (!m_bMountMgr) {
	    m_bMountMgr = !m_bMountMgr;
        if (m_bMountMgr) {
            m_bDosDev = FALSE;
            m_bRegistry = FALSE;
        }
    }

    SET_CHECK(IDC_DOSDEV_MP,    m_bDosDev);
    SET_CHECK(IDC_PERMANENT_MP, m_bMountMgr);
    SET_CHECK(IDC_REGISTRY_MP,  m_bRegistry);

    OnOK();
}

void CSelectDrvLetter::OnRegistryMount() 
{
    if (!m_bRegistry) {
	    m_bRegistry = !m_bRegistry;
        if (m_bRegistry) {
            m_bDosDev = FALSE;
            m_bMountMgr = FALSE;
        }
    }

    SET_CHECK(IDC_DOSDEV_MP,    m_bDosDev);
    SET_CHECK(IDC_PERMANENT_MP, m_bMountMgr);
    SET_CHECK(IDC_REGISTRY_MP,  m_bRegistry);

    OnOK();
}
