/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             shutdown.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2ShutDown)
#endif

NTSTATUS
Ext2ShutDown (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status;

    PIRP                    Irp;

    PEXT2_VCB               Vcb;
    PLIST_ENTRY             ListEntry;

    BOOLEAN                 GlobalResourceAcquired = FALSE;

    LARGE_INTEGER           SysTime, LinuxTime;

    __try {

        Status = STATUS_SUCCESS;

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        Irp = IrpContext->Irp;

        if (!ExAcquireResourceExclusiveLite(
                    &Ext2Global->Resource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            __leave;
        }

        GlobalResourceAcquired = TRUE;

        for (ListEntry = Ext2Global->VcbList.Flink;
                ListEntry != &(Ext2Global->VcbList);
                ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD(ListEntry, EXT2_VCB, Next);

            if (ExAcquireResourceExclusiveLite(
                        &Vcb->MainResource,
                        TRUE )) {

                if (IsMounted(Vcb)) {

                    /* update fs write time */
                    KeQuerySystemTime(&SysTime);
                    Ext2TimeToSecondsSince1970(&SysTime, &LinuxTime.LowPart, &LinuxTime.HighPart);
                    Vcb->SuperBlock->s_wtime = LinuxTime.LowPart;
                    Vcb->SuperBlock->s_wtime_hi = (UCHAR)LinuxTime.HighPart;

                    /* update mount count */
                    Vcb->SuperBlock->s_mnt_count++;
                    Ext2SaveSuper(IrpContext, Vcb);

                    /* flush dirty cache for all files */
                    Ext2FlushFiles(IrpContext, Vcb, TRUE);

                    /* flush volume stream's cache to disk */
                    Ext2FlushVolume(IrpContext, Vcb, TRUE);

                    /* send shutdown request to underlying disk */
                    Ext2DiskShutDown(Vcb);
                }

                ExReleaseResourceLite(&Vcb->MainResource);
            }
        }

        /*
                IoUnregisterFileSystem(Ext2Global->DiskdevObject);
                IoUnregisterFileSystem(Ext2Global->CdromdevObject);
        */

    } __finally {

        if (GlobalResourceAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    }

    return Status;
}
