// ServiceManage.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "ServiceManage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceManage dialog


CServiceManage::CServiceManage(CWnd* pParent /*=NULL*/)
	: CDialog(CServiceManage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceManage)
	m_Codepage = _T("");
	m_bExt3Writable = FALSE;
	m_bReadonly = FALSE;
	m_srvStatus = _T("");
	m_sPrefix = _T("");
	m_sSuffix = _T("");
	m_bAutoMount = FALSE;
	//}}AFX_DATA_INIT

    m_nStartmode = 0;

    m_bStarted = Ext2IsServiceStarted();

    if (m_bStarted) {
        m_srvStatus = _T("Ext2Fsd is already started.");
    } else {
        m_srvStatus = _T("Ext2Fsd is NOT started.");
    }

    m_bInited = TRUE;
}


void CServiceManage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceManage)
	DDX_CBString(pDX, IDC_COMBO_CODEPAGE, m_Codepage);
	DDX_Check(pDX, IDC_EXT3_WRITABLE, m_bExt3Writable);
	DDX_Check(pDX, IDC_READ_ONLY, m_bReadonly);
	DDX_Text(pDX, IDC_SERVICE_STATUS, m_srvStatus);
	DDX_Text(pDX, IDC_GLOBAL_PREFIX, m_sPrefix);
	DDV_MaxChars(pDX, m_sPrefix, 31);
	DDX_Text(pDX, IDC_GLOBAL_SUFFIX, m_sSuffix);
	DDV_MaxChars(pDX, m_sSuffix, 31);
	DDX_Check(pDX, IDC_EXT3_AUTOMOUNT, m_bAutoMount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceManage, CDialog)
	//{{AFX_MSG_MAP(CServiceManage)
	ON_BN_CLICKED(IDC_READ_ONLY, OnReadOnly)
	ON_BN_CLICKED(IDC_EXT3_WRITABLE, OnExt3Writable)
	ON_BN_CLICKED(IDC_START_SERVICE, OnStartService)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceManage message handlers

void CServiceManage::OnReadOnly() 
{
	// TODO: Add your control notification handler code here
    UpdateData(TRUE);

    if (m_bReadonly) {
        m_bExt3Writable = FALSE;
    }
    SET_WIN(IDC_EXT3_WRITABLE, !m_bReadonly);

    UpdateData(FALSE);
}

void CServiceManage::OnExt3Writable() 
{
	// TODO: Add your control notification handler code here
	
}

void CServiceManage::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CServiceManage::OnOK() 
{
    BOOL rc;

	// TODO: Add extra validation here
    UpdateData(TRUE);

	CComboBox   *cbStartup = (CComboBox *)GetDlgItem(IDC_COMBO_STARTUP);
    if (cbStartup) {
        m_nStartmode = cbStartup->GetCurSel();
    } else {
        m_nStartmode = CB_ERR;
    }

    if (m_nStartmode == CB_ERR) {
        AfxMessageBox("Invalid startup mode !", MB_OK|MB_ICONWARNING);
        return;
    }

    if (m_Codepage.IsEmpty()) {
        AfxMessageBox("You must select a codepage type.", MB_OK|MB_ICONWARNING);
        return;
    }

    CComboBox   *cbCodepage = (CComboBox *)GetDlgItem(IDC_COMBO_CODEPAGE);
    if (cbCodepage) {
        int rc = cbCodepage->FindStringExact(-1, m_Codepage);
        if (rc == CB_ERR) {
            AfxMessageBox("Invalid codepage type: "+m_Codepage, MB_OK|MB_ICONWARNING);
            return;
        }
    }

    if (AfxMessageBox("Current service settings will be overwritten,\ndo you want continue ?",
                      MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    rc = Ext2SetGlobalProperty(
            m_nStartmode,
            (BOOLEAN)m_bReadonly,
            (BOOLEAN)m_bExt3Writable,
            m_Codepage.GetBuffer(CODEPAGE_MAXLEN),
            m_sPrefix.GetBuffer(HIDINGPAT_LEN),
            m_sSuffix.GetBuffer(HIDINGPAT_LEN),
            (BOOLEAN)m_bAutoMount
            );

    if (rc) {
/*
        AfxMessageBox("Ext2 service settings updated successfully !",
                      MB_OK | MB_ICONINFORMATION);
*/
      	CDialog::OnOK();
    } else {
        AfxMessageBox("Failed to save the service settings !",
                      MB_OK | MB_ICONWARNING);
    }
}

BOOL CServiceManage::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CComboBox   *cbStartup = (CComboBox *)GetDlgItem(IDC_COMBO_STARTUP);
    if (cbStartup) {
        cbStartup->ResetContent();
        cbStartup->AddString(_T("SERVICE_BOOT_START"));
        cbStartup->AddString(_T("SERVICE_SYSTEM_START"));
        cbStartup->AddString(_T("SERVICE_AUTO_START"));
        cbStartup->AddString(_T("SERVICE_DEMAND_START"));
        cbStartup->AddString(_T("SERVICE_DISABLED"));
        cbStartup->SetCurSel(m_nStartmode);
    }

    CComboBox   *cbCodepage = (CComboBox *)GetDlgItem(IDC_COMBO_CODEPAGE);
    if (cbCodepage) {
        int i = 0, j = -1;
        cbCodepage->ResetContent();
        while (gCodepages[i]) {
            cbCodepage->AddString(gCodepages[i]);
            if (!m_Codepage.IsEmpty()) {
                CHAR *buffer = m_Codepage.GetBuffer(CODEPAGE_MAXLEN);
                if (_stricmp(buffer, gCodepages[i]) == 0) {
                    j = i;
                }
            }
            i++;
        }
        if (j == -1) {
            m_Codepage = "default";
        }
    }

    if (m_bReadonly) {
        m_bExt3Writable = FALSE;
    }

    SET_WIN(IDC_EXT3_WRITABLE, !m_bReadonly);
    SET_WIN(IDC_START_SERVICE, !m_bStarted);
	
    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServiceManage::OnStartService() 
{
	// TODO: Add your control notification handler code here

    m_bStarted = Ext2StartExt2Fsd();

    if (m_bStarted) {
        m_srvStatus = _T("Ext2Fsd was just started.");
        GetParent()->SetTimer('REFF',  500, NULL);
        GetParent()->SetTimer('REFR', 3000, NULL);
    } else {
        m_srvStatus = _T("Ext2Fsd could NOT be started.");
    }

    SET_WIN(IDC_START_SERVICE, !m_bStarted);
    UpdateData(FALSE);
}
