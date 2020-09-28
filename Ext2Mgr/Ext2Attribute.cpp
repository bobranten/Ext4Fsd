// Ext2Attribute.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "Ext2Attribute.h"
#include "Ext2MgrDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExt2Attribute dialog


CExt2Attribute::CExt2Attribute(CWnd* pParent /*=NULL*/)
	: CDialog(CExt2Attribute::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExt2Attribute)
	m_Codepage = _T("");
	m_bReadonly = FALSE;
	m_FixedLetter = _T("");
	m_AutoLetter = _T("");
	m_sPrefix = _T("");
	m_sSuffix = _T("");
	m_bAutoMount = FALSE;
	m_bFixMount = FALSE;
    m_autoDrv = 0;
    m_sUID = _T("----");
    m_sGID = _T("----");
    m_sEUID = _T("----");
	//}}AFX_DATA_INIT

    m_MainDlg = NULL;
    m_EVP = NULL;
    m_DevName = _T("");
    m_bCdrom = FALSE;
    m_fixDrv = 0;
}


void CExt2Attribute::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExt2Attribute)
	DDX_CBString(pDX, IDC_COMBO_CODEPAGE, m_Codepage);
	DDX_Check(pDX, IDC_READ_ONLY, m_bReadonly);
	DDX_CBString(pDX, IDC_COMBO_DRVLETTER, m_FixedLetter);
	DDX_Text(pDX, IDC_EXT2_PREFIX, m_sPrefix);
	DDV_MaxChars(pDX, m_sPrefix, 31);
	DDX_Text(pDX, IDC_EXT2_SUFFIX, m_sSuffix);
	DDV_MaxChars(pDX, m_sSuffix, 31);
	DDX_Check(pDX, IDC_AUTOMOUNT, m_bAutoMount);
	DDX_Check(pDX, IDC_FIXMOUNT, m_bFixMount);
	DDX_CBString(pDX, IDC_COMBO_AUTOMP, m_AutoLetter);
    DDX_Text(pDX, IDC_EDIT_UID, m_sUID);
	DDV_MaxChars(pDX, m_sUID, 8);
    DDX_Text(pDX, IDC_EDIT_GID, m_sGID);
	DDV_MaxChars(pDX, m_sGID, 8);
    DDX_Text(pDX, IDC_EDIT_EUID, m_sEUID);
	DDV_MaxChars(pDX, m_sEUID, 8);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CExt2Attribute, CDialog)
	//{{AFX_MSG_MAP(CExt2Attribute)
	ON_BN_CLICKED(IDC_AUTOMOUNT, OnAutomount)
	ON_BN_CLICKED(IDC_FIXMOUNT, OnFixmount)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExt2Attribute message handlers

BOOL CExt2Attribute::OnInitDialog() 
{
    int i = 0;
    CString s;

	CDialog::OnInitDialog();
	
	m_Codepage = m_EVP->Codepage;
	m_bReadonly = m_EVP->bReadonly || m_bCdrom;
    if (m_bCdrom) {
        SET_WIN(IDC_READ_ONLY, FALSE);
    }

    if (m_bCdrom) {
        m_fixDrv = Ext2QueryMountPoint(
                        m_DevName.GetBuffer(MAX_PATH));
    } else {
        m_fixDrv = Ext2QueryRegistryMountPoint(
                        m_DevName.GetBuffer(MAX_PATH));
    }

    if (m_fixDrv) {
        m_bAutoMount = FALSE;
        m_bFixMount = TRUE;
        m_FixedLetter.Format("%c:", m_fixDrv);
    } else {
        m_FixedLetter = "  ";
    }

    if (m_EVP->Flags2 & EXT2_VPROP3_USERIDS) {
        m_sUID.Format("%u", m_EVP->uid);
        m_sGID.Format("%u", m_EVP->gid);
        if (m_EVP->EIDS) {
            m_sEUID.Format("%u", m_EVP->euid);
        }
    }

    if (m_EVP->DrvLetter) {

        m_autoDrv = m_EVP->DrvLetter & 0x7F;
        if (m_autoDrv >= 'a' && m_autoDrv <= 'z')
            m_autoDrv = (CHAR)toupper(m_autoDrv);
        else if (m_autoDrv >= 'A' && m_autoDrv <= 'Z') {
        } else {
            m_autoDrv = 0;
        }
        if (m_autoDrv == 0)
            m_AutoLetter.Format("  ");
        else
            m_AutoLetter.Format("%C:", m_autoDrv);
        m_bAutoMount = !m_bFixMount;
    }

	m_sPrefix = m_EVP->sHidingPrefix;
	m_sSuffix = m_EVP->sHidingSuffix;

    CComboBox   *cbCodepage = (CComboBox *)GetDlgItem(IDC_COMBO_CODEPAGE);
    if (cbCodepage) {
        cbCodepage->ResetContent();
		i = 0;
        while (gCodepages[i]) {
            cbCodepage->AddString(gCodepages[i++]);
        }
    }

    {
	    CComboBox   *cbDrvLetter = (CComboBox *)GetDlgItem(IDC_COMBO_DRVLETTER);
        CComboBox   *cbAutoLetter = (CComboBox *)GetDlgItem(IDC_COMBO_AUTOMP);

        PEXT2_LETTER drvLetter = NULL;
        cbDrvLetter->AddString("  ");
        cbAutoLetter->AddString("  ");

        if (m_autoDrv) {
            m_AutoLetter.Format("%C:", m_autoDrv);
            cbAutoLetter->AddString(m_AutoLetter);
        }

        if (m_fixDrv) {
            cbDrvLetter->AddString(m_FixedLetter);
        }
        for (i=2; i < 26; i++) {
            drvLetter = &drvLetters[i];
            if (!drvLetter->bUsed) {
                s.Format("%c:", drvLetter->Letter);
                cbDrvLetter->AddString(s);
                cbAutoLetter->AddString(s);
            }        
        }
/*
        for (i=0; i < 10; i++) {
            drvLetter = &drvDigits[i];
            if (!drvLetter->bUsed) {
                s.Format("%c:", drvLetter->Letter);
                cbDrvLetter->AddString(s);
            }        
        }
*/

    }

    SET_CHECK(IDC_AUTOMOUNT, m_bAutoMount);
    SET_CHECK(IDC_FIXMOUNT,  m_bFixMount);

    SET_WIN(IDC_COMBO_DRVLETTER, m_bFixMount);
    SET_WIN(IDC_COMBO_AUTOMP, m_bAutoMount);

    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExt2Attribute::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

BOOL IsStringAllDigits(CString s)
{
    for (int i = 0; i < s.GetLength(); i++) {
        if (!isdigit(s.GetAt(i)))
            return FALSE;
    }

    return TRUE;
}


void CExt2Attribute::OnOK() 
{
    NT::NTSTATUS status;
    HANDLE  Handle = NULL;
	CHAR    DrvLetter = 0;
    CString str;
    BOOL rc = FALSE;
    BOOL dc = FALSE;

    UpdateData(TRUE);

    if (m_Codepage.IsEmpty()) {
        m_Codepage = "default";
    }

    CComboBox *cbCodepage = (CComboBox *)GetDlgItem(IDC_COMBO_CODEPAGE);
    if (cbCodepage) {
        int rc = cbCodepage->FindStringExact(-1, m_Codepage);
        if (rc == CB_ERR) {
            AfxMessageBox("Invalid codepage type: "+m_Codepage, MB_OK|MB_ICONWARNING);
            return;
        }
    }

    if (m_EVP->bReadonly && m_EVP->bExt3 && !m_bReadonly) {
        str = "Are you sure to enable writing support"
              " on this EXT3 volume with Ext2Fsd ?";
        if (AfxMessageBox(str, MB_YESNO) != IDYES) {
            m_EVP->bExt3Writable = FALSE;
        } else {
            m_EVP->bExt3Writable = TRUE;
        }
    }

    /* initialize structures to communicate with ext2fsd service*/
    m_EVP->Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    m_EVP->Command = APP_CMD_SET_PROPERTY3;
    m_EVP->Flags = 0;
    m_EVP->Flags2 = 0;
    m_EVP->bReadonly = (BOOLEAN)m_bReadonly;
    memset(m_EVP->Codepage, 0, CODEPAGE_MAXLEN);
    strcpy((CHAR *)m_EVP->Codepage, m_Codepage.GetBuffer(CODEPAGE_MAXLEN));

    /* initialize hiding filter patterns */
    if (m_sPrefix.IsEmpty()) {
        m_EVP->bHidingPrefix = FALSE;
        memset(m_EVP->sHidingPrefix, 0, HIDINGPAT_LEN);
    } else {
        strcpy( m_EVP->sHidingPrefix, 
                m_sPrefix.GetBuffer(m_sPrefix.GetLength()));
        m_EVP->bHidingPrefix = TRUE;
    }

    if (m_sSuffix.IsEmpty()) {
        m_EVP->bHidingSuffix = FALSE;
        memset(m_EVP->sHidingSuffix, 0, HIDINGPAT_LEN);
    } else {
        strcpy(m_EVP->sHidingSuffix, 
            m_sSuffix.GetBuffer(m_sSuffix.GetLength()));
        m_EVP->bHidingSuffix = TRUE;
    }

    if (m_bFixMount) {

        if (!m_FixedLetter.IsEmpty() && m_FixedLetter.GetAt(0) != ' ') {
            DrvLetter = m_FixedLetter.GetAt(0);
        } else {
            DrvLetter = 0;
        }

        if (DrvLetter != m_fixDrv) {

            if (m_fixDrv != 0) {
                Ext2SetRegistryMountPoint(&m_fixDrv, NULL, FALSE);
                Ext2RemoveDriveLetter(DrvLetter);
            }

            if (DrvLetter != 0) {
                Ext2SetRegistryMountPoint(&DrvLetter, m_DevName.GetBuffer(MAX_PATH), TRUE);
            }
        }

        if (DrvLetter && !m_fixDrv) {
            Ext2MountVolumeAs(m_DevName.GetBuffer(MAX_PATH), DrvLetter);
            dc = TRUE;
        }
    } else {

        if (m_fixDrv != 0) {
            Ext2RemoveDriveLetter(m_fixDrv);
            Ext2SetRegistryMountPoint(&m_fixDrv, NULL, FALSE);
        }
    }

    if (!m_sUID.IsEmpty() && !m_sGID.IsEmpty()) {
        if (IsStringAllDigits(m_sUID) &&
            IsStringAllDigits(m_sGID)) {
            m_EVP->Flags2 |= EXT2_VPROP3_USERIDS;
            m_EVP->uid = (USHORT)atoi(m_sUID.GetBuffer(8));
            m_EVP->gid = (USHORT)atoi(m_sGID.GetBuffer(8));
            if (!m_sEUID.IsEmpty() &&
                IsStringAllDigits(m_sEUID)) {
                m_EVP->EIDS = TRUE;
                m_EVP->euid = (USHORT)atoi(m_sEUID.GetBuffer(8));
            }
        }
    }

    if (m_bAutoMount) {

        if (m_bCdrom) {
            m_EVP->DrvLetter = 0;
            Ext2StorePropertyinRegistry(m_EVP);
            goto store_evp;
        }

        if (!m_AutoLetter.IsEmpty() && m_AutoLetter.GetAt(0) != ' ') {
            DrvLetter = m_AutoLetter.GetAt(0);
        } else {
            DrvLetter = 0;
        }

        if (DrvLetter > 'Z' || DrvLetter < 'A') {
            DrvLetter = 0;
        }
        m_EVP->DrvLetter = DrvLetter | 0x80;
        m_EVP->Flags2 |= EXT2_VPROP3_AUTOMOUNT;
        Ext2StorePropertyinRegistry(m_EVP);

        if (m_autoDrv) {
            if (DrvLetter && Ext2IsDrvLetterAvailable(DrvLetter)) {
                Ext2RemoveDriveLetter(m_autoDrv);
                Ext2MountVolumeAs(m_DevName.GetBuffer(MAX_PATH), DrvLetter);
                dc = TRUE;
            }
        } else {
            if (DrvLetter && Ext2IsDrvLetterAvailable(DrvLetter)) {
                Ext2MountVolumeAs(m_DevName.GetBuffer(MAX_PATH), DrvLetter);
                dc = TRUE;
            } else {
                Ext2MountVolume(m_DevName.GetBuffer(MAX_PATH));
                dc = TRUE;
            }
        }

    } else {

        m_EVP->DrvLetter =0 ;
        Ext2StorePropertyinRegistry(m_EVP);

        if (m_autoDrv && !Ext2IsDrvLetterAvailable(m_autoDrv)) {
            Ext2RemoveDriveLetter(m_autoDrv);
            dc = TRUE;
        }
    }

store_evp:

    status = Ext2Open(m_DevName.GetBuffer(m_DevName.GetLength()),
                      &Handle, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        str.Format("Ext2Fsd service is not started.\n");
        AfxMessageBox(str, MB_OK | MB_ICONSTOP);
        rc = TRUE;
        goto errorout;
    }

    rc = Ext2SetExt2Property(Handle, m_EVP);

    if (rc) {
        /* don't bother user at all */
#if 0
        str = "Ext2 volume settings updated successfully!";
        if (dc) {
            str += "\r\n\r\nFixed mountpoint needs reboot to take into affect.";
        }
        AfxMessageBox(str, MB_OK | MB_ICONINFORMATION);
#endif
    } else {
        AfxMessageBox("Failed to save the Ext2 settings !",
                      MB_OK | MB_ICONWARNING);
    }

errorout:

    Ext2Close(&Handle);

    if (rc) {
      	CDialog::OnOK();
    }
}

void CExt2Attribute::OnAutomount() 
{
	UpdateData(TRUE);
    SET_WIN(IDC_COMBO_AUTOMP, m_bAutoMount);
/*
    if (m_bAutoMount) {
        AfxMessageBox("This function is still in experiment. You'd better set a\r\n"
                   "fixed mountpoint for fixed disk or set partition type to\r\n"
                   "0x07 (NTFS) or FAT for non-bootable partition. For removable\r\n"
                   "disks like usb-disk it's better to use the second method.");
    }
*/

    if (m_bAutoMount) {
/*
        CComboBox *cbAutoLetter = (CComboBox *)GetDlgItem(IDC_COMBO_AUTOMP);
        if (cbAutoLetter && cbAutoLetter->GetCurSel() == CB_ERR) {
            cbAutoLetter->SetCurSel(1);
        }
*/
        m_bFixMount = FALSE;
        SET_CHECK(IDC_FIXMOUNT,  FALSE);
        SET_WIN(IDC_COMBO_DRVLETTER, FALSE);

        UpdateData(FALSE);
    }
}

void CExt2Attribute::OnFixmount() 
{
    UpdateData(TRUE);
    SET_WIN(IDC_COMBO_DRVLETTER, m_bFixMount);

    if (m_bFixMount) {
/*
	    CComboBox *cbDrvLetter = (CComboBox *)GetDlgItem(IDC_COMBO_DRVLETTER);
        if (cbDrvLetter && cbDrvLetter->GetCurSel() == CB_ERR) {
            cbDrvLetter->SetCurSel(1);
        }
*/
        m_bAutoMount = FALSE;
        SET_CHECK(IDC_AUTOMOUNT, FALSE);
        SET_WIN(IDC_COMBO_AUTOMP, FALSE);

        UpdateData(FALSE);
    }
}