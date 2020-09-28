// MountPoints.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "MountPoints.h"
#include "Ext2MgrDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMountPoints dialog


CMountPoints::CMountPoints(CWnd* pParent /*=NULL*/)
	: CDialog(CMountPoints::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMountPoints)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_Cdrom = NULL;
    m_Volume = NULL;
    m_Part = NULL;
    m_Letter = "";
    m_bUpdated = TRUE;
    m_bMgrNoted = FALSE;
    m_MainDlg = NULL;
}

void CMountPoints::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMountPoints)
	DDX_Control(pDX, IDC_DRV_LETTER_LIST, m_drvList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMountPoints, CDialog)
	//{{AFX_MSG_MAP(CMountPoints)
	ON_NOTIFY(NM_CLICK, IDC_DRV_LETTER_LIST, OnClickDrvLetterList)
	ON_BN_CLICKED(ID_ADD_MOUNTPOINT, OnAddMountpoint)
	ON_BN_CLICKED(ID_CHANGE_MOUNTPOINT, OnChangeMountpoint)
	ON_BN_CLICKED(ID_REMOVE_MOUNTPOINT, OnRemoveMountpoint)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMountPoints message handlers

void CMountPoints::OnClickDrvLetterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    int item = m_drvList.GetSelectionMark();
    if (item != -1) {
        m_Letter = m_drvList.GetItemText(item, 0);

        if (!m_Letter.IsEmpty()) {
            SET_WIN(ID_CHANGE_MOUNTPOINT, TRUE);
            SET_WIN(ID_REMOVE_MOUNTPOINT, TRUE);
        }
    }

    if (pResult) {
	    *pResult = 0;
    }
}

BOOL
CMountPoints::AddMountPoint(
    CHAR    drvChar,
    BOOL bRegistry,
    BOOL bMountMgr
    )
{
    CHAR            devPath[MAX_PATH];
    PEXT2_LETTER    drvLetter = NULL;
    ULONGLONG       letterMask = 0;
    BOOL         rc = TRUE;
    BOOL         bMount = FALSE;

    PEXT2_VOLUME_PROPERTY3   EVP = NULL;

    memset(devPath, 0, MAX_PATH);

    if (drvChar >= '0' && drvChar <= '9') {
        drvLetter = &drvDigits[drvChar - '0'];
        letterMask =  ((ULONGLONG) 1) << (drvChar - '0' + 32);
    } else if (drvChar >= 'A' && drvChar <= 'Z') {
        drvLetter = &drvLetters[drvChar - 'A'];
        letterMask =  ((ULONGLONG) 1) << (drvChar - 'A');
    }

    if (!drvLetter) {
        return FALSE;
    }

    if (m_Part) {
        if (m_Part->Volume) {
            strcpy(devPath, m_Part->Volume->Name);
        } else {
            sprintf(devPath, "\\Device\\Harddisk%u\\Partition%u",
                    m_Part->Disk->OrderNo, m_Part->Number);
        }
    }

    if (m_Volume) {
        strcpy(devPath, m_Volume->Name);
        EVP = &m_Volume->EVP;
    }

    if (m_Cdrom) {
        strcpy(devPath, m_Cdrom->Name);
        EVP = &m_Cdrom->EVP;
    }

    if (bRegistry) {
        CString str;

        if (Ext2SetRegistryMountPoint(&drvChar, devPath, bRegistry)) {
            Ext2AssignDrvLetter(drvLetter, devPath, FALSE);
            EndDialog(0);
        } else {
            str.Format("Failed to modify registry: SYSTEM\\CurrentControlSet\\Control\\Session Manager\\DOS Devices\n");
            AfxMessageBox(str, MB_OK|MB_ICONWARNING);
            return FALSE;
        }
    }

    if (drvLetter->bUsed)
        return FALSE;

    if ((m_Volume != NULL) && (m_Volume->DrvLetters == 0) &&
        (m_Volume->EVP.bExt2 || m_Volume->EVP.bExt3) ) {
        bMount = TRUE;
    } else if (m_Part != NULL && m_Part->Volume &&
          (m_Part->Volume->DrvLetters == 0) &&
          (m_Part->Volume->EVP.bExt2 || m_Part->Volume->EVP.bExt3) ) {
        EVP = &m_Part->Volume->EVP;
        bMount = TRUE;
    }

    if (EVP) {
        if (Ext2IsNullUuid(&EVP->UUID[0])) {
            AfxMessageBox("UUID is 0.");
        }
        if (!Ext2CheckVolumeRegistryProperty(EVP)) {
            Ext2SetDefaultVolumeRegistryProperty(EVP);
        }
    }

    /* create an entry in regisgtry */
    {

        NT::NTSTATUS status;
        HANDLE  Handle = NULL;
        CString str;

        status = Ext2Open(devPath, &Handle, EXT2_DESIRED_ACCESS);
        if (!NT_SUCCESS(status)) {
            str.Format("Ext2Fsd service is not started.\n");
            AfxMessageBox(str, MB_OK | MB_ICONSTOP);
            return FALSE;
        }

        rc = Ext2QueryExt2Property(Handle, EVP);
        if (!rc) {
            goto errorout;
        }

        EVP->DrvLetter = drvLetter->Letter | 0x80;
        EVP->Flags2 |= EXT2_VPROP3_AUTOMOUNT;
        Ext2StorePropertyinRegistry(EVP);

        rc = Ext2SetExt2Property(Handle, EVP);

errorout:

        Ext2Close(&Handle);
    }

    if (bMount)
    {
        rc = Ext2AssignDrvLetter(drvLetter, devPath, bMountMgr);
        if (!rc && !bMountMgr) {
            CString str;
            str.Format("Failed to assign new drive letter %c:\n", drvChar);
            AfxMessageBox(str, MB_OK|MB_ICONWARNING);
            return FALSE;
        }
    } else {
        rc = FALSE;
    }

    if (0 && bMountMgr) {

        Ext2UpdateDrvLetter(drvLetter, devPath);

        if (!bMount) {
            Ext2RefreshVolumePoint(devPath, drvLetter->Letter);
        }

        Sleep(500);
        drvChar = Ext2QueryMountPoint(devPath);

        if (drvChar >= '0' && drvChar <= '9') {
            drvLetter = &drvDigits[drvChar - '0'];
            letterMask =  ((ULONGLONG) 1) << (drvChar - '0' + 32);
        } else if (drvChar >= 'A' && drvChar <= 'Z') {
            drvLetter = &drvLetters[drvChar - 'A'];
            letterMask =  ((ULONGLONG) 1) << (drvChar - 'A');
        } else {
            drvLetter = NULL; letterMask = 0;
        }

        rc = drvLetter ? TRUE : FALSE; 
    }

    if (rc) {

        m_bUpdated = TRUE;
        if (m_Part) {
            m_Part->DrvLetters |= letterMask;
            if (m_Part->Volume) {
                m_Part->Volume->DrvLetters |= letterMask;
            }
            InitializeList(m_Part->DrvLetters);
        }
        if (m_Volume) {
            m_Volume->DrvLetters |= letterMask;
            InitializeList(m_Volume->DrvLetters);
        }
        if (m_Cdrom) {
            m_Cdrom->DrvLetters |= letterMask;
            InitializeList(m_Cdrom->DrvLetters);
        }

        /*
            ((CExt2MgrDlg *)m_MainDlg)->DriverLetterChangeNotify(drvLetter->Letter, TRUE);
        */

        m_MainDlg->SendMessage(
                    WM_MOUNTPOINT_NOTIFY,
                    'DA', (LPARAM)drvLetter->Letter);
    }

    return TRUE;
}

void CMountPoints::OnAddMountpoint() 
{
	CSelectDrvLetter drvSel;
    STORAGE_BUS_TYPE busType = BusTypeAta;

    if (m_Part) {
        busType = m_Part->Disk->SDD.BusType;
    }

    if (m_Volume && m_Volume->Part) {
        busType = m_Volume->Part->Disk->SDD.BusType;
    }

#if TRUE
    drvSel.m_bDosDev = TRUE;
    drvSel.m_bMountMgr = FALSE;
    drvSel.m_bRegistry = FALSE;
#else
    if (m_Cdrom ||
        busType == BusType1394 ||
        busType == BusTypeUsb ) {

        drvSel.m_bMountMgr = TRUE;
	    drvSel.m_bRegistry = FALSE;
	    drvSel.m_bDosDev = FALSE;
    }
#endif

    if (drvSel.DoModal() != IDOK) {
        return;
    }

    AddMountPoint(drvSel.m_DrvLetter.GetAt(0),
                  drvSel.m_bRegistry,
                  drvSel.m_bMountMgr
                );

    OnOK();
}

void CMountPoints::OnChangeMountpoint() 
{
    CHAR            odrvChar = 0;
    CHAR            ndrvChar = 0;

	CSelectDrvLetter drvSel;
    STORAGE_BUS_TYPE busType = BusTypeAta;

    if (m_Part) {
        busType = m_Part->Disk->SDD.BusType;
    }

    if (m_Volume && m_Volume->Part) {
        busType = m_Volume->Part->Disk->SDD.BusType;
    }

#if TRUE

    drvSel.m_bMountMgr = FALSE;
    drvSel.m_bRegistry = FALSE;
    drvSel.m_bDosDev = TRUE;

#else
    if (m_Cdrom ||
        busType == BusType1394 ||
        busType == BusTypeUsb ) {

        drvSel.m_bMountMgr = TRUE;
	    drvSel.m_bRegistry = FALSE;
	    drvSel.m_bDosDev = FALSE;
    }
#endif

    if (drvSel.DoModal() != IDOK) {
        return;
    }

	ndrvChar = drvSel.m_DrvLetter.GetAt(0);
	odrvChar = m_Letter.GetAt(0);

    if (RemoveMountPoint(odrvChar)) {
        AddMountPoint(
                ndrvChar,
                drvSel.m_bRegistry,
                drvSel.m_bMountMgr
            );
    }

    OnOK();
}

BOOL
CMountPoints::RemoveMountPoint(CHAR drvChar)
{
    PEXT2_LETTER    drvLetter = NULL;
    ULONGLONG       letterMask = 0;

    if (drvChar >= '0' && drvChar <= '9') {
        drvLetter = &drvDigits[drvChar - '0'];
        letterMask =  ((ULONGLONG) 1) << (drvChar - '0' + 32);
    } else if (drvChar >= 'A' && drvChar <= 'Z') {
        drvLetter = &drvLetters[drvChar - 'A'];
        letterMask =  ((ULONGLONG) 1) << (drvChar - 'A');
    }

    if (!drvLetter) {
        return FALSE;
    }

    Ext2SetRegistryMountPoint(&drvChar, NULL, FALSE);
    if (Ext2RemoveDrvLetter(drvLetter)) {

       m_MainDlg->SendMessage(WM_MOUNTPOINT_NOTIFY,
                              'DR', (LPARAM)drvLetter->Letter);

    } else {

        CString str;
        str.Format("Failed to remove drive letter %c:\n", drvChar);
        AfxMessageBox(str, MB_OK|MB_ICONWARNING);
        return FALSE;
    }

    if (m_Part) {
        m_Part->DrvLetters &= (~letterMask);
        if (m_Part->Volume) {
            m_Part->Volume->DrvLetters &= ~letterMask;
        }
        InitializeList(m_Part->DrvLetters);
    }

    if (m_Volume) {
        PEXT2_PARTITION part = Ext2QueryVolumePartition(m_Volume);
        m_Volume->DrvLetters &= ~letterMask;
        if (part) {
            part->DrvLetters &= ~letterMask;
        }
        InitializeList(m_Volume->DrvLetters);
    }

    if (m_Cdrom) {
        m_Cdrom->DrvLetters &= ~letterMask;
        InitializeList(m_Cdrom->DrvLetters);
    }

    return TRUE;
}

void CMountPoints::OnRemoveMountpoint() 
{
    CHAR drvChar = m_Letter.GetAt(0);

    if (RemoveMountPoint(drvChar)) {
        SET_WIN(ID_CHANGE_MOUNTPOINT, FALSE);
        SET_WIN(ID_REMOVE_MOUNTPOINT, FALSE);
        if (m_drvList.GetItemCount() == 0)
            OnOK();
     }
}

void CMountPoints::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CMountPoints::OnCancel() 
{
    CDialog::OnCancel();
}


void CMountPoints::InitializeList(ULONGLONG letters)
{
    CHAR        drvName[] = "C:\0";
    int         i = 0;
    ULONGLONG   drive = 0;

    m_drvList.DeleteAllItems();

    for (i=0; i < 10; i++) {
        drive = ((ULONGLONG) 1) << (i + 32);
        if (letters & drive) {
            drvName[0] = '0' + i;
            m_drvList.InsertItem(
                m_drvList.GetItemCount(),
                drvName);
        }
    }

    for (i=2; i < 26; i++) {
        drive = ((ULONGLONG) 1) << (i);
        if (letters & drive) {
            drvName[0] = 'A' + i;
            m_drvList.InsertItem(
                m_drvList.GetItemCount(),
                drvName);
        }
    }
}

BOOL CMountPoints::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
    ASSERT(m_Volume || m_Part);

    if (m_Part) {
        InitializeList(m_Part->DrvLetters);
    } else if (m_Volume) {
        InitializeList(m_Volume->DrvLetters);
    } else {
        InitializeList(m_Cdrom->DrvLetters);
    }

    m_drvList.SetSelectionMark(0);
    m_drvList.SetFocus();
    m_Letter = m_drvList.GetItemText(0, 0);

    if (m_Letter.IsEmpty()) {
        SET_WIN(ID_CHANGE_MOUNTPOINT, FALSE);
        SET_WIN(ID_REMOVE_MOUNTPOINT, FALSE);
    } else {
        SET_WIN(ID_CHANGE_MOUNTPOINT, TRUE);
        SET_WIN(ID_REMOVE_MOUNTPOINT, TRUE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMountPoints::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class

#if 0
    if (pMsg->message==WM_KEYDOWN) {
        if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE) {
            pMsg->wParam = NULL;
        }
    }
#endif
	
	return CDialog::PreTranslateMessage(pMsg);
}
