// Properties.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "Ext2MgrDlg.h"
#include "Properties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SelectDrvLetter.h"

/////////////////////////////////////////////////////////////////////////////
// CProperties dialog


CProperties::CProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProperties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_bdisk = FALSE;
    m_type = 0;
    m_sdev = NULL;

    m_volume = NULL;
    m_cdrom = NULL;
    m_disk = NULL;
    m_part = NULL;

    cbDiskBox = &m_DiskBox.m_ComboBox;
    cbPartBox = &m_PartBox.m_ComboBox;
}


void CProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProperties)
		// NOTE: the ClassWizard will add DDX and DDV calls here
    DDX_Control(pDX, IDC_PROPERTY_DEVICE, m_DiskBox);
    DDX_Control(pDX, IDC_PROPERTY_SDEV,   m_PartBox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProperties, CDialog)
	//{{AFX_MSG_MAP(CProperties)
	ON_BN_CLICKED(IDC_SDEV_QUICK_MOUNT, OnSdevQuickMount)
	ON_BN_CLICKED(IDC_SDEV_CHANGE_MP, OnSdevChangeMp)
	ON_BN_CLICKED(IDC_SDEV_EXT2_INFO, OnSdevExt2Info)
    ON_MESSAGE(WM_GROUP_BOX_UPDATED,  OnGroupBoxUpdated)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProperties message handlers

void CProperties::ResetDiskGroup()
{
    CString s = "";

    SET_TEXT(IDC_VENDOR_ID, s);
    SET_TEXT(IDC_PRODUCT_ID, s);
    SET_TEXT(IDC_SERIAL_NUMBER, s);
    SET_TEXT(IDC_BUS_TYPE, s);
    SET_TEXT(IDC_DEVICE_TYPE, s);
    SET_TEXT(IDC_TOTAL_SIZE, s);
    SET_TEXT(IDC_MEDIA_TYPE, s);
}

void CProperties::ResetPartGroup()
{
    CString s = "";

    SET_TEXT(IDC_MOUNT_POINTS, s);
    SET_TEXT(IDC_SDEV_STATUS, s);
    SET_TEXT(IDC_SDEV_SIZE, s);
    SET_TEXT(IDC_SDEV_FREE_SIZE, s);
    SET_TEXT(IDC_FILE_SYSTEM, s);
    SET_WIN(IDC_SDEV_CHANGE_MP, FALSE);
    SET_WIN(IDC_SDEV_QUICK_MOUNT, FALSE);
    SET_WIN(IDC_SDEV_EXT2_INFO, FALSE);
}

void CProperties::OnSdevChangeMp() 
{
    CMountPoints    mntPoint;
    BOOL         bInited = FALSE;

    if (m_cdrom) {
        ASSERT(!bInited);
        mntPoint.m_Cdrom = m_cdrom;
        bInited = TRUE;
    }

    if (m_part) {
        ASSERT(!bInited);
        mntPoint.m_Part = m_part;
        bInited = TRUE;
    }

    if (m_volume) {
        ASSERT(!bInited);
        mntPoint.m_Volume = m_volume;
        bInited = TRUE;
    }

    if (!bInited) {
        return;
    }

    mntPoint.m_MainDlg = GetParent();
    mntPoint.DoModal();

    if (mntPoint.m_bUpdated) {
        CExt2MgrDlg * Parent = (CExt2MgrDlg *)GetParent();

        if (mntPoint.m_Volume) {
            Parent->UpdateVolume(mntPoint.m_Volume);
        }
        if (mntPoint.m_Cdrom) {
            Parent->UpdateCdrom(mntPoint.m_Cdrom);
        }
        if (mntPoint.m_Part) {
            Parent->UpdatePartition(mntPoint.m_Part);
        }
    }

    OnOK();
}

void CProperties::OnSdevQuickMount() 
{
    PCHAR           dev = NULL;

    if (m_cdrom) {
        dev = m_cdrom->Name;
    }

    if (m_part) {
        if (m_part->Volume)
            dev = m_part->Volume->Name;
        else
            dev = m_part->Name;
    }

    if (m_volume) {
        dev = m_volume->Name;
    }

    if (!dev) {
        return;
    }

    if (Ext2MountVolume(dev)) {
        CExt2MgrDlg * Parent = (CExt2MgrDlg *)GetParent();
        if (m_cdrom) {
            Parent->UpdateCdrom(m_cdrom);
        } else if (m_volume) {
            Parent->UpdateVolume(m_volume);
        } else if (m_part) {
            Parent->UpdateVolume(m_part->Volume);
        }
    }

    OnOK();
}

void CProperties::OnSdevExt2Info() 
{
    NT::NTSTATUS status;
    HANDLE  Handle = NULL;
    CString s;

    CExt2Attribute EA;
    PEXT2_VOLUME_PROPERTY3 EVP = NULL;

    if (m_cdrom && (m_cdrom->EVP.bExt2 || m_cdrom->EVP.bExt3)) {
        EVP = &m_cdrom->EVP;
        EA.m_bCdrom = TRUE;
        EA.m_DevName = m_cdrom->Name;
    } else if (m_volume) {
        EVP = &m_volume->EVP;
        if (m_volume->Part)
            EA.m_DevName = m_volume->Part->Name;
        else
            EA.m_DevName = m_volume->Name;
    } else if (m_part) {
        EVP = &m_part->Volume->EVP;
        EA.m_DevName = m_part->Volume->Name;
    }

    if (!EVP) {
        return;
    }

    EA.m_MainDlg = GetParent();
    EA.m_EVP = EVP;

    status = Ext2Open(EA.m_DevName.GetBuffer(EA.m_DevName.GetLength()),
                      &Handle, EXT2_DESIRED_ACCESS);

    if (!NT_SUCCESS(status)) {

        s.Format("Ext2Fsd service isn't started.\n");
        AfxMessageBox(s, MB_OK | MB_ICONSTOP);

    } else {

        if (!Ext2QueryExt2Property(Handle, EVP)) {
            Ext2Close(&Handle);
            return;
        }

        Ext2Close(&Handle);
    }


    if (EA.DoModal() == IDOK) {
        CExt2MgrDlg * Parent = (CExt2MgrDlg *)GetParent();
        if (m_cdrom) {
            Parent->UpdateCdrom(m_cdrom);
        } else if (m_volume) {
            Parent->UpdateVolume(m_volume);
        } else if (m_part) {
            Parent->UpdateVolume(m_part->Volume);
        }
    }
}

void CProperties::SetVolume(PEXT2_VOLUME vol)
{
    CString s;

    if (vol->Extent->NumberOfDiskExtents  == 1) {
        m_disk = &gDisks[vol->Extent->Extents[0].DiskNumber];
        m_part = NULL;
        for (UCHAR i = 0; i < m_disk->NumParts; i++) {
            if (m_disk->DataParts[i].Volume == vol) {
                m_part = &m_disk->DataParts[i];
                break;
            }
        }
    } else {
        m_disk = NULL;
        m_part = NULL;
    }

    if (m_part) {
        m_disk = NULL;
        m_volume = NULL;
        SetPartition(m_part);
        if (0 == m_part->DrvLetters)
            SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        else
            SET_WIN(IDC_SDEV_QUICK_MOUNT, FALSE);
        return;
    } else {
        m_disk = NULL;
        ResetDiskGroup();
        if (m_volume) {
            if (0 == m_volume->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
            else
                SET_WIN(IDC_SDEV_QUICK_MOUNT, FALSE);
        }
    }

    cbDiskBox->SetCurSel(-1);
    ResetDiskGroup();

    cbPartBox->ResetContent();
    cbPartBox->AddString("Volume");
    cbPartBox->SetCurSel(0);

    /* set mount points */
    SET_TEXT(IDC_MOUNT_POINTS, 
             Ext2QueryVolumeLetterStrings(
                            vol->DrvLetters, NULL));

    /* set volume status */
    s = "Online";
    if (vol->bRecognized && (vol->EVP.bExt2 || vol->EVP.bExt3)) {
        s += ",codepage:";
        s += vol->EVP.Codepage;
        if (vol->EVP.bReadonly) {
            s += ",Readonly";
        }
    }
    SET_TEXT(IDC_SDEV_STATUS, s);

    {
        ULONGLONG   totalSize, freeSize;
        totalSize = vol->FssInfo.TotalAllocationUnits.QuadPart;
        freeSize  = vol->FssInfo.AvailableAllocationUnits.QuadPart;
        totalSize = totalSize * vol->FssInfo.BytesPerSector *
                    vol->FssInfo.SectorsPerAllocationUnit;
        freeSize  = freeSize * vol->FssInfo.BytesPerSector *
                    vol->FssInfo.SectorsPerAllocationUnit;
        s.Format("%I64u", totalSize);
        SET_TEXT(IDC_SDEV_SIZE, s);
        s.Format("%I64u", freeSize);
        SET_TEXT(IDC_SDEV_FREE_SIZE, s);
    }

    SET_TEXT(IDC_FILE_SYSTEM, vol->FileSystem);

    SET_WIN(IDC_SDEV_CHANGE_MP, TRUE);
    if (0 == vol->DrvLetters){
        SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
    }

    if (vol->bRecognized && (vol->EVP.bExt2 || vol->EVP.bExt3)) {
        SET_WIN(IDC_SDEV_EXT2_INFO, TRUE);
    }
}

void CProperties::SetPartition(PEXT2_PARTITION part)
{
    CString     s;

    ResetPartGroup();

    if (m_disk != part->Disk) {
        SetDisk(m_disk = part->Disk);
    }

    if (!m_disk->bLoaded || m_disk->Layout == NULL) {
        cbPartBox->SetCurSel(-1);
        return;
    }

    cbPartBox->SetCurSel(part->Number - 1);

    /* set mount points */
    SET_TEXT(IDC_MOUNT_POINTS, 
             Ext2QueryVolumeLetterStrings(
                            part->DrvLetters, NULL));

    if (m_disk->SDD.RemovableMedia) {
        SET_WIN(IDC_SDEV_CHANGE_MP, TRUE);
    }

    if (part->Volume) {
        if (!part->Volume->bDynamic) {
            SET_WIN(IDC_SDEV_CHANGE_MP, TRUE);
        }
        if (0 == part->Volume->DrvLetters){
            SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        }
    } else {
        s.Format("PARTITION %d", 0);
        SET_TEXT(IDC_PROPERTY_SDEV, s);

        if (m_disk->SDD.RemovableMedia) {
            if (m_disk->bEjected) {
                s = "Media ejected";
            } else {
                s = "Stopped";
            }
        } else {
            if (m_disk->Layout) {
                s = "Not recognized";
            } else {
                s = "RAW";
            }
        }

        if (0 == part->DrvLetters){
            SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        }

        SET_TEXT(IDC_SDEV_STATUS, s);
        return;
    }

    if (part->Volume->bRecognized &&
        (part->Volume->EVP.bExt2 || part->Volume->EVP.bExt3)) {
        SET_WIN(IDC_SDEV_EXT2_INFO, TRUE);
    }

    s = "Online,";
    if (part->Entry->PartitionStyle == PARTITION_STYLE_MBR) {
        s += PartitionString(part->Entry->Mbr.PartitionType);
    } else {
        s += "GPT";
    }
    if ( part->Volume->bRecognized && 
         (part->Volume->EVP.bExt2 || part->Volume->EVP.bExt3)) {
        s += ",codepage:";
        s += part->Volume->EVP.Codepage;
        if (part->Volume->EVP.bReadonly) {
            s += ",Readonly";
        }
    }
    SET_TEXT(IDC_SDEV_STATUS, s);

    if (part->Volume->bRecognized) {
        ULONGLONG   totalSize, freeSize;
        totalSize = part->Volume->FssInfo.TotalAllocationUnits.QuadPart;
        freeSize  = part->Volume->FssInfo.AvailableAllocationUnits.QuadPart;
        totalSize = totalSize * part->Volume->FssInfo.BytesPerSector *
                    part->Volume->FssInfo.SectorsPerAllocationUnit;
        freeSize  = freeSize * part->Volume->FssInfo.BytesPerSector *
                    part->Volume->FssInfo.SectorsPerAllocationUnit;
        s.Format("%I64u", totalSize);
        SET_TEXT(IDC_SDEV_SIZE, s);
        s.Format("%I64u", freeSize);
        SET_TEXT(IDC_SDEV_FREE_SIZE, s);
    } else {
        s.Format("%I64u", part->Entry->PartitionLength.QuadPart);
        SET_TEXT(IDC_SDEV_SIZE, s);
        SET_TEXT(IDC_SDEV_FREE_SIZE, "0");
    }

    SET_TEXT(IDC_FILE_SYSTEM, part->Volume->FileSystem);
}

void CProperties::SetDisk(PEXT2_DISK disk)
{
    CString     s;
    ULONGLONG   size = 1;

    ResetPartGroup();
    cbPartBox->ResetContent();

    if (!disk->bLoaded) {
        ResetDiskGroup();
        return;
    } else {
        cbDiskBox->SetCurSel(disk->OrderNo);
    }

    if (disk->Layout) {
        for (UCHAR i=0; i < disk->NumParts; i++) {
            s.Format("PARTITION %u", i+1);
            cbPartBox->AddString(s.GetBuffer(s.GetLength()));
        }
    }

    if (disk->SDD.VendorIdOffset) {
        s = (PCHAR)&disk->SDD + disk->SDD.VendorIdOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_VENDOR_ID, s);

    if (disk->SDD.ProductIdOffset) {
        s = (PCHAR)&disk->SDD + disk->SDD.ProductIdOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_PRODUCT_ID, s);

    if (disk->SDD.SerialNumberOffset) {
        s = (PCHAR)&disk->SDD + disk->SDD.SerialNumberOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_SERIAL_NUMBER, s);

    s = BusTypeString(disk->SDD.BusType);
    SET_TEXT(IDC_BUS_TYPE, s);

    if (disk->SDD.RemovableMedia) {
        s = "Removable";
    } else {
        s = "RAW";
        if (disk->Layout) {
            if (disk->Layout->PartitionStyle == PARTITION_STYLE_MBR) {
                if (disk->Layout->PartitionEntry->Mbr.PartitionType
                    == PARTITION_LDM) {
                    s.LoadString(IDS_DISK_TYPE_DYN);
                } else {
                    s.LoadString(IDS_DISK_TYPE_BASIC);
                }
            } else if (disk->Layout->PartitionStyle == PARTITION_STYLE_MBR) {
                 s = "GUID";
            }
        }
    }
    SET_TEXT(IDC_DEVICE_TYPE, s);

    if (disk->bEjected) {
        s = "No media";
    } else {

        /* set size */
        size = size * disk->DiskGeometry.BytesPerSector;
        size = size * disk->DiskGeometry.SectorsPerTrack;
        size = size * disk->DiskGeometry.TracksPerCylinder;
        size = size * disk->DiskGeometry.Cylinders.QuadPart;
        s.Format("%I64u", size);
        SET_TEXT(IDC_TOTAL_SIZE, s);

        switch (disk->DiskGeometry.MediaType) {
		        case FixedMedia: s="Fixed"; break;
		        case RemovableMedia: s="Removable"; break;
		        case CD_ROM: s="CDROM"; break;
		        case CD_R: s="CDR"; break;
		        case CD_RW: s="CDRW"; break;
		        case DVD_ROM: s="DVD"; break;
		        case DVD_R: s="DVDR"; break;
		        case DVD_RW: s="DVDRW"; break;
		        default: s="Unkown";
        }
    }
 
    SET_TEXT(IDC_MEDIA_TYPE, s);
}


void CProperties::SetCdrom(PEXT2_CDROM cdrom)
{
    CString     s;
    ULONGLONG   size = 1;

    cbDiskBox->SetCurSel(cdrom->OrderNo + g_nDisks);

    ResetPartGroup();
    cbPartBox->ResetContent();
    cbPartBox->AddString("Media");
    cbPartBox->SetCurSel(0);

    SET_WIN(IDC_SDEV_CHANGE_MP, TRUE);

    if (0 == cdrom->DrvLetters)
        SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);


    if (cdrom->SDD.VendorIdOffset) {
        s = (PCHAR)&cdrom->SDD + cdrom->SDD.VendorIdOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_VENDOR_ID, s);

    if (cdrom->SDD.ProductIdOffset) {
        s = (PCHAR)&cdrom->SDD + cdrom->SDD.ProductIdOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_PRODUCT_ID, s);

    if (cdrom->SDD.SerialNumberOffset) {
        s = (PCHAR)&cdrom->SDD + cdrom->SDD.SerialNumberOffset;
    } else {
        s.Empty();
    }
    SET_TEXT(IDC_SERIAL_NUMBER, s);

    s = BusTypeString(cdrom->SDD.BusType);
    SET_TEXT(IDC_BUS_TYPE, s);

    s.LoadString(IDS_DISK_TYPE_BASIC);
    SET_TEXT(IDC_DEVICE_TYPE, s);

    if (cdrom->bLoaded) {
        if (cdrom->bIsDVD) {
            s = "DVD";
        } else {
            s = "CDROM";
        }
        size = size * cdrom->DiskGeometry.BytesPerSector;
        size = size * cdrom->DiskGeometry.SectorsPerTrack;
        size = size * cdrom->DiskGeometry.TracksPerCylinder;
        size = size * cdrom->DiskGeometry.Cylinders.QuadPart;
    } else {
        s = "No media";
        size = 0;
    }
    SET_TEXT(IDC_MEDIA_TYPE, s);

    if (cdrom->bLoaded) {
        if (cdrom->bEjected) {
	        s = "Media ejected";
        } else {
            s.Format("%I64u", size);
            SET_TEXT(IDC_TOTAL_SIZE, s);
            SET_TEXT(IDC_SDEV_SIZE, s);
            SET_TEXT(IDC_SDEV_FREE_SIZE, "0");

            if (cdrom->EVP.bExt2) {
                s = "EXT";
                s += (CHAR)('2' + cdrom->EVP.bExt3);
                SET_WIN(IDC_SDEV_EXT2_INFO, TRUE);
            } else {
                s = "CDFS";
            }
            SET_TEXT(IDC_FILE_SYSTEM, s);

            s = "Online,";
	        switch (cdrom->DiskGeometry.MediaType) {
		        case FixedMedia: s +="Fixed"; break;
		        case RemovableMedia: s += "Media Removable"; break;
		        case CD_ROM: s +=" CDROM"; break;
		        case CD_R: s += "CDR"; break;
		        case CD_RW: s += "CDRW"; break;
		        case DVD_ROM: s += "DVD"; break;
		        case DVD_R: s += "DVDR"; break;
		        case DVD_RW: s += "DVDRW"; break;
		        default: s += "Unkown";
            }
        }
    } else {
        s = "Device stopped";
    }
    SET_TEXT(IDC_SDEV_STATUS, s);

    if (0 == cdrom->DrvLetters){
        SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
    }

    SET_TEXT(IDC_MOUNT_POINTS, 
             Ext2QueryVolumeLetterStrings(
                            cdrom->DrvLetters, NULL));
}

BOOL CProperties::OnInitDialog() 
{
    CString str;

	CDialog::OnInitDialog();

    for (ULONG i = 0; i < g_nDisks; i++) {
        str.Format("DISK %u", i);
        cbDiskBox->AddString(str.GetBuffer(str.GetLength()));
    }

    for (ULONG i = 0; i < g_nCdroms; i++) {
        str.Format("CDROM %u", i);
        cbDiskBox->AddString(str.GetBuffer(str.GetLength()));
    }

    SET_WIN(IDC_SDEV_CHANGE_MP, FALSE);
    SET_WIN(IDC_SDEV_EXT2_INFO, FALSE);
    SET_WIN(IDC_SDEV_QUICK_MOUNT, FALSE);

    if (m_bdisk) {
        if (m_type == EXT2_DISK_MAGIC) {
            m_disk = (PEXT2_DISK) m_sdev;
            SetDisk(m_disk);
        } else if (m_type == EXT2_PART_MAGIC) {
            m_part = (PEXT2_PARTITION) m_sdev;
            SetPartition(m_part);
            if (0 == m_part->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
            if (m_part->Volume && 0 == m_part->Volume->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        } else if (m_type == EXT2_DISK_NULL_MAGIC) {
            m_disk = (PEXT2_DISK)m_sdev;
            SetDisk(m_disk);
        } else if (m_type == EXT2_CDROM_VOLUME_MAGIC) {
            m_cdrom = (PEXT2_CDROM)m_sdev;
            SetCdrom(m_cdrom);
            if (0 == m_cdrom->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        } else if (m_type == EXT2_CDROM_DEVICE_MAGIC){
            m_cdrom = (PEXT2_CDROM)m_sdev;
            SetCdrom(m_cdrom);
            if (0 == m_cdrom->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        }

        if (m_disk && NULL == m_part) {
            if (m_disk->bLoaded && m_disk->NumParts > 0) {
                m_part = &m_disk->DataParts[0];
                SetPartition(m_part);
                if (0 == m_part->DrvLetters)
                    SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
                if (m_part->Volume && 0 == m_part->Volume->DrvLetters)
                    SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
            }
        }

    } else {
        if (m_type == EXT2_VOLUME_MAGIC) {
            m_volume = (PEXT2_VOLUME)m_sdev;
            SetVolume(m_volume);
            if (m_volume && 0 == m_volume->DrvLetters) {
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
            }
        } else {
            ASSERT(m_type == EXT2_CDROM_DEVICE_MAGIC);
            m_cdrom = (PEXT2_CDROM)m_sdev;
            SetCdrom(m_cdrom);
            if (0 == m_cdrom->DrvLetters)
                SET_WIN(IDC_SDEV_QUICK_MOUNT, TRUE);
        }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CProperties::OnGroupBoxUpdated(WPARAM wParam,LPARAM lParam)
{
    ULONG     i;
    BOOL bChanged = FALSE;

    if (wParam == 'GB') {
	    if (lParam == 'DVLU') {
            i = (ULONG)cbDiskBox->GetCurSel();
            if (i >= g_nDisks) {
                if (m_disk != NULL || (m_cdrom != NULL &&
                        m_cdrom != &gCdroms[i - g_nDisks])) {
                    m_disk = NULL;
                    m_part = NULL;
                    m_volume = NULL;
                    m_cdrom = &gCdroms[i - g_nDisks];
                    bChanged = TRUE;
                } else if (m_volume) {
                    m_volume = NULL;
                    m_cdrom = &gCdroms[i - g_nDisks];
                    bChanged = TRUE;
                }
            } else {
                if (m_cdrom != NULL || (m_disk != NULL &&
                        m_disk != &gDisks[i])) {
                    m_disk = &gDisks[i];
                    m_cdrom = NULL;
                    m_volume = NULL;
                    m_part = NULL;
                    bChanged = TRUE;
                } else if (m_volume){
                    m_volume = NULL;
                    m_disk = &gDisks[i];
                    bChanged = TRUE;
                }
            }

            if (bChanged) {

                if (m_cdrom) {
                    SetCdrom(m_cdrom);
                }
                if (m_disk) {
                    SetDisk(m_disk);
                    if (m_disk->bLoaded && m_disk->NumParts > 0) {
                        m_part = &m_disk->DataParts[0];
                        SetPartition(m_part);
                    } else {
                        m_part = NULL;
                    }
                }
            }
 	    }

	    if (lParam == 'PVLU') {
            i = cbPartBox->GetCurSel();
            if (m_part && ((i + 1) != m_part->Number)) {
                m_part = &m_disk->DataParts[i];
                SetPartition(m_part);
            }
 	    }
    }

    return TRUE;
}
