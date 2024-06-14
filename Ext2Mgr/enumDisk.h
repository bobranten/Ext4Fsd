#ifndef _ENUM_DISK_INCLUDE_
#define _ENUM_DISK_INCLUDE_

#include "ntdll.h"
#include <objbase.h>
#include <initguid.h>
#include <cfgmgr32.h>
#include <setupapi.h> 
#include <regstr.h>
#include <winsvc.h>
#include "dbt.h"
#include "super.h"
#include "swap.h"
#include <winioctl.h>

//#include <devioctl.h> 
//#include <mountmgr.h>

#define SUPER_BLOCK_OFFSET              (0x400)
#define SUPER_BLOCK_SIZE                (0x400)

/******************************************************************
*                                                                 *
*  VersionHelpers.h -- This module defines helper functions to    *
*                      promote version check with proper          *
*                      comparisons.                               *
*                                                                 *
*  Copyright (c) Microsoft Corp.  All rights reserved.            *
*                                                                 *
******************************************************************/
#ifndef _versionhelpers_H_INCLUDED_
#define _versionhelpers_H_INCLUDED_
 
#ifndef ___XP_BUILD
#define WINAPI_PARTITION_DESKTOP   (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#define WINAPI_FAMILY_PARTITION(Partitions)     (Partitions)
 
#define _WIN32_WINNT_NT4                    0x0400
#define _WIN32_WINNT_WIN2K                  0x0500
#define _WIN32_WINNT_WINXP                  0x0501
#define _WIN32_WINNT_WS03                   0x0502
#define _WIN32_WINNT_WIN6                   0x0600
#define _WIN32_WINNT_VISTA                  0x0600
#define _WIN32_WINNT_WS08                   0x0600
#define _WIN32_WINNT_LONGHORN               0x0600
#define _WIN32_WINNT_WIN7                   0x0601
#define _WIN32_WINNT_WIN8                   0x0602
#endif
 
#ifdef _MSC_VER
#pragma once
#endif  // _MSC_VER
 
#ifdef __cplusplus
 
#define VERSIONHELPERAPI inline bool
 
#else  // __cplusplus
 
#define VERSIONHELPERAPI FORCEINLINE BOOL
 
#endif // __cplusplus
 
#define _WIN32_WINNT_WINBLUE                0x0603
#define _WIN32_WINNT_WIN10                  0x0A00
 
typedef NT::NTSTATUS( NTAPI* fnRtlGetVersion )(PRTL_OSVERSIONINFOW lpVersionInformation);
 
VERSIONHELPERAPI
IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
    /*OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG        const dwlConditionMask = VerSetConditionMask(
    VerSetConditionMask(
    VerSetConditionMask(
    0, VER_MAJORVERSION, VER_GREATER_EQUAL),
    VER_MINORVERSION, VER_GREATER_EQUAL),
    VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    osvi.dwMajorVersion = wMajorVersion;
    osvi.dwMinorVersion = wMinorVersion;
    osvi.wServicePackMajor = wServicePackMajor;
    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;*/
 
    RTL_OSVERSIONINFOEXW verInfo = { 0 };
    verInfo.dwOSVersionInfoSize = sizeof( verInfo );
 
    static fnRtlGetVersion RtlGetVersion = (fnRtlGetVersion)GetProcAddress( GetModuleHandleW( L"ntdll.dll" ), "RtlGetVersion" );
 
    if (RtlGetVersion != 0 && RtlGetVersion( (PRTL_OSVERSIONINFOW)&verInfo ) == 0)
    {
        if (verInfo.dwMajorVersion > wMajorVersion)
            return true;
        else if (verInfo.dwMajorVersion < wMajorVersion)
            return false;
 
        if (verInfo.dwMinorVersion > wMinorVersion)
            return true;
        else if (verInfo.dwMinorVersion < wMinorVersion)
            return false;
 
        if (verInfo.wServicePackMajor >= wServicePackMajor)
            return true;
    }
 
    return false;
}
 
VERSIONHELPERAPI
IsWindowsXPOrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WINXP ), LOBYTE( _WIN32_WINNT_WINXP ), 0 );
}
 
VERSIONHELPERAPI
IsWindowsXPSP1OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WINXP ), LOBYTE( _WIN32_WINNT_WINXP ), 1 );
}
 
VERSIONHELPERAPI
IsWindowsXPSP2OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WINXP ), LOBYTE( _WIN32_WINNT_WINXP ), 2 );
}
 
VERSIONHELPERAPI
IsWindowsXPSP3OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WINXP ), LOBYTE( _WIN32_WINNT_WINXP ), 3 );
}
 
VERSIONHELPERAPI
IsWindowsVistaOrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 );
}
 
VERSIONHELPERAPI
IsWindowsVistaSP1OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 1 );
}
 
VERSIONHELPERAPI
IsWindowsVistaSP2OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 2 );
}
 
VERSIONHELPERAPI
IsWindows7OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN7 ), LOBYTE( _WIN32_WINNT_WIN7 ), 0 );
}
 
VERSIONHELPERAPI
IsWindows7SP1OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN7 ), LOBYTE( _WIN32_WINNT_WIN7 ), 1 );
}
 
VERSIONHELPERAPI
IsWindows8OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN8 ), LOBYTE( _WIN32_WINNT_WIN8 ), 0 );
}
 
VERSIONHELPERAPI
IsWindows8Point1OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WINBLUE ), LOBYTE( _WIN32_WINNT_WINBLUE ), 0 );
}
 
VERSIONHELPERAPI
IsWindows10OrGreater()
{
    return IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN10 ), LOBYTE( _WIN32_WINNT_WIN10 ), 0 );
}
 
 
VERSIONHELPERAPI
IsWindowsServer()
{
    OSVERSIONINFOEXW osvi = { sizeof( osvi ), 0, 0, 0, 0, { 0 }, 0, 0, 0, VER_NT_WORKSTATION };
    DWORDLONG        const dwlConditionMask = VerSetConditionMask( 0, VER_PRODUCT_TYPE, VER_EQUAL );
 
    return !VerifyVersionInfoW( &osvi, VER_PRODUCT_TYPE, dwlConditionMask );
}
 
 
#endif // _VERSIONHELPERS_H_INCLUDED_

/*
 *  system definitions
 */

#define USING_IOCTL_EX TRUE

#if (USING_IOCTL_EX)

//
// New IOCTLs for GUID Partition tabled disks.
//


#define IOCTL_DISK_GET_DRIVE_LAYOUT_EXT    IOCTL_DISK_GET_DRIVE_LAYOUT_EX
#define IOCTL_DISK_SET_DRIVE_LAYOUT_EXT    IOCTL_DISK_SET_DRIVE_LAYOUT_EX
#define PARTITION_INFORMATION_EXT PARTITION_INFORMATION_EX
#define PPARTITION_INFORMATION_EXT PARTITION_INFORMATION_EX *
#define DRIVE_LAYOUT_INFORMATION_EXT DRIVE_LAYOUT_INFORMATION_EX
#define PDRIVE_LAYOUT_INFORMATION_EXT DRIVE_LAYOUT_INFORMATION_EX *

#else

#define IOCTL_DISK_GET_DRIVE_LAYOUT_EXT    IOCTL_DISK_GET_DRIVE_LAYOUT
#define IOCTL_DISK_SET_DRIVE_LAYOUT_EXT    IOCTL_DISK_SET_DRIVE_LAYOUT
typedef PARTITION_INFORMATION PARTITION_INFORMATION_EXT, *PPARTITION_INFORMATION_EXT;
typedef DRIVE_LAYOUT_INFORMATION DRIVE_LAYOUT_INFORMATION_EXT, *PDRIVE_LAYOUT_INFORMATION_EXT;

#endif // USING_IOCTL_EX



//
// Bus Type
//

static char* BusType[] = {
    "UNKNOWN",  // 0x00
    "SCSI",
    "ATAPI",
    "ATA",
    "IEEE 1394",
    "SSA",
    "FIBRE",
    "USB",
    "RAID"
};

//
// SCSI Device Type
//

static char* DeviceType[] = {
    "Direct Access Device", // 0x00
    "Tape Device",          // 0x01
    "Printer Device",       // 0x02
    "Processor Device",     // 0x03
    "WORM Device",          // 0x04
    "CDROM Device",         // 0x05
    "Scanner Device",       // 0x06
    "Optical Disk",         // 0x07
    "Media Changer",        // 0x08
    "Comm. Device",         // 0x09
    "ASCIT8",               // 0x0A
    "ASCIT8",               // 0x0B
    "Array Device",         // 0x0C
    "Enclosure Device",     // 0x0D
    "RBC Device",           // 0x0E
    "Unknown Device"        // 0x0F
};


/*
 * IFS format callbacks
 */

//
// Output command
//
typedef struct {
	DWORD Lines;
	PCHAR Output;
} TEXTOUTPUT, *PTEXTOUTPUT;

//
// Callback command types
//
typedef enum {
	PROGRESS,
	DONEWITHSTRUCTURE,
	UNKNOWN2,
	UNKNOWN3,
	UNKNOWN4,
	UNKNOWN5,
	INSUFFICIENTRIGHTS,
	UNKNOWN7,
	UNKNOWN8,
	UNKNOWN9,
	UNKNOWNA,
	DONE,
	UNKNOWNC,
	UNKNOWND,
	OUTPUT,
	STRUCTUREPROGRESS
} CALLBACKCOMMAND;

/* 
 *  ext2 codepages
 */

extern CHAR * gCodepages[];

//
// FMIFS callback definition
//
typedef BOOL (__stdcall *PFMIFSCALLBACK)( CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo ); 

//
// Chkdsk command in FMIFS
//
typedef VOID (__stdcall *PCHKDSK)( PWCHAR DriveRoot, 
						PWCHAR Format,
						BOOL CorrectErrors, 
						BOOL Verbose, 
						BOOL CheckOnlyIfDirty,
						BOOL ScanDrive, 
						PVOID Unused2, 
						PVOID Unused3,
						PFMIFSCALLBACK Callback );

//
// Format command in FMIFS
//

// media flags
#define FMIFS_HARDDISK 0xC
#define FMIFS_FLOPPY   0x8

typedef VOID (__stdcall *PFORMATEX)( PWCHAR DriveRoot,
						  DWORD MediaFlag,
						  PWCHAR Format,
						  PWCHAR Label,
						  BOOL QuickFormat,
						  DWORD ClusterSize,
						  PFMIFSCALLBACK Callback );

#include "..\ext4fsd\include\common.h"

/*
 * structure definitions
 */

typedef struct _EXT2_PARTITION *PEXT2_PARTITION;

typedef struct _EXT2_DISK {

    ULONG               Magic;
    ULONG               Null;
    CHAR                Name[256];
    ULONGLONG           Size;

    BOOL                bEjected;
    BOOL                bLoaded;
    BOOL                IsFile;
    UCHAR               OrderNo;
    UCHAR               NumParts;
    UCHAR               ExtStart;
    DISK_GEOMETRY       DiskGeometry;
    STORAGE_DEVICE_DESCRIPTOR   SDD;
    STORAGE_ADAPTER_DESCRIPTOR  SAD;
    PDRIVE_LAYOUT_INFORMATION_EXT Layout;

    PEXT2_PARTITION     DataParts;
} EXT2_DISK, *PEXT2_DISK;

#define EXT2_DISK_MAGIC      'EDSK'
#define EXT2_DISK_NULL_MAGIC 'ENUL'

typedef struct _EXT2_CDROM {
    ULONG               Magic[2];
    CHAR                Name[256];
    ULONGLONG           Size;

    UCHAR               OrderNo;
    BOOL                bLoaded;
    BOOL                bEjected;
    BOOL                bIsDVD;
    ULONGLONG           DrvLetters;

    DISK_GEOMETRY       DiskGeometry;
    STORAGE_DEVICE_DESCRIPTOR   SDD;
    STORAGE_ADAPTER_DESCRIPTOR  SAD;
    EXT2_VOLUME_PROPERTY3       EVP;
} EXT2_CDROM, *PEXT2_CDROM;

#define EXT2_CDROM_DEVICE_MAGIC 'ECDR'
#define EXT2_CDROM_VOLUME_MAGIC 'ECDV'


typedef struct _EXT2_VOLUME {
    ULONG                   Magic;
    struct _EXT2_VOLUME *   Next;
    CHAR                    Name[REGSTR_VAL_MAX_HCID_LEN];
    ULONGLONG               DrvLetters;
    BOOL                    bRecognized;
    BOOL                    bDynamic;
    PVOLUME_DISK_EXTENTS    Extent;
    NT::FILE_FS_DEVICE_INFORMATION FsdInfo;
    NT::FILE_FS_SIZE_INFORMATION   FssInfo;
    union {
        NT::FILE_FS_ATTRIBUTE_INFORMATION FsaInfo;
        CHAR _tmp_alinged_buf[MAX_PATH];
    };
    CHAR                    FileSystem[64];
    EXT2_VOLUME_PROPERTY3   EVP;
    PEXT2_PARTITION         Part;
} EXT2_VOLUME, *PEXT2_VOLUME;

#define EXT2_VOLUME_MAGIC 'EVOL'

typedef struct _EXT2_PARTITION {
    ULONG                   Magic;
    DWORD                   PartType;
    PEXT2_DISK              Disk;
    PPARTITION_INFORMATION_EXT  Entry;
    ULONGLONG               DrvLetters;
    PEXT2_VOLUME            Volume;
    UCHAR                   Number;
    CHAR                    Name[REGSTR_VAL_MAX_HCID_LEN];
} EXT2_PARTITION;
#define EXT2_PART_MAGIC 'EPRT'

typedef struct _EXT2_LETTER {

    UCHAR               Letter;
    BOOL                bInvalid;
    BOOL                bUsed;
    BOOL                bTemporary;
    UINT                DrvType;

    PVOLUME_DISK_EXTENTS    Extent;
    PSTORAGE_DEVICE_NUMBER  SDN;

    CHAR                SymLink[MAX_PATH];
} EXT2_LETTER, *PEXT2_LETTER;


/*
 * global definitions
 */

extern BOOL g_bAutoMount;
extern ULONG g_nFlps;
extern ULONG g_nDisks;
extern ULONG g_nCdroms;
extern ULONG g_nVols;

extern EXT2_LETTER drvLetters[26];
extern EXT2_LETTER drvDigits[10];

extern PEXT2_DISK      gDisks;
extern PEXT2_CDROM     gCdroms;
extern PEXT2_VOLUME    gVols;


/*
 * routines definitions
 */

char *PartitionString(int type);
char *DriveTypeString(UINT type);
char *DeviceTypeString(DEVICE_TYPE type);
char *BusTypeString(STORAGE_BUS_TYPE BusType);

BOOL IsWindows2000();
BOOL IsVistaOrAbove();

BOOL CanDoLocalMount();

#define EXT2_DESIRED_ACCESS (GENERIC_READ)

NT::NTSTATUS
Ext2Open(
    PCHAR               FileName,
    PHANDLE             Handle,
    ULONG               DesiredAccess
    );

VOID
Ext2Close(HANDLE*   Handle);

NT::NTSTATUS
Ext2WriteDisk(
    HANDLE              Handle,
    BOOL             IsFile,
    ULONG               SectorSize,
    ULONGLONG           Offset,
    ULONG               Length,
    PVOID               Buffer
    );

NT::NTSTATUS 
Ext2Read(
    IN  HANDLE          Handle,
    IN  BOOL         IsFile,
    IN  ULONG           SectorSize,
    IN  ULONGLONG       Offset,
    IN  ULONG           Length,
    IN  PVOID           Buffer
    );

NT::NTSTATUS
Ext2QueryDisk(
    HANDLE              Handle,
    PDISK_GEOMETRY      DiskGeometry
    );

PVOLUME_DISK_EXTENTS
Ext2QueryVolumeExtents(
    HANDLE              hVolume
    );

PVOLUME_DISK_EXTENTS
Ext2QueryDriveExtents(
    CHAR                DriveLetter
    );

BOOL
Ext2QueryDrvLetter(
    PEXT2_LETTER    drvLetter
    );

NT::NTSTATUS
Ext2QueryMediaType(
    HANDLE              Handle,
    PDWORD              MediaType
    );

NT::NTSTATUS
Ext2QueryProperty(
    HANDLE              Handle, 
    STORAGE_PROPERTY_ID Id,
    PVOID               DescBuf,
    ULONG               DescSize
    );

PDRIVE_LAYOUT_INFORMATION_EXT
Ext2QueryDriveLayout(
    HANDLE              Handle,
    PUCHAR              NumOfParts
    );

NT::NTSTATUS
Ext2SetDriveLayout(
    HANDLE  Handle,
    PDRIVE_LAYOUT_INFORMATION_EXT Layout
    );

BOOL
Ext2SetPartitionType(
    PEXT2_PARTITION Part,
    BYTE            Type
    );

PEXT2_PARTITION
Ext2QueryVolumePartition(
    PEXT2_VOLUME    Volume
    );

BOOL
Ext2FlushVolume(CHAR *Device);

BOOL
Ext2QuerySysConfig();

BOOL
Ext2CompareExtents(
    PVOLUME_DISK_EXTENTS ext1,
    PVOLUME_DISK_EXTENTS ext2
    );

ULONGLONG
Ext2QueryVolumeDrvLetters(PEXT2_VOLUME Volume);

VOID
Ext2QueryVolumeDisks(PEXT2_VOLUME Volume);

ULONGLONG
Ext2QueryCdromDrvLetters(PEXT2_CDROM Cdrom);

VOID
Ext2DrvNotify(UCHAR drive, int mount);

BOOL
Ext2QueryExt2Property (
    HANDLE                      Handle,
    PEXT2_VOLUME_PROPERTY3      EVP
    );

BOOL
Ext2QueryPerfStat (
    HANDLE                      Handle,
    PEXT2_QUERY_PERFSTAT        Stat,
    PEXT2_PERF_STATISTICS_V1   *PerfV1,
    PEXT2_PERF_STATISTICS_V2   *PerfV2
    );

BOOL Ext2IsNullUuid (__u8 * uuid);
BOOL
Ext2CheckVolumeRegistryProperty(
    PEXT2_VOLUME_PROPERTY3 EVP
    );

VOID
Ext2SetDefaultVolumeRegistryProperty(
    PEXT2_VOLUME_PROPERTY3 EVP
    );

VOID
Ext2StorePropertyinRegistry(
    PEXT2_VOLUME_PROPERTY3 EVP
    );

BOOL
Ext2SetExt2Property (
    HANDLE                Handle,
    PEXT2_VOLUME_PROPERTY3 EVP
    );

BOOL
Ext2QueryGlobalProperty(
    ULONG *     ulStartup,
    BOOL *   bReadonly,
    BOOL *   bExt3Writable,
    CHAR *      Codepage,
    CHAR *      sPrefix,
    CHAR *      sSuffix,
    BOOL *   bAutoMount
    );

INT
Ext2QueryDrvVersion(
    CHAR *      Version,
    CHAR *      Date,
    CHAR *      Time
    );

BOOL
Ext2SetGlobalProperty (
    ULONG       ulStartup,
    BOOLEAN     bReadonly,
    BOOLEAN     bExt3Writable,
    CHAR *      Codepage,
    CHAR *      sPrefix,
    CHAR *      sSuffix,
    BOOL     bAutoMount
    );

BOOL Ext2IsX64System();

BOOL
Ext2IsServiceStarted();

BOOL
Ext2StartService(CHAR *service);

BOOL
Ext2StartExt2Srv();

BOOL
Ext2StartExt2Fsd();

CString
Ext2SysInformation();

BOOL
Ext2LoadDisks();

BOOL Ext2ProcessExt2Volumes();

VOID
Ext2CleanupDisks();

BOOL
Ext2LoadCdroms();

VOID
Ext2LoadCdromDrvLetters();

VOID
Ext2CleanupCdroms();

BOOL
Ext2LoadDiskPartitions(PEXT2_DISK Disk);

VOID
Ext2LoadAllDiskPartitions();

VOID
Ext2MountingVolumes();

BOOL
Ext2LoadVolumes();

VOID
Ext2LoadAllVolumeDrvLetters();

BOOL
Ext2LoadRemovableVolumes();

CString
Ext2QueryVolumeLetterStrings(
    ULONGLONG       letters,
    PEXT2_LETTER *  first
    );

VOID
Ext2RefreshVLVI(
    CListCtrl *List,
    PEXT2_VOLUME chain,
    int  nItem
    );

VOID
Ext2InsertVolume(
    CListCtrl *List,
    PEXT2_VOLUME chain
    );

VOID
Ext2RefreshVLCD(
    CListCtrl *List,
    PEXT2_CDROM Cdrom,
    int nItem
    );

VOID
Ext2InsertCdromAsVolume(
    CListCtrl *List,
    PEXT2_CDROM Cdrom
    );


VOID
Ext2RefreshVolumeList(CListCtrl *List);

VOID
Ext2RefreshDVPT(
    CListCtrl*      List,
    PEXT2_PARTITION Part,
    int nItem
    );

VOID
Ext2InsertPartition(
    CListCtrl*      List,
    PEXT2_DISK      Disk,
    PEXT2_PARTITION Part
    );

VOID
Ext2InsertDisk(
    CListCtrl *List,
    PEXT2_DISK Disk
    );

VOID
Ext2RefreshDVCM(
    CListCtrl *List,
    PEXT2_CDROM Cdrom,
    int nItem
    );

VOID
Ext2InsertCdromAsDisk(
    CListCtrl *List,
    PEXT2_CDROM Cdrom
    );

VOID
Ext2RefreshDiskList(CListCtrl *List);

VOID
Ext2CleanupVolumes();

VOID
Ext2LoadDrvLetter(PEXT2_LETTER drvLetter, CHAR cLetter);

VOID
Ext2LoadDrvLetters();

VOID
Ext2CleanDrvLetter(PEXT2_LETTER drvLetter);

VOID
Ext2CleanupDrvLetters();

BOOL
Ext2RemoveDrvLetter(
    PEXT2_LETTER   drvLetter
    );
BOOL
Ext2RemoveDriveLetter(CHAR DrvLetter);

CHAR
Ext2QueryRegistryMountPoint (
    CHAR * devName
    );

BOOL
Ext2SetRegistryMountPoint (
    CHAR * dosPath,
    CHAR * devName,
    BOOL bSet
    );

BOOL
Ext2InsertMountPoint(
    CHAR * volume,
    UCHAR drvChar,
    BOOL  bGlobal
    );

VOID
Ext2UpdateDrvLetter(
    PEXT2_LETTER   drvLetter,
    PCHAR          devPath
    );

BOOL
Ext2AssignDrvLetter(
    PEXT2_LETTER   drvLetter,
    PCHAR          devPath,
    BOOL        bPermanent
    );

BOOL Ext2IsDrvLetterAvailable(CHAR drive);
CHAR Ext2MountVolume(CHAR *voldev);

CHAR Ext2MountVolumeAs(CHAR *voldev, CHAR letter);

VOID
Ext2RemoveMountPoint(
    PEXT2_LETTER    drvLetter,
    BOOL         bPermanent
    );

BOOL
Ext2SymLinkRemoval(CHAR drvLetter);

BOOL
Ext2SetVolumeMountPoint (
    CHAR * dosPath,
    CHAR * devName
    );

UCHAR
Ext2QueryMountPoint(
    CHAR *      volume
    );

BOOL
Ext2RefreshVolumePoint(
    CHAR *          volume,
    UCHAR           drvChar
    );

BOOL
Ext2NotifyVolumePoint(
    PEXT2_VOLUME    volume,
    UCHAR           drvChar
    );

BOOL
Ext2VolumeArrivalNotify(PCHAR  VolumePath);

BOOL
Ext2SetAppAutorun(BOOL bInstall);

BOOL Ext2RunMgrForCurrentUser();

int
Ext2SetManagerAsService(BOOL bInstall);

extern BOOL g_bAutoRemoveDeadLetters;

VOID
Ext2AddLetterMask(ULONGLONG LetterMask);

VOID
Ext2AutoRemoveDeadLetters();

BOOL
Ext2RemoveDosSymLink(CHAR drvChar);

BOOL Ext2DismountVolume(CHAR *voldev);

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
WTSQueryUserToken(
    ULONG SessionId, 
    PHANDLE phToken
    );

BOOL WINAPI ConvertStringSidToSidA(
  LPCTSTR StringSid,
  PSID* Sid
);

WINUSERAPI
HWND
WINAPI
GetShellWindow(
    VOID);

#ifdef __cplusplus
}
#endif

#endif // _ENUM_DISK_INCLUDE_
