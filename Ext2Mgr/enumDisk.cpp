
#include "stdafx.h"
#include "mountmgr.h"

/* global management information */

BOOL g_bAutoMount = 0;

ULONG g_nFlps     = 0;
ULONG g_nDisks    = 0;
ULONG g_nCdroms   = 0;
ULONG g_nVols     = 0;

EXT2_LETTER drvLetters[26];
EXT2_LETTER drvDigits[10];

ULONGLONG Ext2DrvLetters[2] = {(ULONGLONG)-1, (ULONGLONG)-1};

PEXT2_DISK      gDisks  = NULL;
PEXT2_CDROM     gCdroms = NULL;
PEXT2_VOLUME    gVols = NULL;

/* information string array */

typedef struct {
  int PartitionType;
  char *type;
} PARTITION_LIST;

typedef struct {
  UINT DriveType;
  char *type;
} DRIVE_LIST;

typedef struct {
  DEVICE_TYPE DeviceType;
  char *type;
} DEVICE_LIST;

typedef struct {
  STORAGE_BUS_TYPE BusType;
  char *type;
} BUSTYPE_LIST;

CHAR *
IrpMjStrings[] = {
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP"
};

CHAR *
PerfStatStrings[] = {
        "IRP_CONTEXT",
        "VCB",
        "FCB",
        "CCB",
        "MCB",
        "EXTENT",
        "RW_CONTEXT",
        "VPB",
        "FCB_NAME",
        "MCB_NAME",
        "FILE_NAME",
        "DIR_ENTRY",
        "DIR_PATTERN",
        "DISK_EVENT",
        "DISK_BUFFER",
        "BLOCK_DATA",
        "inode",
        "dentry",
        "buffer head",
        NULL
    };

PARTITION_LIST PartitionList[] = {
    {PARTITION_ENTRY_UNUSED ,   "Empty"},
    {PARTITION_FAT_12 ,         "FAT12"},   /* 01 */
    {PARTITION_XENIX_1 ,        "Xenix-1"}, /* 02 */
    {PARTITION_XENIX_2 ,        "Xenix-2"},
    {PARTITION_FAT_16 ,         "FAT16"},   /* 04 */
    {PARTITION_EXTENDED ,       "Extended"},
    {PARTITION_HUGE ,           "FAT16 HUGE"}, /* 06*/
    {PARTITION_IFS ,            "HPFS/NTFS"},     /* 07 */

    {PARTITION_OS2BOOTMGR,      "OS/2"},    /* 0A */
    {PARTITION_FAT32 ,          "FAT32"},   /* 0B */
    {PARTITION_FAT32_XINT13 ,   "FAT32X"},  /* 0C*/
    {PARTITION_XINT13 ,         "XINT13"},  /* 0E */
    {PARTITION_XINT13_EXTENDED ,"EXINT13"}, /* 0F */
    {0x11 ,                     "Hidden FAT12"},
    {0x14 ,                     "Hidden FAT16"},
    {0x16 ,                     "Hidden FAT16"},
    {0x17 ,                     "Hidden HPFS/NTFS"},

    {0x1B ,                     "Hidden FAT32"},
    {0x1C ,                     "Hidden FAT32X"},

    {PARTITION_PREP ,           "OS/2"},    /* 41 */
    {PARTITION_LDM ,            "LDM"},     /* 42 */

    {0x52 ,                     "CP/M"},

    {PARTITION_UNIX ,           "UNIX"},    /* 63 */

    {PARTITION_NTFT ,           "NTFT"},    /* 80 */
    {0x81,                      "Minix"},
    {0x82,                      "Linux swap"},
    {0x83,                      "Linux"},

    {0x85,                      "Linux extend"},

    {0x8e,                      "Linux LVM"},

    {0xa5,                      "FreeBSD"},
    {0xa6,                      "OpenBSD"},

    {0xa8,                      "Darwin UFS"},
    {0xa9,                      "NetBSD"},

    {0xbe,                      "Solaris Boot"},
    {0xbf,                      "Solaris"},

    {VALID_NTFT ,               "VNTFT"},   /* C0 */
    {-1 ,"UNKNOWN"}
};

DRIVE_LIST DriveList[] = {
    {DRIVE_UNKNOWN,     "Unkown"},
    {DRIVE_NO_ROOT_DIR, "NoRoot"},
    {DRIVE_REMOVABLE,   "Removable"},
    {DRIVE_FIXED,       "Fixed"},
    {DRIVE_REMOTE,      "Remote"},
    {DRIVE_CDROM,       "CDROM"},
    {DRIVE_RAMDISK,     "RAMdisk"},
    {(UINT)-1,          "Invalid"}
};

BUSTYPE_LIST BusTypeList[] = {
    {BusTypeUnknown,    "Unkown"},
    {BusTypeScsi,       "SCSI"},
    {BusTypeAtapi,      "ATAPI"},
    {BusTypeAta,        "ATA"},
    {BusType1394,       "1394"},
    {BusTypeSsa,        "Ssa"},
    {BusTypeFibre,      "Fibre"},
    {BusTypeUsb,        "USB"},
    {BusTypeRAID,       "RAID"},
    {(STORAGE_BUS_TYPE)-1, "Invalid"}
};


DEVICE_LIST DeviceList[] = {
    {FILE_DEVICE_8042_PORT          ,"8042_PORT"},
    {FILE_DEVICE_ACPI               ,"ACPI"},
    {FILE_DEVICE_BATTERY            ,"BATTERY"},
    {FILE_DEVICE_BEEP               ,"BEEP"},
    {FILE_DEVICE_BUS_EXTENDER       ,"BUS_EXTENDER"},
    {FILE_DEVICE_CD_ROM             ,"CD_ROM"},
    {FILE_DEVICE_CD_ROM_FILE_SYSTEM ,"CD_ROM_FILE_SYSTEM"},
    {FILE_DEVICE_CHANGER            ,"CHANGER"},
    {FILE_DEVICE_CONTROLLER         ,"CONTROLLER"},
    {FILE_DEVICE_DATALINK           ,"DATALINK"},
    {FILE_DEVICE_DFS                ,"DFS"},
    {FILE_DEVICE_DFS_FILE_SYSTEM    ,"DFS_FILE_SYSTEM"},
    {FILE_DEVICE_DFS_VOLUME         ,"DFS_VOLUME"},
    {FILE_DEVICE_DISK               ,"DISK"},
    {FILE_DEVICE_DISK_FILE_SYSTEM   ,"DISK_FILE_SYSTEM"},
    {FILE_DEVICE_DVD                ,"DVD"},
    {FILE_DEVICE_FILE_SYSTEM        ,"FILE_SYSTEM"},
    {0x0000003a /*FILE_DEVICE_FIPS*/ ,"FIPS"},
    {FILE_DEVICE_FULLSCREEN_VIDEO   ,"FULLSCREEN_VIDEO"},
    {FILE_DEVICE_INPORT_PORT        ,"INPORT_PORT"},
    {FILE_DEVICE_KEYBOARD           ,"KEYBOARD"},
    {FILE_DEVICE_KS                 ,"KS"},
    {FILE_DEVICE_KSEC               ,"KSEC"},
    {FILE_DEVICE_MAILSLOT           ,"MAILSLOT"},
    {FILE_DEVICE_MASS_STORAGE       ,"MASS_STORAGE"},
    {FILE_DEVICE_MIDI_IN            ,"MIDI_IN"},
    {FILE_DEVICE_MIDI_OUT           ,"MIDI_OUT"},
    {FILE_DEVICE_MODEM              ,"MODEM"},
    {FILE_DEVICE_MOUSE              ,"MOUSE"},
    {FILE_DEVICE_MULTI_UNC_PROVIDER ,"MULTI_UNC_PROVIDER"},
    {FILE_DEVICE_NAMED_PIPE         ,"NAMED_PIPE"},
    {FILE_DEVICE_NETWORK            ,"NETWORK"},
    {FILE_DEVICE_NETWORK_BROWSER    ,"NETWORK_BROWSER"},
    {FILE_DEVICE_NETWORK_FILE_SYSTEM,"NETWORK_FILE_SYSTEM"},
    {FILE_DEVICE_NETWORK_REDIRECTOR ,"NETWORK_REDIRECTOR"},
    {FILE_DEVICE_NULL               ,"NULL"},
    {FILE_DEVICE_PARALLEL_PORT      ,"PARALLEL_PORT"},
    {FILE_DEVICE_PHYSICAL_NETCARD   ,"PHYSICAL_NETCARD"},
    {FILE_DEVICE_PRINTER            ,"PRINTER"},
    {FILE_DEVICE_SCANNER            ,"SCANNER"},
    {FILE_DEVICE_SCREEN             ,"SCREEN"},
    {FILE_DEVICE_SERENUM            ,"SERENUM"},
    {FILE_DEVICE_SERIAL_MOUSE_PORT  ,"SERIAL_MOUSE_PORT"},
    {FILE_DEVICE_SERIAL_PORT        ,"SERIAL_PORT"},
    {FILE_DEVICE_SMARTCARD          ,"SMARTCARD"},
    {FILE_DEVICE_SMB                ,"SMB"},
    {FILE_DEVICE_SOUND              ,"SOUND"},
    {FILE_DEVICE_STREAMS            ,"STREAMS"},
    {FILE_DEVICE_TAPE               ,"TAPE"},
    {FILE_DEVICE_TAPE_FILE_SYSTEM   ,"TAPE_FILE_SYSTEM"},
    {FILE_DEVICE_TERMSRV            ,"TERMSRV"},
    {FILE_DEVICE_TRANSPORT          ,"TRANSPORT"},
    {FILE_DEVICE_UNKNOWN            ,"UNKNOWN"},
    {FILE_DEVICE_VDM                ,"VDM"},
    {FILE_DEVICE_VIDEO              ,"VIDEO"},
    {FILE_DEVICE_VIRTUAL_DISK       ,"VIRTUAL_DISK"},
    {FILE_DEVICE_WAVE_IN            ,"WAVE_IN"},
    {FILE_DEVICE_WAVE_OUT           ,"WAVE_OUT"},
    {(DEVICE_TYPE)-1                ,"UNKNOWN"}
};

char *PartitionString(int type)
{
    PARTITION_LIST *p = PartitionList;

    while ( p->PartitionType != -1 ) {
        if ( type == p->PartitionType ) {
            return p->type;
        }
        p++;
    }
    return p->type;
}

char *DriveTypeString(UINT media)
{
    DRIVE_LIST *p = DriveList;

    while ( p->DriveType != (UINT)-1 ) {
        if ( media == p->DriveType ) {
            return p->type;
        }
        p++;
    }
    return p->type;
}

char *DeviceTypeString(DEVICE_TYPE media)
{
    DEVICE_LIST *p = DeviceList;

    while ( p->DeviceType != (DEVICE_TYPE)-1 ) {
        if ( media == p->DeviceType ) {
            return p->type;
        }
        p++;
    }
    return p->type;
}

char *BusTypeString(STORAGE_BUS_TYPE BusType)
{
    BUSTYPE_LIST *p = BusTypeList;

    while ( p->BusType != (STORAGE_BUS_TYPE)-1 ) {
        if ( BusType == p->BusType ) {
            return p->type;
        }
        p++;
    }

    return p->type;
}

/* Ext2Fsd supported codepages */

CHAR * gCodepages[] =
  {
    "default",
    "cp936",
    "gb2312",
    "utf8",
    "cp1251",
    "cp1255",
    "cp437",
    "cp737",
    "cp775",
    "cp850",
    "cp852",
    "cp855",
    "cp857",
    "cp860",
    "cp861",
    "cp862",
    "cp863",
    "cp864",
    "cp865",
    "cp866",
    "cp869",
    "cp874",
    "tis-620",
    "cp932",
    "euc-jp",
    "sjis",
    "cp949",
    "euc-kr",
    "cp950",
    "big5",
    "iso8859-1",
    "iso8859-13",
    "iso8859-14",
    "iso8859-15",
    "iso8859-2",
    "iso8859-3",
    "iso8859-4",
    "iso8859-5",
    "iso8859-6",
    "iso8859-7",
    "iso8859-8",
    "iso8859-9",
    "koi8-r",
    "koi8-u",
    "koi8-ru",
    "cp1250",
    "acsii",
    NULL
  };

/* routines */

BOOL g_isWow64 = FALSE;
BOOL g_isX64 = FALSE;

#define PROCESSOR_ARCHITECTURE_AMD64            9

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;

typedef void (WINAPI *LPFN_GETNATIVESYSINFO)(
                  LPSYSTEM_INFO lpSystemInfo);

LPFN_GETNATIVESYSINFO fnGetNativeSystemInfo = NULL;
BOOL Ext2IsWow64()
{
    if (NULL != fnIsWow64Process) {
        if (!fnIsWow64Process(GetCurrentProcess(), &g_isWow64)) {
        }
    }
    return g_isWow64;
}


BOOL Ext2IsX64System()
{
    SYSTEM_INFO sysInfo;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle("kernel32"), "IsWow64Process");

    fnGetNativeSystemInfo = (LPFN_GETNATIVESYSINFO)GetProcAddress(
        GetModuleHandle("kernel32"), "GetNativeSystemInfo");

    if (fnGetNativeSystemInfo) {
        fnGetNativeSystemInfo(&sysInfo);

        if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 &&
            Ext2IsWow64()) {
            g_isWow64 = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsVistaOrAbove()
{
    OSVERSIONINFO   OsVerInfo;

    OsVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 
    if (GetVersionEx(&OsVerInfo)) {

       if (OsVerInfo.dwMajorVersion == 6 && OsVerInfo.dwBuildNumber > 3790)
           return TRUE;

       if (OsVerInfo.dwMajorVersion > 6)
            return TRUE;
    }

    return FALSE;
}

BOOL CanDoLocalMount()
{
    return !IsWindowsVistaOrGreater();
}

BOOL
IsWindows2000()
{
    OSVERSIONINFO OsVer;
    memset(&OsVer, 0, sizeof(OsVer));
    OsVer.dwOSVersionInfoSize = sizeof(OsVer);

    if (GetVersionEx(&OsVer)) {
        if (OsVer.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            OsVer.dwMajorVersion <= 5 && 
            OsVer.dwMinorVersion == 0) {
            return TRUE;
        }
    } else {
        return TRUE;
    }

    return FALSE;
}

BOOL Ext2DismountVolume(CHAR *voldev)
{

	HANDLE	device;
	ULONG	bytes;
    BOOL    rc = FALSE;

	device = CreateFile(voldev, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (device == INVALID_HANDLE_VALUE) {
		goto errorout;
	}

	if (!DeviceIoControl(device, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
                          &bytes, NULL)) {
	}

    rc = DeviceIoControl(device, FSCTL_DISMOUNT_VOLUME,	NULL, 0, NULL, 0,
                         &bytes, NULL);
	CloseHandle(device);

errorout:

	return rc;
}

/*
 * Ext2LockVolume
 *     Lock the volume ...
 *
 * ARGUMENTS:
 *     VolumeHandle:    Volume handle.
 *
 * RETURNS: 
 *     Success or Fail
 *
 * NOTES: 
 *     N/A
 */


NT::NTSTATUS
Ext2LockVolume(HANDLE Handle)
{
    NT::NTSTATUS status;
    NT::IO_STATUS_BLOCK ioSb;

    status = NT::ZwFsControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
    return status;
}


NT::NTSTATUS
Ext2UnLockVolume(HANDLE Handle)
{
    NT::NTSTATUS status;
    NT::IO_STATUS_BLOCK ioSb;

    status = NT::ZwFsControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
    return status;
}


NT::NTSTATUS
Ext2DisMountVolume(HANDLE Handle)
{
    NT::NTSTATUS status;
    NT::IO_STATUS_BLOCK ioSb;

    status = NT::ZwFsControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);

    return status;
}

PDRIVE_LAYOUT_INFORMATION_EXT
Ext2QueryDriveLayout(
    HANDLE Handle,
    PUCHAR NumOfParts
    )
{
    NT::IO_STATUS_BLOCK       ioSb;
    NT::NTSTATUS              status;

    PDRIVE_LAYOUT_INFORMATION_EXT driveLayout = NULL;
    ULONG                     dataSize = 512;
    ULONG                     retSize = 0;

QueryDrive:

    status = STATUS_SUCCESS;
    driveLayout = (PDRIVE_LAYOUT_INFORMATION_EXT) malloc(dataSize);
    if(!driveLayout) {
        goto errorout;
    }
    memset(driveLayout, 0, dataSize);

    if (IsWindows2000()) {

        PDRIVE_LAYOUT_INFORMATION oldLayout = 
              (PDRIVE_LAYOUT_INFORMATION ) malloc(dataSize);
        if (!oldLayout) {
            goto errorout;
        }
        
        status = NT::ZwDeviceIoControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    IOCTL_DISK_GET_DRIVE_LAYOUT,
                    NULL, 0, (PVOID)oldLayout, dataSize
                    );

        if (NT_SUCCESS(status)) {

            ULONG newLayoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry);
            newLayoutSize += sizeof(PARTITION_INFORMATION_EX) * oldLayout->PartitionCount;

            if (dataSize >= newLayoutSize) {

                driveLayout->PartitionStyle = PARTITION_STYLE_MBR;
                driveLayout->PartitionCount = oldLayout->PartitionCount;
                driveLayout->Mbr.Signature = oldLayout->Signature;

                for (DWORD i=0; i < oldLayout->PartitionCount; i++) {
                    driveLayout->PartitionEntry[i].PartitionStyle = PARTITION_STYLE_MBR;

                    driveLayout->PartitionEntry[i].StartingOffset = 
                            oldLayout->PartitionEntry[i].StartingOffset;
                    driveLayout->PartitionEntry[i].PartitionLength =
                            oldLayout->PartitionEntry[i].PartitionLength;
                    driveLayout->PartitionEntry[i].PartitionNumber =
                            oldLayout->PartitionEntry[i].PartitionNumber;
                    driveLayout->PartitionEntry[i].RewritePartition =
                            oldLayout->PartitionEntry[i].RewritePartition;
                    driveLayout->PartitionEntry[i].Mbr.PartitionType =
                            oldLayout->PartitionEntry[i].PartitionType;
                    driveLayout->PartitionEntry[i].Mbr.BootIndicator =
                            oldLayout->PartitionEntry[i].BootIndicator;
                    driveLayout->PartitionEntry[i].Mbr.RecognizedPartition =
                            oldLayout->PartitionEntry[i].RecognizedPartition;
                    driveLayout->PartitionEntry[i].Mbr.HiddenSectors = 
                            oldLayout->PartitionEntry[i].HiddenSectors;
                }

            } else {
                dataSize = newLayoutSize;
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        free(oldLayout);

    } else {

        status = NT::ZwDeviceIoControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    IOCTL_DISK_GET_DRIVE_LAYOUT_EXT,
                    NULL, 0, (PVOID)driveLayout, dataSize
                    );
    }

    if (status == STATUS_BUFFER_TOO_SMALL) {
        free(driveLayout); driveLayout = NULL;
        dataSize *= 2;
        goto QueryDrive;
    }

    if (!NT_SUCCESS(status)) {
        free(driveLayout); driveLayout = NULL;
        goto errorout;
    }

    retSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EXT, PartitionEntry);
    retSize += sizeof(PARTITION_INFORMATION) *  driveLayout->PartitionCount;

    if (driveLayout->PartitionStyle == PARTITION_STYLE_MBR) {

        PPARTITION_INFORMATION_EXT  Part;
        UCHAR i = 0, cnt = 0;

        /* Now walk the Drive_Layout to count the partitions */
        while (i < (UCHAR)driveLayout->PartitionCount) {
            Part = &driveLayout->PartitionEntry[i++];
            if (Part->Mbr.PartitionType != PARTITION_ENTRY_UNUSED && 
                Part->Mbr.PartitionType != PARTITION_EXTENDED && 
                Part->Mbr.PartitionType != PARTITION_XINT13_EXTENDED) {
                cnt++;
            }
        }
        *NumOfParts = cnt;
    } else if (driveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
        *NumOfParts = (UCHAR)driveLayout->PartitionCount;
    } else {
        *NumOfParts = 0;
         free(driveLayout); driveLayout = NULL;
    }

    if (*NumOfParts == 0) {
         free(driveLayout); driveLayout = NULL;
    }


errorout:

    return driveLayout;
}

NT::NTSTATUS
Ext2SetDriveLayout(
    HANDLE  Handle,
    PDRIVE_LAYOUT_INFORMATION_EXT Layout
    )
{
    NT::IO_STATUS_BLOCK       ioSb;
    NT::NTSTATUS              status = STATUS_SUCCESS;
    ULONG                     dataSize;

    dataSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EXT, PartitionEntry);
    dataSize += sizeof(PARTITION_INFORMATION_EXT) * Layout->PartitionCount;

    if (IsWindows2000()) {

        if (Layout->PartitionStyle != PARTITION_STYLE_MBR) {
            return STATUS_UNSUCCESSFUL;
        }

        ULONG newLayoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry);
        newLayoutSize += sizeof(PARTITION_INFORMATION) * Layout->PartitionCount;

        PDRIVE_LAYOUT_INFORMATION oldLayout = 
              (PDRIVE_LAYOUT_INFORMATION ) malloc(newLayoutSize);
        if (!oldLayout) {
            goto errorout;
        }
        
        oldLayout->PartitionCount = Layout->PartitionCount;
        oldLayout->Signature = Layout->Mbr.Signature;

        for (DWORD i=0; i < oldLayout->PartitionCount; i++) {

            oldLayout->PartitionEntry[i].StartingOffset = 
                    Layout->PartitionEntry[i].StartingOffset;
            oldLayout->PartitionEntry[i].PartitionLength =
                    Layout->PartitionEntry[i].PartitionLength;
            oldLayout->PartitionEntry[i].PartitionNumber =
                    Layout->PartitionEntry[i].PartitionNumber;
            oldLayout->PartitionEntry[i].RewritePartition =
                    Layout->PartitionEntry[i].RewritePartition;
            oldLayout->PartitionEntry[i].PartitionType =
                    Layout->PartitionEntry[i].Mbr.PartitionType;
            oldLayout->PartitionEntry[i].BootIndicator =
                    Layout->PartitionEntry[i].Mbr.BootIndicator;
            oldLayout->PartitionEntry[i].RecognizedPartition =
                    Layout->PartitionEntry[i].Mbr.RecognizedPartition;
            oldLayout->PartitionEntry[i].HiddenSectors =
                    Layout->PartitionEntry[i].Mbr.HiddenSectors;
        }

        status = NT::ZwDeviceIoControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    IOCTL_DISK_SET_DRIVE_LAYOUT,
                    (PVOID)oldLayout, newLayoutSize,
                    NULL, 0
                    );

        free(oldLayout);

    } else {

        status = NT::ZwDeviceIoControlFile(
                    Handle, NULL, NULL, NULL, &ioSb,
                    IOCTL_DISK_SET_DRIVE_LAYOUT_EXT,
                    (PVOID)Layout, dataSize,NULL, 0
                    );
    }

    return status;

errorout:

    return STATUS_UNSUCCESSFUL;
}

BOOL
Ext2IsDeviceValid(CHAR *Device)
{
    HANDLE  handle = NULL;
    NT::NTSTATUS status;

    status = Ext2Open(Device, &handle, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

errorout:

    if (handle) {
        Ext2Close(&handle);
    }

    return NT_SUCCESS(status);
}


BOOL
Ext2SetPartitionType(
    PEXT2_PARTITION Part,
    BYTE            Type
    )
{
    BOOL rc = FALSE;
    HANDLE  Handle = NULL;
    NT::NTSTATUS status;

    UCHAR  NumParts = 0;
    PDRIVE_LAYOUT_INFORMATION_EXT Layout = NULL;

    DWORD i;

    status = Ext2Open(Part->Disk->Name, &Handle,
                      EXT2_DESIRED_ACCESS | GENERIC_WRITE);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    Layout = Part->Disk->Layout;

    if (!Layout) {
        goto errorout;
    }

    if (Layout->PartitionStyle != PARTITION_STYLE_MBR) {
        goto errorout;
    }

    for (i=0; i < Layout->PartitionCount; i++) {

        if ((Layout->PartitionEntry[i].StartingOffset.QuadPart == 
             Part->Entry->StartingOffset.QuadPart) &&
            (Layout->PartitionEntry[i].PartitionLength.QuadPart ==
             Part->Entry->PartitionLength.QuadPart) &&
            (Layout->PartitionEntry[i].PartitionNumber ==
             Part->Entry->PartitionNumber) ) {

            Layout->PartitionEntry[i].Mbr.PartitionType = Type;
            Layout->PartitionEntry[i].RewritePartition = TRUE;

            rc = TRUE;
            break;
        }
    }

    if (!rc) {
        goto errorout;
    }

    status = Ext2SetDriveLayout(Handle, Layout);
    rc = NT_SUCCESS(status);

errorout:

    Ext2Close(&Handle);

    return rc;
}


BOOL
Ext2FlushVolume(CHAR *Device)
{
    HANDLE  handle = NULL;
    NT::NTSTATUS status;
    NT::IO_STATUS_BLOCK iosb;

    status = Ext2Open(Device, &handle, EXT2_DESIRED_ACCESS | GENERIC_WRITE);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    status = NT::ZwFlushBuffersFile(handle, &iosb);

errorout:

    if (handle) {
        Ext2Close(&handle);
    }

    return NT_SUCCESS(status);
}

PEXT2_PARTITION
Ext2QueryVolumePartition(
    PEXT2_VOLUME    Volume
    )
{
    PEXT2_PARTITION Part = NULL;
    DWORD i, j;

    for (i=0; i < g_nDisks; i++) {
        for (j=0; j < (int)gDisks[i].NumParts; j++) {
            if (gDisks[i].DataParts[j].Volume == Volume) {
                Part = &gDisks[i].DataParts[j];
                break;
            }
        }
    }

    return Part;
}

NT::NTSTATUS
Ext2QueryProperty(
    HANDLE                      Handle, 
    STORAGE_PROPERTY_ID         Id,
    PVOID                       DescBuf,
    ULONG                       DescSize
    )
{
	STORAGE_PROPERTY_QUERY	    SPQ;
    NT::NTSTATUS                status;
    NT::IO_STATUS_BLOCK         ioSb;

	SPQ.PropertyId = Id;
	SPQ.QueryType  = PropertyStandardQuery;

    memset(DescBuf, 0, DescSize);

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &ioSb,
                IOCTL_STORAGE_QUERY_PROPERTY,
                &SPQ, sizeof(STORAGE_PROPERTY_QUERY),
                DescBuf,  DescSize
            );

	return status;
}

/*
 * Ext2QueryDisk
 *     Get volume gemmetry information ...
 *
 * ARGUMENTS:
 *     VolumeHandle:    Volume handle.
 *
 * RETURNS: 
 *     Success or Fail
 *
 * NOTES: 
 *     N/A
 */

NT::NTSTATUS
Ext2QueryDisk(
    HANDLE                  Handle,
    PDISK_GEOMETRY          DiskGeometry
    )
{
    NT::NTSTATUS        status;
    NT::IO_STATUS_BLOCK ioSb;

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &ioSb,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                DiskGeometry, sizeof(DISK_GEOMETRY),
                DiskGeometry, sizeof(DISK_GEOMETRY));


    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

errorout:

    return status;
}

PVOLUME_DISK_EXTENTS
Ext2QueryVolumeExtents(HANDLE hVolume)
{
	ULONG	dataSize = 1024;
	PVOLUME_DISK_EXTENTS dskExtents = NULL;

    NT::NTSTATUS        status;
    NT::IO_STATUS_BLOCK ioSb;

QueryExtent:

    dskExtents = (PVOLUME_DISK_EXTENTS)malloc(dataSize);
    if (NULL == dskExtents) {
        goto errorout;
    }

    status = NT::ZwDeviceIoControlFile(
                hVolume, NULL, NULL, NULL, &ioSb,
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL, 0, dskExtents, dataSize );

    if (status == STATUS_BUFFER_TOO_SMALL) {
        free(dskExtents); dskExtents = NULL;
        dataSize += 1024;
        goto QueryExtent;
    }

    if (!NT_SUCCESS(status)) {
        free(dskExtents); dskExtents = NULL;
        goto errorout;
    }

errorout:

    return dskExtents;
}

PSTORAGE_DEVICE_NUMBER
Ext2QueryDeviceNumber(HANDLE hVolume)
{
	ULONG	            dataSize;
	PSTORAGE_DEVICE_NUMBER SDN = NULL;

    NT::NTSTATUS        status;
    NT::IO_STATUS_BLOCK ioSb;

    dataSize = sizeof(STORAGE_DEVICE_NUMBER);

QuerySDN:

    SDN = (PSTORAGE_DEVICE_NUMBER)malloc(dataSize);
    if (NULL == SDN) {
        goto errorout;
    }
    memset(SDN, 0, dataSize);

    status = NT::ZwDeviceIoControlFile(
                hVolume, NULL, NULL, NULL, &ioSb,
                IOCTL_STORAGE_GET_DEVICE_NUMBER,
                NULL, 0, SDN, dataSize );

    if (status == STATUS_BUFFER_TOO_SMALL) {
        free(SDN); SDN = NULL;
        dataSize += sizeof(STORAGE_DEVICE_NUMBER);
        goto QuerySDN;
    }

    if (!NT_SUCCESS(status)) {
        free(SDN); SDN = NULL;
        goto errorout;
    }

errorout:

    return SDN;
}

BOOL
Ext2QueryDrvLetter(
    PEXT2_LETTER    drvLetter
    )
{
	HANDLE	        hVolume;
    NT::NTSTATUS    status = STATUS_SUCCESS;
    DWORD           rc = 0; 

    drvLetter->DrvType = Ext2QueryDrive(drvLetter->Letter,
                                        drvLetter->SymLink);

    if (drvLetter->DrvType == DRIVE_NO_ROOT_DIR) {
        drvLetter->bUsed = FALSE;
        goto errorout;
    } else {
        drvLetter->bUsed = TRUE;
    }

    if (drvLetter->Letter == 'A' || 
        drvLetter->Letter == 'B' ) {
        drvLetter->bUsed = TRUE;
        goto errorout;
    }

    if (drvLetter->DrvType == DRIVE_REMOVABLE ||
        drvLetter->DrvType == DRIVE_FIXED) {

	    status = Ext2Open(drvLetter->SymLink, &hVolume, EXT2_DESIRED_ACCESS);
	    if (!NT_SUCCESS(status) && status != STATUS_ACCESS_DENIED) {
            drvLetter->bInvalid = TRUE;
            goto errorout;
	    }

  	    drvLetter->Extent = Ext2QueryVolumeExtents(hVolume);
        if (drvLetter->DrvType == DRIVE_REMOVABLE) {
            drvLetter->SDN = Ext2QueryDeviceNumber(hVolume);
        }

	    Ext2Close(&hVolume);
    }

errorout:

    return NT_SUCCESS(status);
}


PEXT2_LETTER Ext2GetFirstUnusedDrvLetter()
{
    PEXT2_LETTER    drvLetter = NULL;
    CHAR            devPath[] = "C:";
    int             i;

    for (i = 5; i < 24; i++) {
        drvLetter = &drvLetters[i];
        if (drvLetter->DrvType == DRIVE_NO_ROOT_DIR) {
            drvLetter->DrvType = Ext2QueryDrive(drvLetter->Letter,
                                                drvLetter->SymLink);
            if (drvLetter->DrvType == DRIVE_NO_ROOT_DIR) {
                /* we got it */
                break;
            } else {
                /* we need do a new refresh */
            }
        }
        drvLetter = NULL;
    }

    return drvLetter;
}


CHAR Ext2MountVolume(CHAR *voldev)
{
    PEXT2_LETTER drvLetter;
    CHAR         rc = 0;
   
    /* query drive letter to check whether it's mounted */
    drvLetter = Ext2GetFirstUnusedDrvLetter();
    if (!drvLetter) {
        goto errorout;
    }

    if (Ext2AssignDrvLetter(drvLetter, voldev, FALSE)) {
        rc = drvLetter->Letter;
    }

errorout:

    return rc;
}

CHAR Ext2MountVolumeAs(CHAR *voldev, CHAR DrvLetter)
{
    PEXT2_LETTER drvLetter;
    CHAR         rc = 0;
   
            
    if (DrvLetter >= '0' && DrvLetter <= '9') {
        drvLetter = &drvDigits[DrvLetter - '0'];
    } else if (DrvLetter >= 'A' && DrvLetter <= 'Z') {
        drvLetter = &drvLetters[DrvLetter - 'A'];
    } else if (DrvLetter >= 'a' && DrvLetter <= 'z') {
        drvLetter = &drvLetters[DrvLetter - 'a'];
    }

    if (!drvLetter || drvLetter->bUsed) {
        goto errorout;
    }

    if (Ext2AssignDrvLetter(drvLetter, voldev, FALSE)) {
        rc = drvLetter->Letter;
    }

errorout:

    return rc;
}

NT::NTSTATUS
Ext2QueryMediaType(
    HANDLE                  Handle,
    PDWORD                  MediaType
    )
{
    NT::NTSTATUS        status;
    NT::IO_STATUS_BLOCK ioSb;
    PGET_MEDIA_TYPES    mediaTypes;
    UCHAR               buffer[1024];

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &ioSb,
                IOCTL_STORAGE_GET_MEDIA_TYPES_EX,
                NULL, 0, buffer, 1024 );


    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    mediaTypes = (PGET_MEDIA_TYPES) buffer;
    *MediaType = mediaTypes->DeviceType;

errorout:

    return status;
}

/*
 * Ext2Read
 *     Read data from disk or file ...
 *
 * ARGUMENTS:
 *     VolumeHandle: Volume Handle
 *     Offset      : Disk Offset
 *     Length      : Data Length to be read
 *     Buffer      : ...
 *
 * RETURNS: 
 *     Success: STATUS_SUCCESS
 *     Fail:  ...
 *
 * NOTES: 
 *     Both Length and Offset should be SECTOR_SIZE aligned.
 */
NT::NTSTATUS 
Ext2Read(
    IN  HANDLE          Handle,
    IN  BOOL         IsFile,
    IN  ULONG           SectorSize,
    IN  ULONGLONG       Offset,
    IN  ULONG           Length,
    IN  PVOID           Buffer
    )
{
    NT::NTSTATUS        status;
    NT::IO_STATUS_BLOCK ioSb;
    LARGE_INTEGER   address;
    ULONG               aLength = 0;
    PVOID               aBuffer = NULL;

    if (SectorSize == 0 || SectorSize == 1)
        IsFile = TRUE;

    ASSERT(Buffer != NULL);

    if (IsFile) {

        address.QuadPart = Offset;
        status = NT::ZwReadFile(
                    Handle, 0, NULL, NULL, &ioSb,
                    Buffer, Length, &address, NULL
                    );
    } else {

        address.QuadPart = Offset & (~((ULONGLONG)SectorSize - 1));
        aLength  = (Length + SectorSize - 1)&(~(SectorSize - 1));
        aLength += ((ULONG)(Offset - address.QuadPart) + SectorSize - 1)
                    & (~(SectorSize - 1));

        aBuffer = malloc(aLength);
        if (!aBuffer) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        status = NT::ZwReadFile(
                    Handle, 0, NULL, NULL, &ioSb,
                    aBuffer, aLength, &address, NULL
                    );

        if (!NT_SUCCESS(status)) {
            goto errorout;
        }

        memmove( Buffer, (PUCHAR)aBuffer + 
                 (ULONG)(Offset - address.QuadPart), Length);
    }
 
errorout:

    if (aBuffer)
        free(aBuffer);

    return status;
}

/*
 * Ext2Write
 *     Write data to disk or file ...
 *
 * ARGUMENTS:
 *     VolumeHandle: Volume Handle
 *     Offset      : Disk Offset
 *     Length      : Data Length to be written
 *     Buffer      : Data to be written ...
 *
 * RETURNS: 
 *     Success: STATUS_SUCCESS
 *     Fail:  ...
 *
 * NOTES: 
 *     Both Length and Offset should be SECTOR_SIZE aligned.
 */

NT::NTSTATUS
Ext2WriteDisk(
    HANDLE          Handle,
    BOOL         IsFile,
    ULONG           SectorSize,
    ULONGLONG       Offset,
    ULONG           Length,
    PVOID           Buffer
    )
{
    LARGE_INTEGER           address;
    NT::NTSTATUS            status;
    NT::IO_STATUS_BLOCK     ioSb;

    ULONG                   aLength = 0;
    PVOID                   aBuffer = NULL;

    if (SectorSize == 0 || SectorSize == 1)
        IsFile = TRUE;

    ASSERT(Buffer != NULL);

    if (IsFile) {

        address.QuadPart = Offset;
        status = NT::ZwWriteFile(
                    Handle, 0, NULL, NULL, &ioSb,
                    Buffer, Length, &address, NULL
                    );
    } else  {

        address.QuadPart = Offset & (~((ULONGLONG)SectorSize - 1));
        aLength = (Length + SectorSize - 1)&(~(SectorSize - 1));
        aLength += ((ULONG)(Offset - address.QuadPart) + SectorSize - 1)
                         & (~(SectorSize - 1));

        aBuffer = malloc(aLength);
        if (!aBuffer) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        if ( (aLength != Length) || 
             (address.QuadPart != (LONGLONG)Offset)) {
            status = NT::ZwReadFile(
                        Handle, 0, NULL, NULL, &ioSb,
                        aBuffer, aLength, &address, NULL
                        );

            if (!NT_SUCCESS(status)) {
                goto errorout;
            }
        }

        memmove((PUCHAR)aBuffer + (ULONG)(Offset - address.QuadPart),
                Buffer, Length);
        status = NT::ZwWriteFile(
                    Handle, 0, NULL, NULL, &ioSb,
                    aBuffer, aLength, &address, NULL
                    );
    }

errorout:

    if (aBuffer)
        free(aBuffer);

    return status;
}

NT::NTSTATUS
Ext2Open(
    PCHAR                   FileName,
    PHANDLE                 Handle,
    ULONG                   DesiredAccess
    )
{
    NT::IO_STATUS_BLOCK     iosb;
    NT::NTSTATUS            status;
    NT::ANSI_STRING         AnsiFilespec;
    NT::UNICODE_STRING      UnicodeFilespec;
    NT::OBJECT_ATTRIBUTES   ObjectAttributes;

    SHORT                   UnicodeName[MAX_PATH];
    CHAR                    AnsiName[MAX_PATH];
    USHORT                  NameLength = 0;

    memset(UnicodeName, 0, sizeof(SHORT) * MAX_PATH);
    memset(AnsiName, 0, sizeof(UCHAR) * MAX_PATH);

    NameLength = (USHORT)strlen(FileName);
    ASSERT(NameLength < MAX_PATH);

    if (FileName[0] == '\\')  {
        memmove(AnsiName, FileName, NameLength);
    } else {
        memmove(&AnsiName[0], "\\DosDevices\\", 12);
        memmove(&AnsiName[12], FileName, NameLength);
        NameLength += 12;
    }

    AnsiFilespec.MaximumLength = AnsiFilespec.Length = NameLength;
    AnsiFilespec.Buffer = AnsiName;

    UnicodeFilespec.MaximumLength = MAX_PATH * 2;
    UnicodeFilespec.Length = 0;
    UnicodeFilespec.Buffer = (PWSTR)UnicodeName;

    NT::RtlAnsiStringToUnicodeString(&UnicodeFilespec, &AnsiFilespec, FALSE);

    //
    // Setup the name in an object attributes structure.
    // Note that we create a name that is case INsensitive
    //

    ObjectAttributes.Length = sizeof(NT::OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.Attributes = 0; /*OBJ_CASE_INSENSITIVE;*/
    ObjectAttributes.ObjectName = &UnicodeFilespec;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;

    //
    // Do the create.  In this particular case, we'll have the I/O Manager
    // make our write requests syncrhonous for our convenience.
    //
    status = NT::ZwCreateFile(
                Handle,                           // returned file handle
                (DesiredAccess | SYNCHRONIZE),    // desired access
                &ObjectAttributes,                // ptr to object attributes
                &iosb,                            // ptr to I/O status block
                0,                                // allocation size
                FILE_ATTRIBUTE_NORMAL,            // file attributes
                FILE_SHARE_WRITE | FILE_SHARE_READ, // share access
                FILE_OPEN  /*FILE_SUPERSEDE*/,    // create disposition
                FILE_SYNCHRONOUS_IO_NONALERT |    // create options
                ((DesiredAccess & GENERIC_WRITE) ? 
                 FILE_NO_INTERMEDIATE_BUFFERING : 0),
                NULL,                             // ptr to extended attributes
                0);                               // length of ea buffer

    //
    // Check the system service status
    //
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Check the returned status too...
    //
    if (!NT_SUCCESS(iosb.Status)) {
        return iosb.Status;
    }

    return status;
}

VOID
Ext2Close(HANDLE * Handle)
{
    NT::NTSTATUS status = 0;
    if (Handle != NULL && *Handle != 0 && *Handle != INVALID_HANDLE_VALUE) {
        status = NT::ZwClose(*Handle); 
        if (NT_SUCCESS(status)) {
            *Handle = 0;
        } else {
            ::MessageBox(NULL, "Failed to close handle", "Ext2Close", MB_OK);
        }
    }
}

BOOL
Ext2QuerySysConfig()
{
    NT::NTSTATUS status;
    NT::_SYSTEM_CONFIGURATION_INFORMATION ConfigInfo;

    status = NT::ZwQuerySystemInformation(
                    NT::SystemConfigurationInformation,
					&ConfigInfo, sizeof(ConfigInfo), 0);
    if (NT_SUCCESS(status)) {
        g_nDisks  = ConfigInfo.DiskCount;
        g_nFlps   = ConfigInfo.FloppyCount;
        g_nCdroms = ConfigInfo.CdRomCount;
        return TRUE;
    }

    return FALSE;
}

BOOL
Ext2LoadDisks()
{
    ULONG           i = 0, j = 0;
    NT::NTSTATUS    status;
    CHAR            drvName[MAX_PATH];

    if (g_nDisks == 0) {
        return FALSE;
    }

    gDisks = (PEXT2_DISK) malloc(sizeof(EXT2_DISK) * g_nDisks);
    if (gDisks == NULL) {
        return FALSE;
    }
    memset(gDisks, 0, sizeof(EXT2_DISK) * g_nDisks);

    while (i < g_nDisks && j < 256) {

        HANDLE  Handle = NULL;

        gDisks[i].Magic = EXT2_DISK_MAGIC;
        gDisks[i].Null  = EXT2_DISK_NULL_MAGIC;
        gDisks[i].OrderNo = (UCHAR) j;
        gDisks[i].bLoaded = FALSE;
        sprintf(drvName, "PhysicalDrive%d\0", j);
        if (QueryDosDevice(drvName, gDisks[i].Name, MAX_PATH) == 0) {
            sprintf(gDisks[i].Name, "\\DosDevices\\PhysicalDrive%d\0", j);
        }
        j++;

        status = Ext2Open(gDisks[i].Name, &Handle, EXT2_DESIRED_ACCESS);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        status = Ext2QueryDisk(Handle, &gDisks[i].DiskGeometry);
        if (NT_SUCCESS(status)) {
            gDisks[i].bEjected = FALSE;
        } else {
            if (STATUS_NO_MEDIA_IN_DEVICE == status) {
                gDisks[i].bEjected = TRUE;
            } else {
                goto Next;
            }
        }

        status = Ext2QueryProperty( Handle, StorageDeviceProperty,
                                    (PVOID)&gDisks[i].SDD,
                                    sizeof(STORAGE_DEVICE_DESCRIPTOR)
                                  );
        if (!NT_SUCCESS(status)) {
            goto Next;
        }

        status = Ext2QueryProperty( Handle, StorageAdapterProperty,
                                    (PVOID)&gDisks[i].SAD,
                                    sizeof(STORAGE_ADAPTER_DESCRIPTOR)
                                  );
        if (!NT_SUCCESS(status)) {
            goto Next;
        }

        if (!gDisks[i].bEjected) {
            gDisks[i].Layout = Ext2QueryDriveLayout(
                        Handle, &gDisks[i].NumParts);
        }

        gDisks[i].bLoaded = TRUE;

Next:

        Ext2Close(&Handle);
        i++;
    }

    g_nDisks = i;

    return TRUE;
}

CString
Ext2PartInformation(PEXT2_PARTITION part)
{
    CString     s, ret="";

    s.Format("\r\n    Partition No: %d\r\n\r\n", part->Number);
    if (!part->Entry) {
        return ret;
    } 

    s = "      Partition Type: ";
    if (part->Entry->PartitionStyle == PARTITION_STYLE_MBR) {
        s += PartitionString(part->Entry->Mbr.PartitionType);
    } else if (part->Entry->PartitionStyle == PARTITION_STYLE_GPT){
        s += "GPT";
        if (part->Entry->Gpt.Name && wcslen(part->Entry->Gpt.Name)) {
            s += ":";
            for (size_t i = 0; i < wcslen(part->Entry->Gpt.Name); i++)
                s += (CHAR)part->Entry->Gpt.Name[i];
        }
    } else {
        s += "GPT";
    }
    ret += s;
    ret += "\r\n";

    s.Format("      StartingOffset: %I64u\r\n", part->Entry->StartingOffset.QuadPart);
    ret += s;

    s.Format("      PartitionLength: %I64u\r\n", part->Entry->PartitionLength.QuadPart);
    ret += s;

    /* mount points */
    ret += "      MountPoints: ";
    ret += Ext2QueryVolumeLetterStrings(part->DrvLetters, NULL);
    ret += "\r\n";

    if (!part->Volume) {
        return ret;
    }

    if (part->Volume->bRecognized) {

        ULONGLONG   totalSize, freeSize;

        s.Format("      Filesystem: %s\r\n", part->Volume->FileSystem);
        ret += s;

        if ((part->Volume->EVP.bExt2 || part->Volume->EVP.bExt3)) {
            s = "      codepage:";
            s += part->Volume->EVP.Codepage;
            if (part->Volume->EVP.bReadonly) {
                s += ",Readonly";
            }
        }
        ret += s;
        ret += "\r\n";

        totalSize = part->Volume->FssInfo.TotalAllocationUnits.QuadPart;
        freeSize  = part->Volume->FssInfo.AvailableAllocationUnits.QuadPart;
        totalSize = totalSize * part->Volume->FssInfo.BytesPerSector *
                    part->Volume->FssInfo.SectorsPerAllocationUnit;
        freeSize  = freeSize * part->Volume->FssInfo.BytesPerSector *
                    part->Volume->FssInfo.SectorsPerAllocationUnit;
        s.Format("      Size: %I64u\r\n", totalSize);
        ret += s;

        s.Format("      Free: %I64u\r\n", freeSize);
        ret += s;
    } 

    return ret;
}

CString
Ext2DiskInformation(PEXT2_DISK  disk)
{
    CString     s, ret="";

    s.Format("\r\nDisk %d: %s\r\n\r\n", disk->OrderNo, disk->Name);
    ret += s;

    if (disk->SDD.VendorIdOffset) {
        ret += "\r\n  VendorId: ";
        ret += (PCHAR)&disk->SDD + disk->SDD.VendorIdOffset;
        ret += "\r\n";
    }

    if (disk->SDD.ProductIdOffset) {
        ret += "    ProductId: ";
        ret += (PCHAR)&disk->SDD + disk->SDD.ProductIdOffset;
        ret += "\r\n";
    }

    if (disk->SDD.SerialNumberOffset) {
        ret += "    SerialNumber: ";
        ret += (PCHAR)&disk->SDD + disk->SDD.SerialNumberOffset;
        ret += "\r\n";
    }

    ret += "    BusType: ";
    ret += BusTypeString(disk->SDD.BusType);
    ret += "\r\n";

    ret += "    Media Type: ";
    if (disk->SDD.RemovableMedia) {
        ret += " Removable\r\n";
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
            } else if (disk->Layout->PartitionStyle == PARTITION_STYLE_GPT) {
                 s = "GPT";
            }
        }
        s   += "\r\n";
        ret += s;
    }

    if (disk->bEjected) {
        ret += "    No media\r\n";
    } else {
        /* geometry information */
        ret +=   "    DiskGeometry Layout:\r\n";
        s.Format("      BytesPerSector = %u\r\n", disk->DiskGeometry.BytesPerSector);
        ret += s;
        s.Format("      SectorsPerTrack = %u\r\n", disk->DiskGeometry.SectorsPerTrack);
        ret += s;
        s.Format("      TracksPerCylinder = %u\r\n", disk->DiskGeometry.TracksPerCylinder);
        ret += s;
        s.Format("      Cylinderst = %I64u\r\n", disk->DiskGeometry.Cylinders.QuadPart);
        ret += s;

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

        ret += "    MediaType: ";
        ret += s;
        ret += "\r\n";
    }
 
    ret += "\r\n";
    if (disk->Layout) {
        s.Format("    Partition Numbers: %d\r\n", disk->NumParts);
        ret += s;

        for (UCHAR i=0; i < disk->NumParts; i++) {
            ret += Ext2PartInformation(&disk->DataParts[i]);
            ret += "\r\n";
        }
    }

    return ret;
}

CString
Ext2CdromInformation(PEXT2_CDROM cdrom)
{
    CString     s, ret="";

    s.Format("\r\nCdrom %d: %s\r\n\r\n", cdrom->OrderNo, cdrom->Name);
    ret += s;

    if (cdrom->SDD.VendorIdOffset) {
        ret += "    VendorId: ";
        ret += (PCHAR)&cdrom->SDD + cdrom->SDD.VendorIdOffset;
        ret += "\r\n";
    }

    if (cdrom->SDD.ProductIdOffset) {
        ret += "    ProductId: ";
        ret += (PCHAR)&cdrom->SDD + cdrom->SDD.ProductIdOffset;
        ret += "\r\n";
    }

    if (cdrom->SDD.SerialNumberOffset) {
        ret += "    SerialNumber: ";
        ret += (PCHAR)&cdrom->SDD + cdrom->SDD.SerialNumberOffset;
        ret += "\r\n";
    }

    ret += "    BusType: ";
    ret += BusTypeString(cdrom->SDD.BusType);
    ret += "\r\n";

    if (cdrom->bLoaded) {
        ret += "    Media Type: ";
        if (cdrom->bIsDVD) {
            s = "DVD\r\n";
        } else {
            s = "CDROM\r\n";
        }
        ret += s;

        ret +=   "    DiskGeometry Layout:\r\n";
        s.Format("      BytesPerSector = %u\r\n", cdrom->DiskGeometry.BytesPerSector);
        ret += s;
        s.Format("      SectorsPerTrack = %u\r\n", cdrom->DiskGeometry.SectorsPerTrack);
        ret += s;
        s.Format("      TracksPerCylinder = %u\r\n", cdrom->DiskGeometry.TracksPerCylinder);
        ret += s;
        s.Format("      Cylinderst = %I64u\r\n", cdrom->DiskGeometry.Cylinders.QuadPart);
        ret += s;
    } else {
        s = "    No media\r\n";
        ret += s;
    }

    if (cdrom->bLoaded) {
        if (cdrom->bEjected) {
	        ret += "    Media ejected\r\n";
        } else {
            ret += "    File system: ";
            if (cdrom->EVP.bExt2) {
                s = "EXT";
                s += (CHAR)('2' + cdrom->EVP.bExt3);
            } else {
                s = "CDFS";
            }
            ret += s;
            ret += "\r\n";

            s = "    Online,";
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
            ret += s;
            ret += "\r\n";
        }
    } else {
        ret += "    Device stopped\r\n";
    }

    ret += "    Mountpoints: ";
    ret += Ext2QueryVolumeLetterStrings(cdrom->DrvLetters, NULL);
    ret += "\r\n";

    return ret;
}

CString
Ext2VolumeInformation(PEXT2_VOLUME vol)
{
    CString s, ret = "";

    s.Format("\r\nVolume: %s:\r\n\r\n", vol->Name);
    ret += s;

    ret += "    Filesystem: ";
    ret += vol->FileSystem;
    ret += "\r\n";

    /* mount points */
    ret += "    Mountpoints: ";
    ret += Ext2QueryVolumeLetterStrings(vol->DrvLetters, NULL);
    ret += "\r\n";

    /* set volume status */
    s = "    Volume status: Online";
    if (vol->bRecognized && (vol->EVP.bExt2 || vol->EVP.bExt3)) {
        s += ",codepage:";
        s += vol->EVP.Codepage;
        if (vol->EVP.bReadonly) {
            s += ",Readonly";
        }
    }
    ret += s;
    ret += "\r\n";

    {
        ULONGLONG   totalSize, freeSize;
        totalSize = vol->FssInfo.TotalAllocationUnits.QuadPart;
        freeSize  = vol->FssInfo.AvailableAllocationUnits.QuadPart;
        totalSize = totalSize * vol->FssInfo.BytesPerSector *
                    vol->FssInfo.SectorsPerAllocationUnit;
        freeSize  = freeSize * vol->FssInfo.BytesPerSector *
                    vol->FssInfo.SectorsPerAllocationUnit;
        s.Format("    size: %I64u\r\n", totalSize);
        ret += s;

        s.Format("    free space: %I64u\r\n", freeSize);
        ret += s;
    }

    if (vol->Extent) {
        for (DWORD i=0; i < vol->Extent->NumberOfDiskExtents; i++) {
            s.Format("    Extent: %d\r\n", i);
            ret += s;
            s.Format("      DiskNumber: %d\r\n", vol->Extent->Extents[i].DiskNumber);
            ret += s;
            s.Format("      StartingOffset: %I64u\r\n", vol->Extent->Extents[i].StartingOffset.QuadPart);
            ret += s;
            s.Format("      ExtentLength: %I64u\r\n", vol->Extent->Extents[i].ExtentLength.QuadPart);
            ret += s;
        }
    }

    return ret;
}


CString
Ext2SysInformation()
{
    ULONG   i = 0;
    CString s;
    PEXT2_VOLUME  chain = gVols;

    s = "\r\nDisk devices:\r\n";
    for (i=0; i < g_nDisks; i++) {
        s += Ext2DiskInformation(&gDisks[i]);
    }

    s += "\r\nCdrom/DVD devices:\r\n";

    for (i=0; i < g_nCdroms; i++) {
        s += Ext2CdromInformation(&gCdroms[i]);
    }

    while (chain) {
        s += Ext2VolumeInformation(chain);
        chain = chain->Next;
    }

    return s;
}

VOID
Ext2CleanupDisks()
{
    ULONG   i = 0;

    for (i=0; i < g_nDisks; i++) {
        if (gDisks[i].bLoaded) {
            if (gDisks[i].Layout) {
                free(gDisks[i].Layout);
                gDisks[i].Layout = NULL;
            }
            gDisks[i].bLoaded = FALSE;
            if (gDisks[i].DataParts) {
                free(gDisks[i].DataParts);
            }
        }
    }

    g_nDisks = 0;
    if (gDisks) {
        free(gDisks); gDisks = NULL;
    }
}

BOOL
Ext2LoadCdroms()
{
    ULONG           i = 0, j = 0;
    NT::NTSTATUS    status;
    DWORD           mediaType;

    if (g_nCdroms == 0) {
        return TRUE;
    }

    gCdroms = (PEXT2_CDROM) malloc(sizeof(EXT2_CDROM) * g_nCdroms);
    if (gCdroms == NULL) {
        return FALSE;
    }
    memset(gCdroms, 0, sizeof(EXT2_CDROM) * g_nCdroms);

    while (i < g_nCdroms && j < 256) {

        HANDLE          Handle = NULL;

        gCdroms[i].Magic[0] = EXT2_CDROM_DEVICE_MAGIC;
        gCdroms[i].Magic[1] = EXT2_CDROM_VOLUME_MAGIC;
        gCdroms[i].OrderNo = (UCHAR) j;
        gCdroms[i].bLoaded = FALSE;
        sprintf(gCdroms[i].Name, "\\Device\\Cdrom%d\0", j);

        j++;

        status = Ext2Open(gCdroms[i].Name, &Handle, EXT2_DESIRED_ACCESS);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        status = Ext2QueryProperty( Handle, StorageDeviceProperty,
                                    (PVOID)&gCdroms[i].SDD,
                                    sizeof(STORAGE_DEVICE_DESCRIPTOR)
                                  );
        if (!NT_SUCCESS(status)) {
            goto Next;
        }

        status = Ext2QueryProperty( Handle, StorageAdapterProperty,
                                    (PVOID)&gCdroms[i].SAD,
                                    sizeof(STORAGE_ADAPTER_DESCRIPTOR)
                                  );
        if (!NT_SUCCESS(status)) {
            goto Next;
        }

        status = Ext2QueryDisk(Handle, &gCdroms[i].DiskGeometry);
        if (NT_SUCCESS(status)) {
            gCdroms[i].bEjected = FALSE;
        } else {
            // (status == STATUS_NO_MEDIA_IN_DEVICE) {
            gCdroms[i].bEjected = TRUE;
        }

        status = Ext2QueryMediaType(Handle, &mediaType);
        if (NT_SUCCESS(status) && mediaType == FILE_DEVICE_DVD) {
            gCdroms[i].bIsDVD = TRUE;
        } else {
            gCdroms[i].bIsDVD = FALSE;
        }

        if (!gCdroms[i].bEjected) {
            Ext2QueryExt2Property(Handle, &gCdroms[i].EVP);
        }

        gCdroms[i].bLoaded = TRUE;

Next:

        Ext2Close(&Handle);
        i++;
    }

    g_nCdroms = i;
    return TRUE;
}

VOID
Ext2LoadCdromDrvLetters()
{
    for (ULONG i = 0; i < g_nCdroms; i++) {
        gCdroms[i].DrvLetters = Ext2QueryCdromDrvLetters(&gCdroms[i]);
    }
}


VOID
Ext2CleanupCdroms()
{
    ULONG   i = 0;

    for (i=0; i < g_nCdroms; i++) {
        if (gCdroms[i].bLoaded) {
            gCdroms[i].bLoaded = FALSE;
        }
    }

    g_nCdroms = 0;
    if (gCdroms) {
        free(gCdroms); gCdroms = NULL;
    }
}

BOOL
Ext2CompareExtents(
    PVOLUME_DISK_EXTENTS ext1,
    PVOLUME_DISK_EXTENTS ext2
    )
{
    DWORD nExt;

    if (ext1->NumberOfDiskExtents != ext2->NumberOfDiskExtents) {
        return FALSE;
    }

    for (nExt = 0; nExt < ext1->NumberOfDiskExtents; nExt++) {
        if ((ext1->Extents[nExt].DiskNumber != ext2->Extents[nExt].DiskNumber) ||
            (ext1->Extents[nExt].StartingOffset.QuadPart != ext2->Extents[nExt].StartingOffset.QuadPart) ||
            (ext1->Extents[nExt].ExtentLength.QuadPart != ext2->Extents[nExt].ExtentLength.QuadPart) ) {
            return FALSE;
        }
    }
    return TRUE;
}

ULONGLONG
Ext2EjectedDiskLetters(
    PEXT2_DISK   Disk
    )
{
    ULONGLONG letters = 0;
    int i;

    /* checking the digits ltters */
    for (i=0; i < 10; i++) {
        if (drvDigits[i].bUsed && drvDigits[i].SDN) {
            if (drvDigits[i].SDN->DeviceNumber == Disk->OrderNo) {
                letters |= (((ULONGLONG) 1) << (32 + i));
            }
        }
    }

    for (i=0; i <26; i++) {
        if (drvLetters[i].bUsed && drvLetters[i].SDN) {
            if (drvLetters[i].SDN->DeviceNumber == Disk->OrderNo) {
                letters |= (((ULONGLONG) 1) << i);
            }
        }
    }

    return letters;
}

ULONGLONG
Ext2QueryVolumeDrvLetters(PEXT2_VOLUME Volume)
{
    ULONGLONG letters = 0;
    int i;

    UCHAR   drvChar = Ext2QueryMountPoint(Volume->Name);
    if (drvChar) {
        letters |= (((ULONGLONG) 1) << (drvChar - 'A'));
    }

    if (!Volume->Extent) {
        goto errorout;
    }

    /* checking the digits ltters */
    for (i=0; i < 10; i++) {
        if (drvDigits[i].bUsed && drvDigits[i].Extent) {
            if (Ext2CompareExtents(drvDigits[i].Extent, Volume->Extent)) {
                letters |= (((ULONGLONG) 1) << (32 + i));
                if (drvChar != drvDigits[i].Letter) {
                    drvDigits[i].bTemporary = TRUE;
                }
            }
        }
    }

    for (i=0; i <26; i++) {
        if (drvLetters[i].bUsed && drvLetters[i].Extent) {
            if (Ext2CompareExtents(drvLetters[i].Extent, Volume->Extent)) {
                letters |= (((ULONGLONG) 1) << i);
                if (drvChar != drvLetters[i].Letter) {
                    drvLetters[i].bTemporary = TRUE;
                }
            }
        }
    }

errorout:

    return letters;
}

ULONGLONG
Ext2QueryCdromDrvLetters(PEXT2_CDROM Cdrom)
{
    ULONGLONG letters = 0;
    UCHAR   drvChar;
    int i;

    if (!Cdrom) {
        goto errorout;
    }

    drvChar = Ext2QueryMountPoint(Cdrom->Name);
    if (drvChar) {
        letters |= (((ULONGLONG) 1) << (drvChar - 'A'));
    }

    /* checking the digits ltters */
    for (i=0; i < 10; i++) {
        if (drvDigits[i].bUsed && drvDigits[i].DrvType == DRIVE_CDROM) {
            if (!_stricmp(drvDigits[i].SymLink, Cdrom->Name)) {
                letters |= (((ULONGLONG) 1) << (i + 32));
            }
        }
    }

    for (i=0; i <26; i++) {
        if (drvLetters[i].bUsed &&  drvLetters[i].DrvType == DRIVE_CDROM) {
            if (!_stricmp(drvLetters[i].SymLink, Cdrom->Name)) {
                letters |= (((ULONGLONG) 1) << i);
            }
        }
    }

errorout:

    return letters;
}

BOOL
Ext2QueryVolumeFS(
    HANDLE          Handle,
    PEXT2_VOLUME    volume
    )
{
    struct ext4_super_block *sb = NULL;
	union swap_header*	swap = NULL;
    PUCHAR  buffer = NULL;

    NT::NTSTATUS          status;

    buffer = (PUCHAR)malloc(PAGE_SIZE);
    if (!buffer) {
        return FALSE;
    }

    swap = (union swap_header*)&buffer[SWAP_HEADER_OFFSET];
    sb = (struct ext4_super_block *)&buffer[SUPER_BLOCK_OFFSET];

    status = Ext2Read( Handle, FALSE, volume->FssInfo.BytesPerSector, 
                       (ULONGLONG)0,PAGE_SIZE, (PUCHAR) buffer);

    if (!NT_SUCCESS(status)) {
        free(buffer);
        return FALSE;
    }

    if (sb->s_magic == EXT4_SUPER_MAGIC) {

        volume->FsaInfo.FileSystemNameLength = 8;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'E';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'X';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'T';

        if (sb->s_feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS) {
            volume->FsaInfo.FileSystemName[3] = (WCHAR)'4';
            volume->EVP.bExt3 = TRUE;
            // Add a "+" after the filesystem name, e.g "EXT4+", if the ondisk filesystem
            // contains features not yet supported by the Windows driver.
            if (sb->s_feature_incompat & ~EXT4_FEATURE_INCOMPAT_SUPP) {
                volume->FsaInfo.FileSystemName[4] = (WCHAR)'+';
                volume->FsaInfo.FileSystemNameLength += 2;
            }
        } else if ((sb->s_feature_incompat & EXT4_FEATURE_INCOMPAT_JOURNAL_DEV) ||
            (sb->s_feature_incompat & EXT4_FEATURE_INCOMPAT_RECOVER) ||
            (sb->s_feature_compat & EXT4_FEATURE_COMPAT_HAS_JOURNAL)) {
            volume->FsaInfo.FileSystemName[3] = (WCHAR)'3';
            volume->EVP.bExt3 = TRUE;
        } else {
            volume->FsaInfo.FileSystemName[3] = (WCHAR)'2';
            volume->EVP.bExt2 = TRUE;
        }
        memcpy(&volume->EVP.UUID[0], &sb->s_uuid[0], 16);
        goto errorout;
    }

    if ((memcmp(swap->magic.magic, SWAP_HEADER_MAGIC_V1, 10) == 0) ||
        (memcmp(swap->magic.magic, SWAP_HEADER_MAGIC_V2, 10) == 0)) {
        volume->FsaInfo.FileSystemNameLength = 8;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'S';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'W';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'A';
        volume->FsaInfo.FileSystemName[3] = (WCHAR)'P';
        // Show swap partitions as unused.
        volume->FssInfo.AvailableAllocationUnits = volume->FssInfo.TotalAllocationUnits;
        volume->bRecognized = TRUE;
        goto errorout;
    }

    if (*((PULONG)&buffer[XFS_MAGIC_OFFSET]) == XFS_SB_MAGIC_LE) {
        volume->FsaInfo.FileSystemNameLength = 6;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'X';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'F';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'S';
        goto errorout;
    }

    // The LVM superblock is stored in one of the first 4 sectors as an option to pvcreate
    if ((memcmp(&buffer[0*512], LVM_MAGIC, 8) == 0) ||
        (memcmp(&buffer[1*512], LVM_MAGIC, 8) == 0) ||
        (memcmp(&buffer[2*512], LVM_MAGIC, 8) == 0) ||
        (memcmp(&buffer[3*512], LVM_MAGIC, 8) == 0)) {
        volume->FsaInfo.FileSystemNameLength = 6;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'L';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'V';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'M';
        goto errorout;
    }

    if (*((PULONG)&buffer[512]) == BSD_DISKMAGIC) {
        volume->FsaInfo.FileSystemNameLength = 6;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'B';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'S';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'D';
        goto errorout;
    }

    Ext2Read(Handle, FALSE, 512, (ULONGLONG)BTRFS_SUPER_BLOCK_OFFSET, 4096, buffer);

    if (*((PULONGLONG)&buffer[BTRFS_MAGIC_OFFSET]) == BTRFS_MAGIC) {
        volume->FsaInfo.FileSystemNameLength = 10;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'B';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'T';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'R';
        volume->FsaInfo.FileSystemName[3] = (WCHAR)'F';
        volume->FsaInfo.FileSystemName[4] = (WCHAR)'S';
        goto errorout;
    }

    Ext2Read(Handle, FALSE, 512, (ULONGLONG)RAID_SUPER_BLOCK_OFFSET, 4096, buffer);

    if (*((PULONG)&buffer[RAID_MAGIC_OFFSET]) == RAID_MAGIC) {
        volume->FsaInfo.FileSystemNameLength = 8;
        volume->FsaInfo.FileSystemName[0] = (WCHAR)'R';
        volume->FsaInfo.FileSystemName[1] = (WCHAR)'A';
        volume->FsaInfo.FileSystemName[2] = (WCHAR)'I';
        volume->FsaInfo.FileSystemName[3] = (WCHAR)'D';
    }

errorout:

    free(buffer);
    return (volume->EVP.bExt2 || volume->EVP.bExt3);
}

BOOL
Ext2QueryExt2Property (
    HANDLE                      Handle,
    PEXT2_VOLUME_PROPERTY3      EVP
    )
{
    NT::NTSTATUS                status;
    NT::IO_STATUS_BLOCK         iosb;

    BOOLEAN                     bExt2, bExt3;
    CHAR                        UUID[16];

    if (!Ext2IsServiceStarted()) {
        return FALSE;
    }

    bExt2 = EVP->bExt2;
    bExt3 = EVP->bExt3;
    memcpy(&UUID[0], &EVP->UUID[0], 16);
    memset(EVP, 0, sizeof(EXT2_VOLUME_PROPERTY2));
    memcpy(&EVP->UUID[0], &UUID[0], 16);
    EVP->bExt2 = bExt2;
    EVP->bExt3 = bExt3;
    EVP->Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    EVP->Command = APP_CMD_QUERY_PROPERTY3;

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &iosb,
                IOCTL_APP_VOLUME_PROPERTY,
                EVP, sizeof(EXT2_VOLUME_PROPERTY3),
                EVP, sizeof(EXT2_VOLUME_PROPERTY3)
            );

    return NT_SUCCESS(status);
}


BOOL
Ext2QueryPerfStat (
    HANDLE                      Handle,
    PEXT2_QUERY_PERFSTAT        Stat,
    PEXT2_PERF_STATISTICS_V1   *PerfV1,
    PEXT2_PERF_STATISTICS_V2   *PerfV2
    )
{
    NT::NTSTATUS                status;
    NT::IO_STATUS_BLOCK         iosb;

    memset(Stat, 0, sizeof(EXT2_QUERY_PERFSTAT));
    Stat->Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    Stat->Command = IOCTL_APP_QUERY_PERFSTAT;

    *PerfV1 = NULL;
    *PerfV2 = NULL;

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &iosb,
                IOCTL_APP_QUERY_PERFSTAT,
                Stat, sizeof(EXT2_QUERY_PERFSTAT),
                Stat, sizeof(EXT2_QUERY_PERFSTAT)
            );
    if (!NT_SUCCESS(!status))
        return FALSE;

    if (iosb.Information == EXT2_QUERY_PERFSTAT_SZV2 && 
        (Stat->Flags & EXT2_QUERY_PERFSTAT_VER2) != 0) {

        if (Stat->PerfStatV2.Magic == EXT2_PERF_STAT_MAGIC && 
            Stat->PerfStatV2.Length == sizeof(EXT2_PERF_STATISTICS_V2) &&
            Stat->PerfStatV2.Version == EXT2_PERF_STAT_VER2) {
            *PerfV2 = &Stat->PerfStatV2;
        }

    } else if (iosb.Information >= EXT2_QUERY_PERFSTAT_SZV1)  {

        *PerfV1 = &Stat->PerfStatV1;
    }

    if (PerfV1 || PerfV2)
        return TRUE;

    return FALSE;
}

VOID
Ext2StorePropertyinRegistry(PEXT2_VOLUME_PROPERTY3 EVP)
{
    CHAR    UUID[50], ID[16];
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH];
    LONG    status;
    CString data = "";

    int     i;
    int     len = 0;

    memset(UUID, 0, 50);
    for (i=0; i < 16; i++) {
        if (i == 0) {
            sprintf(&UUID[len], "{%2.2X", EVP->UUID[i]);
            len += 3;
        } else if (i == 15) {
            sprintf(&UUID[len], "-%2.2X}", EVP->UUID[i]);
            len +=4;
        } else {
            sprintf(&UUID[len], "-%2.2X", EVP->UUID[i]);
            len += 3;
        }
    }

    /* Create or open ext2fsd volumes key */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Volumes") ;
    status = ::RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            0,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL);
    if (status != ERROR_SUCCESS) {
        return;
    }

#define READING_ONLY        "Readonly"
#define WRITING_SUPPORT     "WritingSupport"
#define EXT3_FORCEWRITING   "Ext3ForceWriting"
#define CODEPAGE_NAME       "CodePage"
#define HIDING_PREFIX       "HidingPrefix"
#define HIDING_SUFFIX       "HidingSuffix"
#define MOUNT_POINT         "MountPoint"
#define UID                 "uid"
#define GID                 "gid"
#define EUID                "euid"
#define EGID                "egid"


    if (EVP->bReadonly) {
        data += READING_ONLY";";
    } else if (EVP->bExt3 && EVP->bExt3Writable) {
        data += EXT3_FORCEWRITING";";
    }

    if (EVP->DrvLetter) {
        if ((EVP->DrvLetter & 0x7F) == 0 || EVP->DrvLetter == 0xFF) {
            data += MOUNT_POINT";";
        } else {
            data += MOUNT_POINT"=";
            data += (CHAR)(EVP->DrvLetter & 0x7F);
            data += ":;";
        }
    }

    if (strlen((CHAR*)EVP->Codepage) > 0) {
        data += CODEPAGE_NAME"=";
        data += &EVP->Codepage[0];
        data += ";";
    }

    if (EVP->bHidingPrefix) {
        data += HIDING_PREFIX"=";
        data += EVP->sHidingPrefix;
        data += ";";
    }

    if (EVP->bHidingSuffix) {
        data += HIDING_SUFFIX"=";
        data += EVP->sHidingSuffix;
        data += ";";
    }

    if (EVP->Flags2 & EXT2_VPROP3_USERIDS) {
        sprintf(ID, "%u", EVP->uid);
        data += UID"=";
        data += ID;
        data += ";";

        sprintf(ID, "%u", EVP->gid);
        data += GID"=";
        data += ID;
        data += ";";

        if (EVP->EIDS) {
            sprintf(ID, "%u", EVP->euid);
            data += EUID"=";
            data += ID;
            data += ";";
        }
    }

    /* set volume parameters */
    status = RegSetValueEx( hKey, UUID, 0, REG_SZ, (BYTE *)
                            data.GetBuffer(data.GetLength()),
                            data.GetLength());

    RegCloseKey(hKey);
}

BOOL Ext2IsNullUuid (__u8 * uuid)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (uuid[i]) {
            break;
        }
    }

    return (i >= 16);
}

BOOL
Ext2CheckVolumeRegistryProperty(PEXT2_VOLUME_PROPERTY3 EVP)
{
    CHAR    UUID[50];
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH];
    CHAR    content[MAX_PATH];
    LONG    status, type = 0;

    int     i;
    int     len = 0;
    BOOL rc = TRUE;

    if (Ext2IsNullUuid(&EVP->UUID[0])) {
        return TRUE;
    }

    memset(UUID, 0, 50);
    for (i=0; i < 16; i++) {
        if (i == 0) {
            sprintf(&UUID[len], "{%2.2X", EVP->UUID[i]);
            len += 3;
        } else if (i == 15) {
            sprintf(&UUID[len], "-%2.2X}", EVP->UUID[i]);
            len +=4;
        } else {
            sprintf(&UUID[len], "-%2.2X", EVP->UUID[i]);
            len += 3;
        }
    }

    /* Create or open ext2fsd volumes key */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Volumes") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* Query volume parameters */
    len = MAX_PATH;
    status = RegQueryValueEx(hKey,
                            &UUID[0],
                            0,
                            (LPDWORD)&type,
                            (BYTE *)&content[0],
                            (LPDWORD)&len);
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
    }

    RegCloseKey(hKey);

errorout:
    return rc;
}

VOID
Ext2SetDefaultVolumeRegistryProperty(PEXT2_VOLUME_PROPERTY3 EVP)
{
    ULONG   StartMode;
    BOOL AutoMount = 0;

    if (Ext2IsNullUuid(&EVP->UUID[0])) {
        return;
    }

    /* query global parameters */
    Ext2QueryGlobalProperty(
            &StartMode,
            (BOOL *)&EVP->bReadonly,
            (BOOL *)&EVP->bExt3Writable,
            (CHAR *)EVP->Codepage,
            (CHAR *)NULL,
            (CHAR *)NULL,
            (BOOL *)&AutoMount
            );

    if (EVP->bExt3 && !EVP->bExt3Writable)
        EVP->bReadonly = TRUE;

    EVP->DrvLetter = 0x80;
    EVP->Flags2 |= EXT2_VPROP3_AUTOMOUNT;
    Ext2StorePropertyinRegistry(EVP);
}

BOOL
Ext2SetExt2Property (
    HANDLE                Handle,
    PEXT2_VOLUME_PROPERTY3 EVP
    )
{
    NT::NTSTATUS                status;
    NT::IO_STATUS_BLOCK         iosb;

    ASSERT(EVP->Magic == EXT2_VOLUME_PROPERTY_MAGIC);
    EVP->Command = APP_CMD_SET_PROPERTY3;

    status = NT::ZwDeviceIoControlFile(
                Handle, NULL, NULL, NULL, &iosb,
                IOCTL_APP_VOLUME_PROPERTY,
                EVP, sizeof(EXT2_VOLUME_PROPERTY3),
                EVP, sizeof(EXT2_VOLUME_PROPERTY3)
            );

    if (NT_SUCCESS(status)) {
        return TRUE;
    } else {
        CString s;
        s.Format("Status = %xh\n", status);
        AfxMessageBox(s);
    }

    return FALSE;
}


BOOL
Ext2QueryGlobalProperty(
    ULONG *     ulStartup,
    BOOL *   bReadonly,
    BOOL *   bExt3Writable,
    CHAR *      Codepage,
    CHAR *      sPrefix,
    CHAR *      sSuffix,
    BOOL *   bAutoMount
    )
{
    int     rc = TRUE;
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH];
    DWORD   data = 0;
    LONG    status, type, len;

    /* Open ext2fsd sevice key */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* Query Start type */
    len = sizeof(DWORD);
    status = RegQueryValueEx( hKey,
                            "Start",
                            0,
                            (LPDWORD)&type,
                            (BYTE *)&data,
                            (LPDWORD)&len);
    if (status == ERROR_SUCCESS) {
        *ulStartup = data;
    }

    RegCloseKey(hKey);

    /* Open ext2fsd parameters key */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Parameters") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* Query WritingSupport */
    len = sizeof(DWORD);
    status = RegQueryValueEx( hKey,
                            "WritingSupport",
                            0,
                            (LPDWORD)&type,
                            (BYTE *)&data,
                            (LPDWORD)&len);
    if (status == ERROR_SUCCESS) {
        *bReadonly = (data == 0);
    }

    /* query Ext3ForceWriting */
    len = sizeof(DWORD);
    status = RegQueryValueEx( hKey,
                            "Ext3ForceWriting",
                            0,
                            (LPDWORD)&type,
                            (BYTE *)&data,
                            (LPDWORD)&len);

    if (status == ERROR_SUCCESS) {
        *bExt3Writable = (data != 0);
    }

    /* query AutoMount */
    len = sizeof(DWORD);
    status = RegQueryValueEx( hKey,
                            "AutoMount",
                            0,
                            (LPDWORD)&type,
                            (BYTE *)&data,
                            (LPDWORD)&len);

    if (status == ERROR_SUCCESS) {
        *bAutoMount = (data != 0);
    }

    if (Codepage) {
        /* query codepage */
        len = CODEPAGE_MAXLEN;
        status = RegQueryValueEx( hKey,
                                "CodePage",
                                0,
                                (LPDWORD)&type,
                                (BYTE *)Codepage,
                                (LPDWORD)&len);
    }

    if (sPrefix) {
        /* querying hidding filter patterns */
        len = CODEPAGE_MAXLEN;
        status = RegQueryValueEx( hKey,
                                "HidingPrefix",
                                0,
                                (LPDWORD)&type,
                                (BYTE *)sPrefix,
                                (LPDWORD)&len);
    }

    if (sSuffix) {
        len = CODEPAGE_MAXLEN;
        status = RegQueryValueEx( hKey,
                                "HidingSuffix",
                                0,
                                (LPDWORD)&type,
                                (BYTE *)sSuffix,
                                (LPDWORD)&len);
    }

    RegCloseKey(hKey);


errorout:

    return rc;
}

INT
Ext2QueryDrvVersion(
    CHAR *      Version,
    CHAR *      Date,
    CHAR *      Time
    )
{
    EXT2_VOLUME_PROPERTY_VERSION EVPV;
    NT::NTSTATUS                 status;
    HANDLE                       handle = NULL;
    NT::IO_STATUS_BLOCK          iosb;
    INT                          rc = 0;

    memset(&EVPV, 0, sizeof(EXT2_VOLUME_PROPERTY_VERSION));
    EVPV.Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    EVPV.Command = APP_CMD_QUERY_VERSION;
    EVPV.Flags |= EXT2_FLAG_VP_SET_GLOBAL;

    status = Ext2Open("\\DosDevices\\Ext2Fsd", 
                      &handle, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        rc = -1;
        goto errorout;
    }

    status = NT::ZwDeviceIoControlFile(
                handle, NULL, NULL, NULL, &iosb,
                IOCTL_APP_VOLUME_PROPERTY,
                &EVPV, sizeof(EXT2_VOLUME_PROPERTY_VERSION),
                &EVPV, sizeof(EXT2_VOLUME_PROPERTY_VERSION)
            );

    if (NT_SUCCESS(status)) {
        strncpy(Version,  EVPV.Version, 0x1B);
        strncpy(Date,     EVPV.Date,    0x1F);
        strncpy(Time,     EVPV.Time,    0x1F);
    }

    rc = NT_SUCCESS(status);

errorout:

    Ext2Close(&handle);

    return rc;
}

BOOL
Ext2SetGlobalProperty (
    ULONG       ulStartup,
    BOOLEAN     bReadonly,
    BOOLEAN     bExt3Writable,
    CHAR *      Codepage,
    CHAR *      sPrefix,
    CHAR *      sSuffix,
    BOOL     bAutoMount
    )
{
    EXT2_VOLUME_PROPERTY3 EVP;

    NT::NTSTATUS         status;
    HANDLE  Handle = NULL;
    int     rc = TRUE;
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH] ;

    ULONG   data = 0;

    memset(&EVP, 0, sizeof(EXT2_VOLUME_PROPERTY3));
    EVP.Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    EVP.Command = APP_CMD_SET_PROPERTY3;
    EVP.Flags |= EXT2_FLAG_VP_SET_GLOBAL;
    EVP.bReadonly = bReadonly;
    EVP.bExt3Writable = bExt3Writable;
    strcpy((CHAR *)EVP.Codepage, Codepage);

    if (strlen(sPrefix)) {
        strcpy(EVP.sHidingPrefix, sPrefix);
        EVP.bHidingPrefix = TRUE;
    }

    if (strlen(sSuffix)) {
        strcpy(EVP.sHidingSuffix, sSuffix);
        EVP.bHidingSuffix = TRUE;
    }

    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* set Start type */
    status = RegSetValueEx( hKey,
                            "Start",
                            0,
                            REG_DWORD,
                            (BYTE *)&ulStartup,
                            sizeof(DWORD));
    RegCloseKey(hKey);

    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Parameters") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* set WritingSupport */
    data = !bReadonly;
    status = RegSetValueEx( hKey,
                            "WritingSupport",
                            0,
                            REG_DWORD,
                            (BYTE *)&data,
                            sizeof(ULONG));

    /* set Ext3ForceWriting */
    data = bExt3Writable;
    status = RegSetValueEx( hKey,
                            "Ext3ForceWriting",
                            0,
                            REG_DWORD,
                            (BYTE *)&data,
                            sizeof(ULONG));
    /* set AutoMount */
    data = bAutoMount;
    status = RegSetValueEx( hKey,
                            "AutoMount",
                            0,
                            REG_DWORD,
                            (BYTE *)&data,
                            sizeof(ULONG));


    /* set codepage */
    status = RegSetValueEx( hKey,
                            "CodePage",
                            0,
                            REG_SZ,
                            (BYTE *)Codepage,
                            (int)strlen(Codepage));

    /* set hiding filter patterns */
    status = RegSetValueEx( hKey,
                            "HidingPrefix",
                            0,
                            REG_SZ,
                            (BYTE *)sPrefix,
                            (int)strlen(sPrefix));

    status = RegSetValueEx( hKey,
                            "HidingSuffix",
                            0,
                            REG_SZ,
                            (BYTE *)sSuffix,
                            (int)strlen(sSuffix));

    RegCloseKey(hKey);

    status = Ext2Open("\\DosDevices\\Ext2Fsd", &Handle, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    EVP.Flags2 = EXT2_VPROP3_AUTOMOUNT;
    EVP.AutoMount = g_bAutoMount;

    rc = Ext2SetExt2Property(Handle, &EVP);
    if (!rc) {
    }

errorout:

    Ext2Close(&Handle);

    return rc;
}

BOOL
Ext2SetService(PCHAR Target, PCHAR Name, PCHAR Desc, BOOL bInstall)
{
    SC_HANDLE   hService;
    SC_HANDLE   hManager;

    // open Service Control Manager
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL) {
        // AfxMessageBox("Ext2Mgr: cannot open Service Control Manager",
        //               MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (bInstall) {

        // now create service entry
        hService = CreateService(
                hManager,                   // SCManager database
                Name,                       // name of service
                Desc,                       // name to display
                SERVICE_ALL_ACCESS,	        // desired access
                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                            // service type
                SERVICE_AUTO_START,	        // start type
                SERVICE_ERROR_NORMAL,       // error control type
				Target,	                    // service's binary
                NULL,                       // no load ordering group
                NULL,                       // no tag identifier
                NULL,			            // dependencies
                NULL,						// LocalSystem account
                NULL);                      // no password

        if (hService == NULL) {
            DWORD error = GetLastError();
            if (error == ERROR_SERVICE_EXISTS) {
                // AfxMessageBox("Service is already registered.",
                //               MB_ICONEXCLAMATION | MB_OK);
            } else {
                // AfxMessageBox("Service couldn't be registered.",
                //               MB_ICONEXCLAMATION | MB_OK);
            }
        } else {

            CloseServiceHandle(hService);
        }

    } else {

        /* open the service */
        hService = OpenService(
                hManager,
                Name,
                SERVICE_ALL_ACCESS
                );

        if (hService != NULL) {

            // remove the service from the SCM
            if (DeleteService(hService)) {
            } else {
                DWORD error = GetLastError();
                if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
                    // AfxMessageBox("Service is already unregistered",
                    //               MB_ICONEXCLAMATION | MB_OK);
                } else {
                    // AfxMessageBox("Service could not be unregistered",
                    //                MB_ICONEXCLAMATION | MB_OK);
                }
            }

            CloseServiceHandle(hService);
        }
    }

    CloseServiceHandle(hManager);

	return TRUE;
}


BOOL ExtSaveResourceToFile(UINT id, CHAR *target)
{
	HANDLE	handle;
	ULONG	bytes = 0;
    BOOL rc = FALSE;

    HRSRC   hr = FindResource(NULL, MAKEINTRESOURCE(id), RT_RCDATA);
    ULONG   size = SizeofResource(NULL, hr);
    HGLOBAL hg = LoadResource(NULL, hr);
    PVOID   data = LockResource(hg);

    if (!data)
        goto errorout;

	handle = CreateFile(target, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		goto errorout;
	}

    rc = WriteFile(handle, data, size, &bytes, NULL);
    if (rc)
        rc = (bytes == size);

	CloseHandle(handle);

errorout:

	return rc;
}

TCHAR *Ext2StrLastA(TCHAR *t, TCHAR *s);

BOOL Ext2InstallExt2Srv()
{
    TCHAR   cmd[512] = {0}, *p;
    BOOL    rc = FALSE;

    if (GetWindowsDirectory(cmd, 512)) {
        if (g_isWow64)
            strcat(cmd, "\\SysWOW64\\Ext2Srv.EXE");
        else
            strcat(cmd, "\\System32\\Ext2Srv.EXE");
        rc = ExtSaveResourceToFile(IDR_RCDAT_SRV, cmd);
    }

    if (!rc) {
        GetModuleFileName(NULL, cmd, 510);
        p = Ext2StrLastA(cmd, ".EXE");
        if (p && p > &cmd[3]) {
            p[-3] = 'S';
            p[-2] = 'r';
            p[-1] = 'v';
        } else {
            strcat(cmd, "-Srv.EXE");
        }
        rc = ExtSaveResourceToFile(IDR_RCDAT_SRV, cmd);
    }

    if (!rc) {
        GetTempPath(50, cmd);
        strcat(cmd, "\\Ext2Srv.EXE");
        rc = ExtSaveResourceToFile(IDR_RCDAT_SRV, cmd);
    }

    if (!rc) {
        return rc;
    }

    rc = Ext2SetService(cmd, "Ext2Srv", "Ext2Fsd Service Manager", TRUE);
    if (!rc) {
        return rc;
    }

    return Ext2StartService("Ext2Srv");
}


BOOL
Ext2StartService(CHAR *service)
{
    BOOL     rc = FALSE;
    SC_HANDLE   scmHandle = NULL;
    SC_HANDLE   drvHandle = NULL;

    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scmHandle) {
        goto errorout;
    }

    drvHandle = OpenService(scmHandle,
                            service,
                            SERVICE_ALL_ACCESS);
    if (!drvHandle) {
        goto errorout;
    }

    rc = (0 != StartService(drvHandle, 0, NULL));

errorout:

    if (drvHandle)
        CloseServiceHandle(drvHandle);

    if (scmHandle) {
        CloseServiceHandle(scmHandle);
    }

    return rc;
}

BOOL
Ext2StartExt2Srv()
{
    BOOL rc = Ext2StartService("Ext2Srv");
    if (!rc)
        rc = Ext2InstallExt2Srv();

    return rc;
}


BOOL
Ext2StartExt2Fsd()
{
    if (Ext2IsServiceStarted()) {
        return TRUE;
    }

    return Ext2StartService("ext2fsd");
}

BOOL
Ext2IsServiceStarted()
{
    NT::NTSTATUS         status;
    HANDLE               Handle = NULL;

    status = Ext2Open("\\DosDevices\\Ext2Fsd",
                      &Handle, EXT2_DESIRED_ACCESS);

    Ext2Close(&Handle);
    if (NT_SUCCESS(status)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
Ext2IsRemovable(PEXT2_VOLUME volume)
{
    STORAGE_BUS_TYPE busType = BusTypeAta;
    BOOL bRemovableMedia = FALSE;

    if (volume && volume->Part) {
        busType = volume->Part->Disk->SDD.BusType;
        bRemovableMedia = volume->Part->Disk->SDD.RemovableMedia;
    }

    if (busType == BusType1394 ||
        busType == BusTypeUsb ||
        bRemovableMedia  ) {
        return TRUE;
    }

    return FALSE;
}

BOOL
Ext2RemoveDriveLetter(CHAR DrvLetter)
{
    PEXT2_LETTER drvLetter = NULL;
    BOOL rc = FALSE;

    if (DrvLetter >= '0' && DrvLetter <= '9') {
        drvLetter = &drvDigits[DrvLetter - '0'];
    } else if (DrvLetter >= 'A' && DrvLetter <= 'Z') {
        drvLetter = &drvLetters[DrvLetter - 'A'];
    } else if (DrvLetter >= 'a' && DrvLetter <= 'z') {
        drvLetter = &drvLetters[DrvLetter - 'a'];
    }

    if (drvLetter && drvLetter->bUsed) {
        rc = Ext2RemoveDrvLetter(drvLetter);
    }

    return rc;
}

BOOL
Ext2InitializeVolume(
    PCHAR           NameString,
    PEXT2_VOLUME *  Volume
    )
{
    BOOL rc = FALSE;
    HANDLE hVolume = NULL;
    NT::NTSTATUS status;
    NT::IO_STATUS_BLOCK ioSb;
    PEXT2_VOLUME        volume = NULL;

    volume = (PEXT2_VOLUME) malloc(sizeof(EXT2_VOLUME));
    if (!volume) {
        goto errorout;
    }

    status = Ext2Open(NameString, &hVolume, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        free(volume); volume = NULL;
        goto errorout;
    }

    memset(volume, 0, sizeof(EXT2_VOLUME));
    volume->Magic = EXT2_VOLUME_MAGIC;
    strcpy(volume->Name, NameString);

    status = NT::ZwQueryVolumeInformationFile(
                        hVolume, &ioSb, &volume->FsaInfo,
                        MAX_PATH, NT::FileFsAttributeInformation
                        );

    if (NT_SUCCESS(status)) {

        NT::UNICODE_STRING uniString;
        NT::ANSI_STRING    aniString;

        NT::ZwQueryVolumeInformationFile(
                        hVolume, &ioSb, &volume->FsdInfo,
                        sizeof(NT::FILE_FS_DEVICE_INFORMATION),
                        NT::FileFsDeviceInformation
                        );

        NT::ZwQueryVolumeInformationFile(
                        hVolume, &ioSb, &volume->FssInfo,
                        sizeof(NT::FILE_FS_SIZE_INFORMATION),
                        NT::FileFsSizeInformation
                        );

        if (volume->FsaInfo.FileSystemNameLength == 6 &&
            (volume->FsaInfo.FileSystemName[0] == (WCHAR)'R' ||
             volume->FsaInfo.FileSystemName[0] == (WCHAR)'r') &&
            (volume->FsaInfo.FileSystemName[1] == (WCHAR)'A' ||
             volume->FsaInfo.FileSystemName[1] == (WCHAR)'a') &&
            (volume->FsaInfo.FileSystemName[2] == (WCHAR)'W' ||
             volume->FsaInfo.FileSystemName[2] == (WCHAR)'w')) {
            if (!Ext2QueryVolumeFS(hVolume, volume)) {
                /* memcpy(volume->Codepage, "N/A", 3); */
            }

        } else {
            if (Ext2QueryExt2Property(hVolume, &volume->EVP)) {
            }
            volume->bRecognized = TRUE;
        }

        /* convert the unicode file system name to mbs */

        uniString.MaximumLength = uniString.Length = 
            (USHORT)volume->FsaInfo.FileSystemNameLength; 
        uniString.Buffer = volume->FsaInfo.FileSystemName;

        aniString.Buffer = volume->FileSystem;
        aniString.Length = 0;
        aniString.MaximumLength = 64;

        memset(volume->FileSystem, 0, 64);
        NT::RtlUnicodeStringToAnsiString(&aniString, &uniString, FALSE);
    }

    volume->Extent = Ext2QueryVolumeExtents(hVolume);
    if (!volume->Extent) {
        free(volume); volume = NULL;
        goto errorout;
    }

    *Volume = volume; rc = TRUE;

errorout:

    Ext2Close(&hVolume);
    return rc;
}


CHAR
Ext2ProcessExt2Property(PEXT2_VOLUME volume)
{
    HANDLE hVolume = NULL;
    NT::NTSTATUS status;
    CHAR rc = 0;

    status = Ext2Open(volume->Name, &hVolume, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    if (Ext2QueryExt2Property(hVolume, &volume->EVP)) {
        if (volume->DrvLetters == 0 && volume->EVP.DrvLetter) {
            CHAR DrvLetter = volume->EVP.DrvLetter & 0x7F;
            if (DrvLetter && Ext2IsDrvLetterAvailable(DrvLetter)) {
                rc = Ext2MountVolumeAs(volume->Name, DrvLetter);
            } else {
                rc = volume->EVP.DrvLetter = Ext2MountVolume(volume->Name);
                volume->EVP.DrvLetter |= 0x80;
            }
        }
    }

errorout:

    Ext2Close(&hVolume);
    return rc;
}

BOOL Ext2ProcessExt2Volumes()
{
    PEXT2_VOLUME    volume = gVols;
    BOOL         rc = FALSE;

    while (volume) {
        if (Ext2ProcessExt2Property(volume)) {
            rc = TRUE;
        }
        volume = volume->Next;
    }
    return rc;
}

BOOL
Ext2IsPartitionExtent(
    ULONG                  disk,
    PPARTITION_INFORMATION_EXT part,
    PVOLUME_DISK_EXTENTS   extent
    )
{
    DWORD nExt = 0;

    for (nExt = 0; nExt < extent->NumberOfDiskExtents; nExt++) {
        if ((extent->Extents[nExt].DiskNumber == disk) &&
            (extent->Extents[nExt].StartingOffset.QuadPart == 
             part->StartingOffset.QuadPart ) &&
            (extent->Extents[nExt].ExtentLength.QuadPart ==
             part->PartitionLength.QuadPart)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* should be called after volumes' initiaization */
BOOL
Ext2LoadDiskPartitions(PEXT2_DISK Disk)
{
    UCHAR i = 0, cnt = 0;
    ULONG j = 0;

    BOOL bDynamic = FALSE;

    if (Disk->NumParts == 0) {
        Disk->NumParts = 1;
    }

    Disk->DataParts = (PEXT2_PARTITION) malloc(
                        sizeof(EXT2_PARTITION) * Disk->NumParts);
    if (!Disk->DataParts) {
        return FALSE;
    }

    memset(Disk->DataParts, 0, sizeof(EXT2_PARTITION) * Disk->NumParts);

    if (Disk->Layout) {
        if (Disk->Layout->PartitionStyle == PARTITION_STYLE_MBR && 
            Disk->Layout->PartitionEntry->Mbr.PartitionType == PARTITION_LDM) {
            bDynamic = TRUE;
        }


        /* Now walk through driveLayout and pack partitions */
        for (i = 0; (Disk->Layout != NULL) && 
             (i < (UCHAR)Disk->Layout->PartitionCount); i++) {

            PPARTITION_INFORMATION_EXT  Part;
            Part = &Disk->Layout->PartitionEntry[i];

            if (Disk->Layout->PartitionStyle == PARTITION_STYLE_MBR) {
                if (Part->Mbr.PartitionType == PARTITION_ENTRY_UNUSED ||
                    Part->Mbr.PartitionType == PARTITION_EXTENDED || 
                    Part->Mbr.PartitionType == PARTITION_XINT13_EXTENDED) {
                    continue;
                }
            }

            sprintf(&Disk->DataParts[cnt].Name[0],
                    "\\Device\\Harddisk%u\\Partition%u",
                    Disk->OrderNo, cnt + 1);
            Disk->DataParts[cnt].Magic = EXT2_PART_MAGIC;
            Disk->DataParts[cnt].PartType = Disk->Layout->PartitionStyle;

            Disk->DataParts[cnt].Disk = Disk;
            Disk->DataParts[cnt].Number = cnt + 1;
            Disk->DataParts[cnt++].Entry = Part;
        }

        /* Search the volumes of the partition */
        for (i=0; i < cnt; i++) {

            PEXT2_VOLUME   volume = gVols;
            for (j=0; (ULONG)j < g_nVols; j++) {
                if (Ext2IsPartitionExtent(Disk->OrderNo, 
                        Disk->DataParts[i].Entry,
                        volume->Extent )) {
                    volume->bDynamic = bDynamic;
                    if (volume->Extent->NumberOfDiskExtents == 1) {
                        volume->Part = &Disk->DataParts[i];
                    }
                    Disk->DataParts[i].Volume = volume;
                    Disk->DataParts[i].DrvLetters = volume->DrvLetters;
                    break;
                }
                volume = volume->Next;
            }
        }
    } else {
        Disk->DataParts[cnt].Magic = EXT2_PART_MAGIC;
        Disk->DataParts[cnt].Disk = Disk;
        Disk->DataParts[cnt].Number = cnt + 1;
        Disk->DataParts[cnt].DrvLetters = Ext2EjectedDiskLetters(Disk);
    }

    return TRUE;
}


VOID
Ext2LoadAllDiskPartitions()
{
    for (ULONG i=0; i < g_nDisks; i++) {
        Ext2LoadDiskPartitions(&gDisks[i]);
    }
}

VOID
Ext2MountingVolumes()
{
    PEXT2_VOLUME   volume = gVols;
    int j;

    if (!Ext2IsServiceStarted()) {
        return;
    }

    for (j=0; (ULONG)j < g_nVols; j++) {
        if ((volume->EVP.bExt2 || volume->EVP.bExt3) && !volume->bRecognized) {
            if (Ext2IsRemovable(volume) || (volume->DrvLetters != 0) || g_bAutoMount) {
                if (!Ext2CheckVolumeRegistryProperty(&volume->EVP)) {
                    Ext2SetDefaultVolumeRegistryProperty(&volume->EVP);
                }
                Ext2NotifyVolumePoint(volume, 0);
            }
        }

        volume = volume->Next;
    }

}

/*
 * Must initialize driver letter before loading volumes
 */

BOOL
Ext2LoadVolumes()
{
    ULONG   rc = TRUE;

    TCHAR  *nameString = NULL;
    ULONG   nameLen = REGSTR_VAL_MAX_HCID_LEN;

    HMACHINE hMachine = NULL;
    DEVNODE  dnRoot, dnFirst;

    DEVINSTID  devIds[] = {
            "ROOT\\FTDISK\\0000",
            "ROOT\\DMIO\\0000",
            NULL,
            "ROOT\\VOLMGR\\0000",
            NULL
            };

    int i = 0;

    if (IsVistaOrAbove()) {
        i = 3;
    }

    nameString = new TCHAR [nameLen];
    if (nameString == NULL) {
        goto errorout;
    }

    while (devIds[i] != NULL) {

	    rc = CM_Locate_DevNode_Ex(&dnRoot, devIds[i], 
                CM_LOCATE_DEVNODE_NORMAL , hMachine);
        if (rc != CR_SUCCESS) {
            break;
        }

        rc = CM_Get_Child_Ex(&dnFirst, dnRoot, 0, hMachine);
        if (rc != CR_SUCCESS) {
            goto errorout;
        }

        while (TRUE) {

            PEXT2_VOLUME volume = NULL;

            nameLen = REGSTR_VAL_MAX_HCID_LEN;
            memset(nameString, 0, nameLen);
            rc = CM_Get_DevNode_Registry_Property_Ex(
                        dnFirst, CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                        NULL, nameString, &nameLen, 0, hMachine);
            if (rc != CR_SUCCESS) {
                break;
            }

            if (Ext2InitializeVolume(nameString, &volume)) {

                 volume->bDynamic = 0;

                /* attach the volume to global list */
                if (gVols) {
                    PEXT2_VOLUME chain = gVols;
                    while (chain->Next) {
                        chain = chain->Next;
                    }
                    chain->Next = volume;
                } else {
                    gVols = volume;
                }
                g_nVols++;
            }
            
            rc = CM_Get_Sibling_Ex(&dnFirst, dnFirst, 0, hMachine);
            if (rc != CR_SUCCESS) {
                break;
            }
        }

        i++;
    }

errorout:

    if (nameString) {
        delete []nameString;
    }

    return TRUE;
}

BOOL
Ext2LoadRemovableVolumes()
{
    BOOL                     rc = FALSE;
    HDEVINFO                    devInfo = INVALID_HANDLE_VALUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA ifDetail = NULL;
    SP_DEVICE_INTERFACE_DATA    ifData;

    PCHAR                       nameString;
    int                         nInterface = 0;

    if (IsVistaOrAbove()) {
        return TRUE;
    }

    nameString = (PCHAR)malloc(REGSTR_VAL_MAX_HCID_LEN);
    if (nameString == NULL) {
        goto errorout;
    }

    ifDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(PAGE_SIZE);
    if (ifDetail == NULL) {
        goto errorout;
    }

    devInfo = SetupDiGetClassDevs(
                    &VolumeClassGuid, NULL, NULL,
                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
                    );
    if (devInfo == INVALID_HANDLE_VALUE) {
        goto errorout;
    }

	while (TRUE) {

        BOOL bFound = FALSE;

		ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        bFound = SetupDiEnumDeviceInterfaces(
				        devInfo,
				        NULL,
				        &VolumeClassGuid,
				        (ULONG)nInterface++,
				        &ifData);
        if (!bFound) {
            break;
        }

        memset(ifDetail, 0, PAGE_SIZE);
        ifDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (SetupDiGetInterfaceDeviceDetail(
					     devInfo,
					     &ifData,
					     ifDetail,
					     PAGE_SIZE,
					     NULL,
					     NULL)) {

            PEXT2_VOLUME volume = NULL;

            memset(nameString, 0, REGSTR_VAL_MAX_HCID_LEN);
            QueryDosDevice(&ifDetail->DevicePath[4], nameString, REGSTR_VAL_MAX_HCID_LEN);

            if (Ext2InitializeVolume(nameString, &volume)) {

                if (volume->FsdInfo.Characteristics & FILE_REMOVABLE_MEDIA) {

                    /* attach the volume to global list */
                    if (gVols) {
                        PEXT2_VOLUME chain = gVols;
                        while (chain->Next) {
                            chain = chain->Next;
                        }
                        chain->Next = volume;
                    } else {
                        gVols = volume;
                    }
                    g_nVols++;
                } else {
                    if (volume->Extent) {
                        free(volume->Extent);
                    }
                    free(volume);
                }
            }
        }
    }

    rc = TRUE;

errorout:

    if (nameString) {
        free(nameString);
    }

    if (ifDetail) {
        free(ifDetail);
    }

    if (devInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(devInfo);
    }

    return rc;
}

VOID
Ext2LoadAllVolumeDrvLetters()
{
    PEXT2_VOLUME volume = gVols;
    BOOL      started = Ext2IsServiceStarted();

    // Ext2DrvLetters[0] = Ext2DrvLetters[1];
    // Ext2DrvLetters[1] = 0;

    while (volume) {
        volume->DrvLetters = Ext2QueryVolumeDrvLetters(volume);
        if ( started && volume->bRecognized &&
            (volume->EVP.bExt2 || volume->EVP.bExt3)) {
            Ext2DrvLetters[1] |=  volume->DrvLetters;
        }
        volume = volume->Next;
    }
}


CString
Ext2QueryVolumeLetterStrings(
    ULONGLONG       letters,
    PEXT2_LETTER *  first
    )
{
    CHAR        drvName[] = "C:\0";
    CString     str;
    int         i = 0;
    BOOL     bInserted = FALSE;
    ULONGLONG   drive = 0;

    str.Empty();

    for (i=0; i < 10; i++) {
        drive = ((ULONGLONG) 1) << (i + 32);
        if (letters & drive) {
            if (bInserted) {
                str += ",";
            } else {
                str = "(";
                if (first) {
                    *first = &drvDigits[i];
                }
            }
            drvName[0] = '0' + i;
            str += drvName;
            bInserted = TRUE;
        }
    }

    for (i=0; i < 26; i++) {
        drive = ((ULONGLONG) 1) << (i);
        if (letters & drive) {
            if (bInserted) {
                str += ",";
            } else {
                str = "(";
                if (first) {
                    *first = &drvLetters[i];
                }
            }

            drvName[0] = 'A' + i;
            str += drvName;
            bInserted = TRUE;
        }
    }

    if (bInserted) {
        str += ")";
    }

    return str;
}

VOID
Ext2RefreshVLVI(
    CListCtrl *List,
    PEXT2_VOLUME chain,
    int  nItem
    )
{
    ULONGLONG   totalSize, usedSize;
    CString     sSize, sUnit, s;

    PEXT2_LETTER  first = NULL;
    CString  Letters = Ext2QueryVolumeLetterStrings(
                           chain->DrvLetters, &first);

    List->SetItem( nItem, 1, LVIF_TEXT, (LPCSTR)Letters, 0, 0, 0,0);
    
    if (chain->bDynamic) {
        s.LoadString(IDS_DISK_TYPE_DYN);
    } else {
        s.LoadString(IDS_DISK_TYPE_BASIC);
    }
    List->SetItem(nItem, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0,0);
    List->SetItem(nItem, 3, LVIF_TEXT, (LPCSTR)chain->FileSystem, 0, 0, 0,0);

    totalSize = chain->FssInfo.TotalAllocationUnits.QuadPart;
    usedSize  = totalSize - chain->FssInfo.AvailableAllocationUnits.QuadPart;
    totalSize = totalSize * chain->FssInfo.BytesPerSector *
                chain->FssInfo.SectorsPerAllocationUnit;
    usedSize  = usedSize * chain->FssInfo.BytesPerSector *
                chain->FssInfo.SectorsPerAllocationUnit;

    if (totalSize > (ULONGLONG) 10 * 1024 * 1024 * 1024) {
        totalSize = totalSize / ((ULONG)1024 * 1024 * 1024);
        usedSize  = usedSize / ((ULONG)1024 * 1024 * 1024);
        sUnit = " GB";
    } else if (totalSize > 10 * 1024 * 1024){
        totalSize = totalSize / (1024 * 1024);
        usedSize  = usedSize / (1024 * 1024);
        sUnit = " MB";
    } else if (totalSize > 10 * 1024){
        totalSize = totalSize / (1024);
        usedSize  = usedSize / (1024);
        sUnit = " KB";
    } else {
        sUnit = " B";
    }

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 4, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    if (chain->bRecognized) {
        sSize.Format("%I64u", usedSize);
        sSize += sUnit;
    }
    List->SetItem(nItem, 5, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    List->SetItem(nItem, 6, LVIF_TEXT, (LPCSTR)chain->EVP.Codepage, 0, 0, 0,0);
    List->SetItem(nItem, 7, LVIF_TEXT, (LPCSTR)chain->Name, 0, 0, 0,0);

/*
        List->SetItemState( nItem, LVIS_SELECTED | LVIS_FOCUSED ,
                                   LVIS_SELECTED | LVIS_FOCUSED);
*/
}


VOID
Ext2InsertVolume(
    CListCtrl *List,
    PEXT2_VOLUME chain
    )
{
    int nItem = 0, nImage = 0;

    nItem = List->GetItemCount();
    nImage = 0;

    if (chain->FsdInfo.Characteristics & FILE_VIRTUAL_VOLUME) {
        nImage = IDI_DYNAMIC - IDI_FLOPPY;
    } else if (chain->FsdInfo.Characteristics & FILE_REMOVABLE_MEDIA) {
        nImage = IDI_FLOPPY - IDI_FLOPPY;
    } else {
        nImage = IDI_DISK - IDI_FLOPPY;
    }

    nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                             0, 0, nImage, (LPARAM)chain);

    Ext2RefreshVLVI(List, chain, nItem);
}

VOID
Ext2RefreshVLCD(
    CListCtrl *List,
    PEXT2_CDROM Cdrom,
    int nItem
    )
{
    ULONGLONG   totalSize;
    CString     sSize, sUnit, s;

    PEXT2_LETTER  first = NULL;

    CString  Letters = Ext2QueryVolumeLetterStrings(
                            Cdrom->DrvLetters, &first);

    List->SetItem( nItem, 1, LVIF_TEXT, (LPCSTR)Letters, 0, 0, 0,0);

    s.LoadString(IDS_DISK_TYPE_BASIC);
    List->SetItem(nItem, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0,0);
    List->SetItem(nItem, 3, LVIF_TEXT, (LPCSTR)"CDFS", 0, 0, 0,0);


    totalSize = (ULONGLONG)Cdrom->DiskGeometry.Cylinders.QuadPart;
    totalSize = totalSize * Cdrom->DiskGeometry.TracksPerCylinder *
                Cdrom->DiskGeometry.SectorsPerTrack * 
                Cdrom->DiskGeometry.BytesPerSector;

    if (totalSize > 1024 * 1024 * 1024) {
        totalSize = totalSize / (1024 * 1024 * 1024);
        sUnit = " GB";
    } else if (totalSize > 1024 * 1024){
        totalSize = totalSize / (1024 * 1024);
        sUnit = " MB";
    } else if (totalSize > 1024){
        totalSize = totalSize / (1024);
        sUnit = " KB";
    } else {
        sUnit = " B";
    }

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 4, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 5, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    List->SetItem(nItem, 8, LVIF_TEXT, (LPCSTR)Cdrom->Name, 0, 0, 0,0);
}

VOID
Ext2InsertCdromAsVolume(
    CListCtrl *List,
    PEXT2_CDROM Cdrom
    )
{
    int nItem = 0, nImage = 0;

    nItem = List->GetItemCount();
    nImage = 0;

    if (Cdrom->bIsDVD) {
        nImage = IDI_DVD - IDI_FLOPPY;
    } else {
        nImage = IDI_CDROM - IDI_FLOPPY;
    }

    nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                             0, 0, nImage, (LPARAM)Cdrom);

    Ext2RefreshVLCD(List, Cdrom, nItem);
}

VOID
Ext2RefreshVolumeList(CListCtrl *List)
{
    List->DeleteAllItems();
    PEXT2_VOLUME  chain = gVols;

    /* adding disk volumes */
    while (chain) {
        Ext2InsertVolume(List, chain);
        chain = chain->Next;
    }

    /* adding cdroms */

    ULONG   i = 0;

    for (i=0; i < g_nCdroms; i++) {

        if (!gCdroms[i].bLoaded || gCdroms[i].bEjected) {
            continue;
        }
        Ext2InsertCdromAsVolume(List, &gCdroms[i]);
    }
}

VOID
Ext2RefreshDVPT(
    CListCtrl*      List,
    PEXT2_PARTITION Part,
    int             nItem
    )
{

    ULONGLONG   totalSize, usedSize;
    CString     sSize, sUnit, s;

    PEXT2_LETTER  first = NULL;
    PEXT2_VOLUME    chain = NULL;

    CString  Letters = Ext2QueryVolumeLetterStrings(
                            Part->DrvLetters, &first);

    List->SetItem( nItem, 0, LVIF_TEXT, (LPCSTR)Letters, 0, 0, 0,0);

    if (Part->Entry) {
        CString PartType;
        if (Part->Entry->PartitionStyle == PARTITION_STYLE_MBR) {
            PartType = PartitionString(Part->Entry->Mbr.PartitionType);
        } else if (Part->Entry->PartitionStyle == PARTITION_STYLE_GPT) {
            PartType = "";
            if (Part->Entry->Gpt.Name && wcslen(Part->Entry->Gpt.Name)) {
                for (size_t i = 0; i < wcslen(Part->Entry->Gpt.Name); i++)
                    PartType += (CHAR)Part->Entry->Gpt.Name[i];

            } else {
                PartType = "GPT";
            }
        } else {
            PartType = "RAW";
        }
        
        List->SetItem(nItem, 6, LVIF_TEXT, (LPCSTR)
                      PartType, 0, 0, 0,0);
    }

    /* query the volume information of the partition */
    chain = Part->Volume;
    if (!chain) {
        PEXT2_DISK  disk = Part->Disk;
        CString s;
        if (disk->SDD.RemovableMedia) {
            s.LoadString(IDS_DISK_TYPE_BASIC);
        } else if (disk->bLoaded){
            if (disk->Layout) {
                if (disk->Layout->PartitionStyle == PARTITION_STYLE_MBR) {
                    if (disk->Layout->PartitionEntry->Mbr.PartitionType
                        == PARTITION_LDM) {
                        s.LoadString(IDS_DISK_TYPE_DYN);
                    } else {
                        s.LoadString(IDS_DISK_TYPE_BASIC);
                    }
                } else if (disk->Layout->PartitionStyle == PARTITION_STYLE_GPT) {
                     s = "GPT";
                }
            } else {
                s = "RAW";
            }
        } else {
            s = "Stopped";
        }

        List->SetItem(nItem, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0,0);
        return;
    }

    if (chain->bDynamic) {
        s.LoadString(IDS_DISK_TYPE_DYN);
    } else {
        s.LoadString(IDS_DISK_TYPE_BASIC);
    }
    List->SetItem(nItem, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0,0);

    List->SetItem(nItem, 2, LVIF_TEXT, (LPCSTR)chain->FileSystem, 0, 0, 0,0);

    totalSize = chain->FssInfo.TotalAllocationUnits.QuadPart;
    usedSize  = totalSize - chain->FssInfo.AvailableAllocationUnits.QuadPart;
    totalSize = totalSize * chain->FssInfo.BytesPerSector *
                chain->FssInfo.SectorsPerAllocationUnit;
    usedSize  = usedSize * chain->FssInfo.BytesPerSector *
                chain->FssInfo.SectorsPerAllocationUnit;

    if (totalSize > (ULONGLONG) 10 * 1024 * 1024 * 1024) {
        totalSize = totalSize / ((ULONG)1024 * 1024 * 1024);
        usedSize  = usedSize / ((ULONG)1024 * 1024 * 1024);
        sUnit = " GB";
    } else if (totalSize > 10 * 1024 * 1024){
        totalSize = totalSize / (1024 * 1024);
        usedSize  = usedSize / (1024 * 1024);
        sUnit = " MB";
    } else if (totalSize > 10 * 1024){
        totalSize = totalSize / (1024);
        usedSize  = usedSize / (1024);
        sUnit = " KB";
    } else {
        sUnit = " B";
    }

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 3, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    if (chain->bRecognized) {
        sSize.Format("%I64u", usedSize);
        sSize += sUnit;
    }
    List->SetItem(nItem, 4, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    List->SetItem(nItem, 5, LVIF_TEXT, (LPCSTR)chain->EVP.Codepage, 0, 0, 0,0);
}

VOID
Ext2InsertPartition(
    CListCtrl*      List,
    PEXT2_DISK      Disk,
    PEXT2_PARTITION Part
    )
{
    int nItem = 0, nImage = 0;

    nItem = List->GetItemCount();
    nImage = 0;

    if (!Disk) {
        nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                                 0, 0, nImage, (LPARAM)NULL);
        return;
    }

    if (!Part) {
        nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                                 0, 0, nImage, (LPARAM)&Disk->Null);
        List->SetItem( nItem, 1, LVIF_TEXT, (LPCSTR)"RAW", 0, 0, 0,0);
        return;
    }

    nItem = List->InsertItem( LVIF_PARAM| LVIF_IMAGE, nItem, NULL,
                               0, 0, nImage, (LPARAM)Part);

    Ext2RefreshDVPT(List, Part, nItem);
}

VOID
Ext2InsertDisk(
    CListCtrl *List,
    PEXT2_DISK Disk
    )
{
    UCHAR   i;
    CHAR  devName[64];
    int nItem = 0, nImage = 0;

    sprintf(devName, "DISK %d", Disk->OrderNo);
    nItem = List->GetItemCount();
    nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                             0, 0, nImage, (LPARAM)Disk);
    List->SetItem( nItem, 0, LVIF_TEXT, (LPCSTR)devName, 0, 0, 0,0);

    if (Disk->NumParts > 0) {
        for (i=0; i < Disk->NumParts; i++) {
            Ext2InsertPartition(List, Disk, &Disk->DataParts[i]);
        }
    } else {
        Ext2InsertPartition(List, Disk, NULL);
    }
}


VOID
Ext2RefreshDVCM(
    CListCtrl *List,
    PEXT2_CDROM Cdrom,
    int nItem
    )
{
    ULONGLONG   totalSize;
    CString     sSize, sUnit, s;

    PEXT2_LETTER  first = NULL;

    CString  Letters = Ext2QueryVolumeLetterStrings(
                            Cdrom->DrvLetters, &first);

    if (!Cdrom->bLoaded) {
        List->SetItem( nItem, 1, LVIF_TEXT, (LPCSTR)"Stopped", 0, 0, 0,0);
        return;
    }

    List->SetItem( nItem, 0, LVIF_TEXT, (LPCSTR)Letters, 0, 0, 0,0);

    if (Cdrom->bEjected) {
        return;
    }

    s.LoadString(IDS_DISK_TYPE_BASIC);
    List->SetItem(nItem, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0,0);
    List->SetItem(nItem, 2, LVIF_TEXT, (LPCSTR)"CDFS", 0, 0, 0,0);

    totalSize = (ULONGLONG)Cdrom->DiskGeometry.Cylinders.QuadPart;
    totalSize = totalSize * Cdrom->DiskGeometry.TracksPerCylinder *
                Cdrom->DiskGeometry.SectorsPerTrack * 
                Cdrom->DiskGeometry.BytesPerSector;

    if (totalSize > 1024 * 1024 * 1024) {
        totalSize = totalSize / (1024 * 1024 * 1024);
        sUnit = " GB";
    } else if (totalSize > 1024 * 1024){
        totalSize = totalSize / (1024 * 1024);
        sUnit = " MB";
    } else if (totalSize > 1024){
        totalSize = totalSize / (1024);
        sUnit = " KB";
    } else {
        sUnit = " B";
    }

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 3, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    sSize.Format("%I64u", totalSize);
    sSize += sUnit;
    List->SetItem(nItem, 4, LVIF_TEXT, (LPCSTR)sSize, 0, 0, 0,0);

    List->SetItem(nItem, 5, LVIF_TEXT, (LPCSTR)"", 0, 0, 0,0);
    List->SetItem(nItem, 6, LVIF_TEXT, (LPCSTR)"", 0, 0, 0,0);
}

VOID
Ext2InsertCdromAsDisk(
    CListCtrl *List,
    PEXT2_CDROM Cdrom
    )
{
    CHAR  devName[64];
    int nItem = 0, nImage = 0;

    sprintf(devName, "CDROM %d", Cdrom->OrderNo);
    nItem = List->GetItemCount();
    nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                             0, 0, nImage, (LPARAM)Cdrom);
    List->SetItem( nItem, 0, LVIF_TEXT, (LPCSTR)devName, 0, 0, 0,0);

    nItem = List->GetItemCount();
    nImage = 0;

    if (Cdrom->bIsDVD) {
        nImage = IDI_DVD - IDI_FLOPPY;
    } else {
        nImage = IDI_CDROM - IDI_FLOPPY;
    }

    nItem = List->InsertItem( LVIF_PARAM|LVIF_IMAGE, nItem, NULL,
                             0, 0, nImage, (LPARAM)(&Cdrom->Magic[1]));

    Ext2RefreshDVCM(List, Cdrom, nItem);
}

VOID
Ext2RefreshDiskList(CListCtrl *List)
{
    List->DeleteAllItems();
    ULONG   i = 0;

    /* adding disks */
    for (i=0; i < g_nDisks; i++) {
        Ext2InsertDisk(List, &gDisks[i]);
        Ext2InsertPartition(List, NULL, NULL);
    }

    /* adding cdroms */
    for (i=0; i < g_nCdroms; i++) {
        Ext2InsertCdromAsDisk(List, &gCdroms[i]);
        Ext2InsertPartition(List, NULL, NULL);
    }
}


VOID
Ext2CleanupVolumes()
{
    PEXT2_VOLUME  chain = gVols, next = NULL;

    while (chain) {
        next = chain->Next;
        if (chain->Extent) {
            free(chain->Extent);
        }
        free(chain);
        g_nVols--;
        chain = next;
    }

    ASSERT(g_nVols == 0);
    g_nVols = 0; gVols = NULL;
}

VOID
Ext2LoadDrvLetter(PEXT2_LETTER drvLetter, CHAR cLetter)
{
    if (drvLetter->bUsed) {
        return;
    }

    if (cLetter >= 'a' && cLetter <= 'z') {
        cLetter -= 0x20;
    }

    memset(drvLetter, 0, sizeof(EXT2_LETTER));
    drvLetter->Letter = cLetter;
    Ext2QueryDrvLetter(drvLetter);
}

VOID
Ext2LoadDrvLetters()
{
    int     i;

    for (i=0; i < 36; i++) {

        PEXT2_LETTER    drvLetter = NULL;
        CHAR            cLetter = 0;

        if (i < 10) {
            drvLetter = &drvDigits[i];
            cLetter = '0' + i;
        } else {
            drvLetter = &drvLetters[i-10];
            cLetter = 'A' + (i - 10);
        }

        Ext2LoadDrvLetter(drvLetter, cLetter);
    }
}

VOID
Ext2CleanDrvLetter(PEXT2_LETTER drvLetter)
{
    if (drvLetter->Extent) {
        free(drvLetter->Extent);
        drvLetter->Extent = NULL;
    }

    if (drvLetter->SDN) {
        free(drvLetter->SDN);
        drvLetter->SDN = NULL;
    }

    drvLetter->bUsed = FALSE;
    drvLetter->bTemporary = FALSE;
    drvLetter->DrvType = DRIVE_NO_ROOT_DIR;
    memset(drvLetter->SymLink, 0, MAX_PATH);
}

VOID
Ext2CleanupDrvLetters()
{
    int i = 0;

    for (i=0; i < 10; i++) {
        Ext2CleanDrvLetter(&drvDigits[i]);
    }

    for (i=0; i < 26; i++) {
        Ext2CleanDrvLetter(&drvLetters[i]);
    }
}

VOID
Ext2DrvNotify(UCHAR drive, int mount)
{
    DEV_BROADCAST_VOLUME    dbv;
    DWORD target = BSM_APPLICATIONS;
    unsigned long drv = 0;

    if (drive >= 'A' && drive <= 'Z')
        drv = drive - 'A';
    else if(drive >= 'a' && drive <= 'z')
        drv = drive - 'a';
    else
        return;

    dbv.dbcv_size       = sizeof( dbv );
    dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    dbv.dbcv_reserved   = 0;
    dbv.dbcv_unitmask   = (1 << drv);
    dbv.dbcv_flags      = DBTF_NET;
    BroadcastSystemMessage(BSF_IGNORECURRENTTASK | BSF_FORCEIFHUNG |
                           BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                           &target, WM_DEVICECHANGE, mount ?
                           DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE,
                           (LPARAM)(DEV_BROADCAST_HDR *)&dbv );
}

BOOL
Ext2RemoveDrvLetter(
    PEXT2_LETTER   drvLetter
    )
{
    CHAR drvPath[] = "A:\\\0";
    CHAR dosPath[] = "A:\0";

    BOOL rc = FALSE;

    if (!drvLetter->bUsed) {
        return TRUE;
    }

    dosPath[0] = drvPath[0] = drvLetter->Letter;
    rc = Ext2DefineDosDevice ( 
                DDD_RAW_TARGET_PATH|
                DDD_REMOVE_DEFINITION|
                DDD_EXACT_MATCH_ON_REMOVE,
                dosPath, drvLetter->SymLink);
    DeleteVolumeMountPoint(drvPath);

    if (!rc) {
        return FALSE;
    }

    return TRUE;
}

CHAR
Ext2QueryRegistryMountPoint (
    CHAR * devName
    )
{
    DWORD    i = 0;

    LONG    status;
    HKEY    hKey;

    DWORD   drvSize;
    DWORD   dosSize;
    DWORD   valType;

    CHAR    keyPath[MAX_PATH];
    CHAR    drvPath[MAX_PATH];
    CHAR    dosPath[64];

    /* set dos deivce path */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\DOS Devices");

    /* open session manager\dos devices */
    status = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }


    /* enum all values and compare devName ... */
    while (status != ERROR_NO_MORE_ITEMS && i < 1024) {

        valType = REG_SZ;
        dosSize = 64;
        drvSize = MAX_PATH;

        status = RegEnumValue(hKey, 
                              i++,
                              &dosPath[0],
                              &dosSize,
                              NULL,
                              &valType,
                              (LPBYTE)&drvPath[0],
                              &drvSize);

        if (status == ERROR_SUCCESS) {
            if (_stricmp(devName, drvPath) == 0) {
                RegCloseKey(hKey);
                return dosPath[0];
            }
        }
    }

    RegCloseKey(hKey);

    return 0;
}

BOOL
Ext2SetRegistryMountPoint (
    CHAR * dosPath,
    CHAR * devName,
    BOOL bSet
    )
{
    LONG    status;
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH];
    CHAR    drvPath[MAX_PATH];

    /* set dos driver name */
    sprintf(drvPath, "%c:", dosPath[0]);

    /* set dos deivce path */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\DOS Devices");

    /* open session manager\dos devices */
    status = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    if (bSet) {
        /* set value */
        status = RegSetValueEx(
                    hKey, drvPath,
                    0, REG_SZ,
                    (BYTE *)devName,
                    (int)strlen(devName)
                    );
    } else {
        /* delete key */
        status = RegDeleteValue(
                    hKey, drvPath
                    );
    }

    RegCloseKey(hKey);

    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

VOID
Ext2UpdateDrvLetter(
    PEXT2_LETTER   drvLetter,
    PCHAR          devPath
    )
{
    CHAR dosPath[] = "A:\0";
    BOOL rc = 0;

    if (drvLetter->bUsed) {
        return;
    }

    dosPath[0] = drvLetter->Letter;
    rc = Ext2DefineDosDevice(DDD_RAW_TARGET_PATH,
                         dosPath, devPath);

    if (rc) {
        Ext2DefineDosDevice ( 
                    DDD_RAW_TARGET_PATH|
                    DDD_REMOVE_DEFINITION|
                    DDD_EXACT_MATCH_ON_REMOVE,
                    dosPath, devPath);
    }
}

BOOL
Ext2AssignDrvLetter(
    PEXT2_LETTER   drvLetter,
    PCHAR          devPath,
    BOOL        bMountMgr
    )
{
    CHAR dosPath[] = "A:\0";
    CHAR drvPath[] = "A:\\\0";
    CHAR volumeName[MAX_PATH];

    BOOL rc = 0;

    if (drvLetter->bUsed) {
        return FALSE;
    }

    memset(volumeName, 0, MAX_PATH);
    dosPath[0] = drvPath[0] = drvLetter->Letter;

    rc = Ext2DefineDosDevice(DDD_RAW_TARGET_PATH,
                         dosPath, devPath);

    if (rc) {

        /* Now it's done, since user only
           needs a temporary mount point */
        if (!bMountMgr) {
            goto DrvMounted;
        }

        rc = GetVolumeNameForVolumeMountPoint (
                    drvPath, volumeName, MAX_PATH);

        Ext2DefineDosDevice ( 
                    DDD_RAW_TARGET_PATH|
                    DDD_REMOVE_DEFINITION|
                    DDD_EXACT_MATCH_ON_REMOVE,
                    dosPath, devPath);

        if (rc) {
            rc = SetVolumeMountPoint(drvPath, volumeName); 
        }
    }

    if (!rc) {
        return FALSE;
    }

DrvMounted:

    drvLetter->bTemporary = !bMountMgr;
    Ext2QueryDrvLetter(drvLetter);

    return TRUE;
}

/*
 *  Mount Manager Support Routines
 */

VOID
Ext2AnsiToUnicode(
    CHAR *      AnsiName,
    WCHAR*      UniName,
    USHORT      UniLength
    )
{
    USHORT                  NameLength = 0;
    NT::ANSI_STRING         AnsiFilespec;
    NT::UNICODE_STRING      UnicodeFilespec;

    memset(UniName, 0, sizeof(WCHAR) * UniLength);

    NameLength = (USHORT)strlen(AnsiName);
    ASSERT(NameLength < UniLength);

    AnsiFilespec.MaximumLength = AnsiFilespec.Length = NameLength;
    AnsiFilespec.Buffer = AnsiName;

    UnicodeFilespec.MaximumLength = UniLength * 2;
    UnicodeFilespec.Length = 0;
    UnicodeFilespec.Buffer = (PWSTR)UniName;

    NT::RtlAnsiStringToUnicodeString(&UnicodeFilespec, &AnsiFilespec, FALSE);
}

BOOL
Ext2VolumeArrivalNotify(PCHAR  VolumePath)
{
    HANDLE              hMountMgr = NULL;

    NT::IO_STATUS_BLOCK iosb;
    NT::NTSTATUS        status;
    DWORD               rc = 0; 

    WCHAR      volPath[MAX_PATH + 2];
    PMOUNTMGR_TARGET_NAME target;

    target = (PMOUNTMGR_TARGET_NAME) volPath;
    Ext2AnsiToUnicode(VolumePath, &target->DeviceName[0], MAX_PATH);
    target->DeviceNameLength = (USHORT)wcslen(target->DeviceName) * 2;

    status = Ext2Open("\\Device\\MountPointManager", &hMountMgr, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    status = NT::ZwDeviceIoControlFile(
                hMountMgr, NULL, NULL, NULL, &iosb,
                IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
                (PVOID)target, sizeof(USHORT) +
                target->DeviceNameLength, NULL, 0
                );

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

errorout:

    Ext2Close(&hMountMgr);
    return NT_SUCCESS(status);
}


BOOL
Ext2SymLinkRemoval(CHAR drvLetter)
{
    HANDLE              hMountMgr = NULL;

    NT::IO_STATUS_BLOCK iosb;
    NT::NTSTATUS        status;
    DWORD               rc = 0; 

    WCHAR buffer[MAX_PATH], *dosName;
    PMOUNTMGR_MOUNT_POINT  pmp = NULL;
    PMOUNTMGR_MOUNT_POINTS pms = NULL;

    memset(buffer, 0, sizeof(WCHAR) * MAX_PATH);
    pmp = (PMOUNTMGR_MOUNT_POINT) buffer;
    pmp->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    pmp->SymbolicLinkNameLength = 14 * sizeof(WCHAR);

    dosName = (WCHAR *) (pmp + 1);
    swprintf_s(dosName, MAX_PATH - sizeof(MOUNTMGR_MOUNT_POINT),
               L"\\DosDevices\\%c:\0", drvLetter);

    status = Ext2Open("\\Device\\MountPointManager", &hMountMgr, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    status = NT::ZwDeviceIoControlFile(
                hMountMgr, NULL, NULL, NULL, &iosb,
                IOCTL_MOUNTMGR_DELETE_POINTS,
                (PVOID)pmp, sizeof(WCHAR) * MAX_PATH, 
                (PVOID)pmp, sizeof(WCHAR) * MAX_PATH
                );

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

errorout:

    Ext2Close(&hMountMgr);
    return NT_SUCCESS(status);
}

BOOL
Ext2SetVolumeMountPoint (
    CHAR * dosPath,
    CHAR * devName
    )
{
    HANDLE              hMountMgr;

    NT::IO_STATUS_BLOCK iosb;
    NT::NTSTATUS        status;
    DWORD               rc = 0; 

    CHAR       dosDevice[MAX_PATH];
    WCHAR      volPath[MAX_PATH];
    WCHAR      devPath[MAX_PATH];

    PMOUNTMGR_CREATE_POINT_INPUT  mcpi = NULL;

    USHORT   lmcpi = 0, lvolp = 0, ldevp = 0;

    memset(dosDevice, 0, MAX_PATH);
    memset(volPath, 0, MAX_PATH * sizeof(WCHAR));
    memset(devPath, 0, MAX_PATH * sizeof(WCHAR));

    /* covert names to unicode */
    sprintf(dosDevice, "\\DosDevices\\%c:", toupper(dosPath[0]));
    Ext2AnsiToUnicode(dosDevice, volPath, MAX_PATH);
    Ext2AnsiToUnicode(devName, devPath, MAX_PATH);

    ldevp = (USHORT)wcslen(devPath) * 2;
    lvolp = (USHORT)wcslen(volPath) * 2;
    lmcpi = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + lvolp + ldevp;

    /* initialize MMGR_CREATE_MOUNT_POINT_INPUT */
    mcpi = (PMOUNTMGR_CREATE_POINT_INPUT) malloc(lmcpi);
    if (mcpi == NULL) {
        status = STATUS_UNSUCCESSFUL;
        goto errorout;
    }

    mcpi->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    mcpi->SymbolicLinkNameLength = lvolp;
    mcpi->DeviceNameOffset = mcpi->SymbolicLinkNameOffset + lvolp;
    mcpi->DeviceNameLength = ldevp;

    memcpy((PCHAR)mcpi + mcpi->SymbolicLinkNameOffset,
           volPath, lvolp);
    memcpy((PCHAR)mcpi + mcpi->DeviceNameOffset,
           devPath, ldevp);

    /* open MountMgr */
    status = Ext2Open("\\Device\\MountPointManager", &hMountMgr, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    /* ioctl .... */
    status = NT::ZwDeviceIoControlFile(
                hMountMgr, NULL, NULL, NULL, &iosb,
                IOCTL_MOUNTMGR_CREATE_POINT,
                (PVOID)mcpi, lmcpi, NULL, 0
                );

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    Ext2Close(&hMountMgr);

errorout:
    if (mcpi) {
        free(mcpi);
    }

    return NT_SUCCESS(status);
}


UCHAR
Ext2QueryMountPoint(
    CHAR *      volume
    )
{
    NT::IO_STATUS_BLOCK     iosb;
    NT::NTSTATUS            status;
    ULONG                   lp, lps;
    ULONG                   i;
    HANDLE                  hMountMgr = 0;

    NT::UNICODE_STRING      volPoint;
    NT::UNICODE_STRING      VolumeName;
    WCHAR                   VolumeBuffer[MAX_PATH];

    PMOUNTMGR_MOUNT_POINT   point = NULL;
    PMOUNTMGR_MOUNT_POINTS  points = NULL;
    UCHAR                   drvLetter = 0;

    memset(VolumeBuffer, 0, MAX_PATH * sizeof(WCHAR));
    Ext2AnsiToUnicode(volume, VolumeBuffer, MAX_PATH);

    VolumeName.MaximumLength = MAX_PATH;
    VolumeName.Length = (USHORT)wcslen(VolumeBuffer) * 2;
    VolumeName.Buffer = VolumeBuffer;

    status = Ext2Open("\\Device\\MountPointManager", &hMountMgr, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    lp = VolumeName.Length + sizeof(MOUNTMGR_MOUNT_POINT) + 2;
    point = (PMOUNTMGR_MOUNT_POINT)malloc(lp);
    if (point == NULL) {
        goto errorout;
    }

    /* initialize MountMgr ioctl input structure */
    memset(point, 0, lp);
    point->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->DeviceNameLength = VolumeName.Length;
    RtlMoveMemory((PCHAR) point + point->DeviceNameOffset,
                  VolumeBuffer, VolumeName.Length);
    lps = 1024;

Again:

    /* allocate ioctl output structure */
    points = (PMOUNTMGR_MOUNT_POINTS)malloc(lps);
    if (!points) {
        goto errorout;
    }
    RtlZeroMemory(points, lps);

    /* could MoungMgr to create volume points for us ? */
    status = NT::ZwDeviceIoControlFile(
                hMountMgr, NULL, NULL, NULL, &iosb,
                IOCTL_MOUNTMGR_QUERY_POINTS,
                point, lp, points, lps
                );

    if (status == STATUS_BUFFER_OVERFLOW) {
        lps = (ULONG)iosb.Information + 2;
        free(points);
        points = NULL;
        goto Again;
    }

    for (i = 0; i < points->NumberOfMountPoints; i++) {

        volPoint.Length = volPoint.MaximumLength =
                points->MountPoints[i].SymbolicLinkNameLength;
        volPoint.Buffer = (PWSTR) ((PCHAR) points +
                     points->MountPoints[i].SymbolicLinkNameOffset);

#if 0
        if (MOUNTMGR_IS_VOLUME_NAME(&volPoint)) {
            if (length) {
                if (*length >= volPoint.Length + 2) {
                    *length = volPoint.Length + 2;
                    memcpy(vp, volPoint.Buffer, volPoint.Length);
                    vp[1] = (WCHAR)'\\';
                    vp[volPoint.Length / 2] = (WCHAR)'\\';
                    vp[*length / 2] = 0;
                } else {
                    *length = 0;
                }
            } 
        }
#endif

        if (MOUNTMGR_IS_DRIVE_LETTER(&volPoint)) {
            drvLetter = (UCHAR)(volPoint.Buffer[12]);
            break;
        }
    }

errorout:

    if (points) {
        free(points);
    }

    if (point) {
        free(point);
    }

    if (hMountMgr) {
        Ext2Close(&hMountMgr);
    }

    return drvLetter;
}

PEXT2_LETTER
Ext2GetDrvLetterPoint(CHAR drvChar)
{
    PEXT2_LETTER    drvLetter = NULL;

    if (drvChar >= '0' && drvChar <= '9') {
        drvLetter = &drvDigits[drvChar - '0'];
    } else if (drvChar >= 'A' && drvChar <= 'Z') {
        drvLetter = &drvLetters[drvChar - 'A'];
    } else {
        drvLetter = NULL;
    }

    return drvLetter;
}

BOOL
Ext2InsertMountPoint(
    CHAR * volume,
    UCHAR drvChar,
    BOOL  bGlobal
    )
{
    CHAR volumeName[MAX_PATH];

    CHAR dosPath[] = "A:\0";
    CHAR drvPath[] = "A:\\\0";
    BOOL rc = FALSE;

    dosPath[0] = drvPath[0] = drvChar & 0x7F;
    rc = Ext2DefineDosDevice(DDD_RAW_TARGET_PATH,
                             dosPath, volume);

    if (rc && bGlobal) {
        rc = GetVolumeNameForVolumeMountPoint (
                    drvPath, volumeName, MAX_PATH);

        Ext2DefineDosDevice ( 
                    DDD_RAW_TARGET_PATH|
                    DDD_REMOVE_DEFINITION,
                    dosPath, volume);
        if (rc) {
            rc = SetVolumeMountPoint(drvPath, volumeName); 
        }
    }

    return rc;
}

VOID
Ext2RemoveMountPoint(
    PEXT2_LETTER    drvLetter,
    BOOL         bPermanent
    )
{
    CHAR drvPath[] = "A:\\\0";
    CHAR dosPath[] = "A:\0";

    dosPath[0] = drvPath[0] = drvLetter->Letter;

    Ext2DefineDosDevice ( 
            DDD_RAW_TARGET_PATH|
            DDD_REMOVE_DEFINITION|
            DDD_EXACT_MATCH_ON_REMOVE,
            dosPath, drvLetter->SymLink);
    if (bPermanent) {
        DeleteVolumeMountPoint(drvPath);
    }

    Ext2CleanDrvLetter(drvLetter);
    Ext2QueryDrvLetter(drvLetter);
}

BOOL
Ext2RefreshVolumePoint(
    CHAR *          volume,
    UCHAR           drvChar
    )
{
    PEXT2_LETTER    drvLetter = NULL;
    BOOL         rc = TRUE;
    CHAR            cLetter = 0;

    cLetter = Ext2QueryMountPoint(volume);
    if (cLetter) {
        goto errorout;
    } else {
        rc = FALSE;
    }

    if (drvChar & 0x7F) {
        if (Ext2InsertMountPoint(volume, drvChar & 0x7F, TRUE)) {
            return TRUE;
        }
    }

    if (!rc) {
        Ext2VolumeArrivalNotify(volume);
    }

    cLetter = Ext2QueryMountPoint(volume);
	if (!cLetter && (cLetter != (drvChar & 0x7F))) {
        Ext2InsertMountPoint(volume, drvChar & 0x7F, TRUE);
    }

    cLetter = Ext2QueryMountPoint(volume);
    if (cLetter) {
        Ext2InsertMountPoint(volume, cLetter, FALSE);
    }

    rc =  (cLetter > 0) ?  TRUE: FALSE;

errorout:

    return rc;
}


BOOL Ext2IsDrvLetterAvailable(CHAR drive)
{
    CHAR            symLink[MAX_PATH] = {0};
    UINT            drvType;

    drvType = Ext2QueryDrive(drive, symLink);
    if (drvType == DRIVE_NO_ROOT_DIR) {
        return TRUE;
    }

    return FALSE;
}


CHAR Ext2GetFirstAvailableDrvLetter()
{
    int             i;

    for (i = 2; i < 24; i++) {
        if (Ext2IsDrvLetterAvailable('A' + i))
            return ('A' + i);
    }

    return 0;
}


BOOL
Ext2NotifyVolumePoint(
    PEXT2_VOLUME    volume,
    UCHAR           drvChar
    )
{
    PEXT2_LETTER letter;
    UCHAR   mounted = 0;
    BOOL    rc = FALSE;
    PCHAR   Name = NULL;

    drvChar = (UCHAR)toupper(drvChar);
    letter  = &drvLetters[drvChar - 'A'];

    if (volume->Part) {
        Name = &volume->Part->Name[0];
    } else {
        Name = &volume->Name[0];
    }

    /* do drive update */
    if (Ext2IsDrvLetterAvailable(drvChar))
        Ext2UpdateDrvLetter(letter, Name);

    rc = Ext2VolumeArrivalNotify(Name);

    Sleep(500);

    mounted = Ext2QueryMountPoint(Name);
    if (mounted) {
        rc = TRUE;
        goto errorout;
    }

    if (!Ext2IsDrvLetterAvailable(drvChar)) {
        drvChar = Ext2GetFirstAvailableDrvLetter();
        if ((drvChar|0x20) >= 'a' && (drvChar | 0x20) <= 'z') {
            letter  = &drvLetters[drvChar - 'A'];
        } else {
            letter = NULL;
        }
    }

    if (letter) {
        rc = Ext2AssignDrvLetter(letter, Name, FALSE);
    }

errorout:

    return rc;
}

#define EXT2_MANAGER_NAME  "Ext2 Volume Manager"

BOOL Ext2RunMgrForCurrentUserXP()
{
    HKEY   key ;
    CHAR   keyPath[MAX_PATH] ;
    LONG   status ;
    DWORD  type, len = MAX_PATH;
    BOOL   rc = FALSE;

    CHAR   	appPath[MAX_PATH] ;

    GetModuleFileName(NULL, appPath, MAX_PATH - 6);
    strcat(appPath, " -quiet");

    strcpy (keyPath, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run") ;
    status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              keyPath,
                              0,
                              KEY_ALL_ACCESS,
                              &key) ;

    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegQueryValueEx( key, EXT2_MANAGER_NAME, 0, &type, 
                               (BYTE *)appPath, &len);
    if (status != ERROR_SUCCESS) {
        goto errorout;
    }

    rc = TRUE;

errorout:

    RegCloseKey (key) ;
    return rc;
}
	
BOOL
Ext2SetAppAutorunXP(BOOL bInstall)
{
    BOOL   rc = FALSE;
    HKEY   key ;
    CHAR   keyPath[MAX_PATH] ;
    LONG   status ;

    CHAR   	appPath[MAX_PATH] ;

    GetModuleFileName(NULL, appPath, MAX_PATH - 6);
    strcat(appPath, " -quiet");

    strcpy (keyPath, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run") ;
    status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              keyPath,
                              0,
                              KEY_ALL_ACCESS,
                              &key) ;

    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    if (!bInstall) {
        status = RegDeleteValue (key, EXT2_MANAGER_NAME) ;
        if (status != ERROR_SUCCESS) {
            goto errorout;
        }
    } else {
        status = RegSetValueEx( key, EXT2_MANAGER_NAME, 0, REG_SZ, 
                                   (BYTE *)appPath, (int)strlen(appPath));
        if (status != ERROR_SUCCESS) {
            goto errorout;
        }
    }

    rc = TRUE;

errorout:

    RegCloseKey (key) ;
    return rc;
}

CHAR *Ext2QueryAutoUserList()
{
    int     rc = TRUE;
    HKEY    hKey;
    CHAR    keyPath[MAX_PATH];
    CHAR   *userList = NULL;
    LONG    status, type, len;

    /* Open ext2fsd sevice key */
    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Parameters") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* query autorun user list */
    len = PAGE_SIZE - 1;
    userList = new CHAR[len + 1];
    if (!userList)
        goto errorout;
    memset(userList, 0, len + 1);
    status = RegQueryValueEx( hKey,
                              "AutorunUsers",
                              0,
                              (LPDWORD)&type,
                              (BYTE *)userList,
                              (LPDWORD)&len);

errorout:

    RegCloseKey(hKey);

    return userList;
}

BOOL Ext2SetAutoRunUserList(CHAR *userList)
{
    HKEY   key;
    CHAR   keyPath[MAX_PATH] ;
    LONG   status;

    strcpy (keyPath, "SYSTEM\\CurrentControlSet\\Services\\Ext2Fsd\\Parameters") ;
    status = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                            &key) ;
    if (status != ERROR_SUCCESS) {
        goto errorout;
    }

    status = RegSetValueEx( key, "AutorunUsers", 0, REG_SZ, 
                            (BYTE *)userList, (int)strlen(userList));
    if (status != ERROR_SUCCESS) {
        goto errorout;
    }

errorout:

    RegCloseKey(key) ;

    return (status == ERROR_SUCCESS);
}

TCHAR *
Ext2StrStr(TCHAR *s, TCHAR *t)
{
    int ls = (int)_tcslen(s), lt = (int)_tcslen(t), i;
    for (i = 0; i + lt <= ls; i++) {
        if (0 == _tcsnicmp(&s[i], t, lt))
            return &s[i];
    }

    return NULL;
}


BOOL Ext2RunMgrForCurrentUserVista()
{
    CHAR *userList = NULL, *user, e;
    CHAR  userName[256] = {0};
    DWORD userLen = 255;
    BOOL  rc = FALSE;

    if (!GetUserName(userName, &userLen))
        return FALSE;

    userList = Ext2QueryAutoUserList();
    if (!userList)
        return FALSE;

    user = userList;
    while (user = Ext2StrStr(user, userName)) {
        if (user > userList) {
            e = user[-1];
            if (e != ',' && e != ';') {
                user = user + strlen(userName);
                continue;
            }
        }
        e = user[strlen(userName)];
        if (!e || e == ',' || e == ';') {
            rc = TRUE;
            goto errorout;
        }
        user = user + strlen(userName);
    }

errorout:

    if (userList)
        delete [] userList;

    return rc;
}

BOOL
Ext2SetAppAutorunVista(BOOL bAutorun)
{
    CHAR *userList = NULL, *user, *e;
    CHAR  userName[256] = {0};
    DWORD userLen = 255;
    BOOL  changed = FALSE;

    if (!GetUserName(userName, &userLen))
        return FALSE;

    userList = Ext2QueryAutoUserList();
    if (!userList)
        return FALSE;

    if (bAutorun) {

        user = userList;
        while (user = Ext2StrStr(user, userName)) {
            if (user > userList) {
                e = user-1;
                if (*e != ',' && *e != ';') {
                    user = user + strlen(userName);
                    continue;
                }
            }
            e = &user[strlen(userName)];
            if (!*e || *e == ',' || *e == ';') {
                goto errorout;
            }
            user = user + strlen(userName);
        }

        e = &userList[strlen(userList) - 1];
        if (e > userList && *e != ',' && *e != ';') {
            strcat_s(userList, PAGE_SIZE - 1, ";");
        }
        strcat_s(userList, PAGE_SIZE - 1, userName);
        strcat_s(userList, PAGE_SIZE - 1, ";");
        changed = TRUE;

    } else {
        user = userList;
        while (user = Ext2StrStr(user, userName)) {
            if (user > userList) {
                e = user - 1;
                if (*e != ',' && *e != ';') {
                    user = user + strlen(userName);
                    continue;
                }
            }
            e = &user[strlen(userName)];
            if (!*e) {
                memset(user, 0, strlen(userName) + 1);
                changed = TRUE;
            } else if (*e == ',' || *e == ';') {
                memmove(user, e + 1, strlen(e));
                changed = TRUE;
            } else {
                user = user + strlen(userName);
            }
        }
    }

    if (changed)
        changed = Ext2SetAutoRunUserList(userList);
    else
        changed = TRUE;

errorout:

    if (userList)
        delete []userList;

    return TRUE;
}

BOOL
Ext2SetAppAutorun(BOOL bAutorun)
{
    if (IsWindowsVistaOrGreater())
        return Ext2SetAppAutorunVista(bAutorun);

    return Ext2SetAppAutorunXP(bAutorun);
}


BOOL Ext2RunMgrForCurrentUser()
{
    if (IsWindowsVistaOrGreater())
        return Ext2RunMgrForCurrentUserVista();

    return Ext2RunMgrForCurrentUserXP();
}


#define SERVICE_CMD_LENGTH (MAX_PATH * 2)

int
Ext2SetManagerAsService(BOOL bInstall)
{
    SC_HANDLE   hService;
    SC_HANDLE   hManager;

    CHAR Target[SERVICE_CMD_LENGTH];

    // get the filename of this executable
    if (GetModuleFileName(NULL, Target, SERVICE_CMD_LENGTH - 20) == 0) {
        AfxMessageBox("Unable to install Ext2Mgr as service",
                      MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // append parameters to the end of the path:
    strcat(Target, " -service -hide");

    // open Service Control Manager
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL) {
        AfxMessageBox("Ext2Mgr: cannot open Service Control Manager",
                      MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (bInstall) {

        // now create service entry for Ext2Mgr
        hService = CreateService(
                hManager,                   // SCManager database
                "Ext2Mgr",                  // name of service
                "Ext2 Volume Manger",       // name to display
                SERVICE_ALL_ACCESS,	        // desired access
                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                            // service type
                SERVICE_AUTO_START,	        // start type
                SERVICE_ERROR_NORMAL,       // error control type
				Target,	                    // service's binary
                NULL,                       // no load ordering group
                NULL,                       // no tag identifier
                NULL,			            // dependencies
                NULL,						// LocalSystem account
                NULL);                      // no password

        if (hService == NULL) {
            DWORD error = GetLastError();
            if (error == ERROR_SERVICE_EXISTS) {
                AfxMessageBox("Ext2Mgr service is already registered.",
                              MB_ICONEXCLAMATION | MB_OK);
            } else {
                AfxMessageBox("Ext2Mgr service couldn't be registered.",
                              MB_ICONEXCLAMATION | MB_OK);
            }
        } else {

            CloseServiceHandle(hService);

			// got Ext2Mgr installed as a service
            AfxMessageBox(
                "Ext2Mgr service was successfully registered. \n\n"
				"You can modify the default settings and start/stop it from Control Panel.\n"
				"The service will automatically run the next time when system is restarted.\n",
				MB_ICONINFORMATION | MB_OK);
        }

        Ext2SetAppAutorun(FALSE);

    } else {

        /* open the service of Ext2Mgr */
        hService = OpenService(
                hManager,
                "Ext2Mgr",
                SERVICE_ALL_ACCESS
                );

        if (hService != NULL) {

#if 0
            SERVICE_STATUS status;

            // stop the service
            if (ControlService(hService, SERVICE_CONTROL_STOP, &status)) {

                while(QueryServiceStatus(hService, &status)) {
                    if (status.dwCurrentState == SERVICE_STOP_PENDING) {
                        Sleep(1000);
                    } else {
                        break;
                    }
                }

                if (status.dwCurrentState != SERVICE_STOPPED) {
                    AfxMessageBox("Could not stop Ext2Mgr service !",
                                  MB_ICONEXCLAMATION | MB_OK);
                }
            }
#endif
            // remove the service from the SCM
            if (DeleteService(hService)) {
                AfxMessageBox("The Ext2Mgr service has been unregistered.",
                MB_ICONINFORMATION | MB_OK);
            } else {
                DWORD error = GetLastError();
                if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
                    AfxMessageBox("Ext2Mgr service is already unregistered",
                                  MB_ICONEXCLAMATION | MB_OK);
                } else {
                    AfxMessageBox("Ext2Mgr service could not be unregistered",
                                   MB_ICONEXCLAMATION | MB_OK);
                }
            }

            CloseServiceHandle(hService);
        }
    }

    CloseServiceHandle(hManager);

	return TRUE;
}

BOOL g_bAutoRemoveDeadLetters = TRUE;

VOID
Ext2AddLetterMask(ULONGLONG LetterMask)
{
    Ext2DrvLetters[1] |= LetterMask;
}

VOID
Ext2AutoRemoveDeadLetters()
{
    ULONGLONG   LetterMask = Ext2DrvLetters[0];
    DWORD       i, j;
    PEXT2_DISK      disk;
    PEXT2_PARTITION part;
    PEXT2_CDROM     cdrom;

    if (LetterMask != -1) {
        AfxMessageBox("Different Mask");
    }

    for (i = 0; i < g_nDisks; i++) {
        disk = &gDisks[i];
        if (disk->DataParts == NULL) {
            continue;
        }
        for (j=0; j < disk->NumParts; j++) {
            part = &disk->DataParts[j];
            if (part) {
                 LetterMask &= ~(part->DrvLetters);
            }
        }
    }

    for (i = 0; i < g_nCdroms; i++) {
        cdrom = &gCdroms[i];
        LetterMask &= ~(cdrom->DrvLetters);
    }

    for (i=0; i < 10; i++) {
        if (drvDigits[i].bUsed && (drvDigits[i].Extent == NULL) &&
            (LetterMask & (((ULONGLONG) 1) << (i + 32)) ) ) {
            if (drvDigits[i].bInvalid && drvDigits[i].DrvType == DRIVE_FIXED) {
                LetterMask &= (~(((ULONGLONG) 1) << (i + 32)));
                Ext2RemoveMountPoint(&drvDigits[i], FALSE);
                Ext2RemoveDosSymLink(drvDigits[i].Letter);
            }
        }
    }

    for (i=2; i <26; i++) {
        if (drvLetters[i].bUsed && (drvLetters[i].Extent == NULL) &&
            (LetterMask & (((ULONGLONG) 1) << i)) ) {
            if (drvLetters[i].bInvalid && drvLetters[i].DrvType == DRIVE_FIXED) {
                LetterMask &= (~(((ULONGLONG) 1) << i));
                Ext2RemoveMountPoint(&drvLetters[i], FALSE);
                Ext2RemoveDosSymLink(drvLetters[i].Letter);
            }
        }
    }

    // Ext2DrvLetters[0] = LetterMask;
}

BOOL
Ext2RemoveDosSymLink(CHAR drvChar)
{
    EXT2_MOUNT_POINT             E2MP;
    NT::NTSTATUS                 status;
    HANDLE                       handle = NULL;
    NT::IO_STATUS_BLOCK          iosb;

    Ext2SymLinkRemoval(drvChar);

    memset(&E2MP, 0, sizeof(EXT2_MOUNT_POINT));
    E2MP.Magic = EXT2_APP_MOUNTPOINT_MAGIC;
    E2MP.Command = APP_CMD_DEL_DOS_SYMLINK;
    E2MP.Link[0] = (USHORT) drvChar;

    status = Ext2Open("\\DosDevices\\Ext2Fsd", 
                      &handle, EXT2_DESIRED_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    status = NT::ZwDeviceIoControlFile(
                handle, NULL, NULL, NULL, &iosb,
                IOCTL_APP_MOUNT_POINT,
                &E2MP, sizeof(EXT2_MOUNT_POINT),
                &E2MP, sizeof(EXT2_MOUNT_POINT)
            );

errorout:

    Ext2Close(&handle);

    return NT_SUCCESS(status);
}
