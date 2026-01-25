/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             close.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS
Ext2Close (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb = NULL;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb = NULL;
    PEXT2_CCB       Ccb = NULL;

    BOOLEAN         VcbResourceAcquired = FALSE;
    BOOLEAN         FcbResourceAcquired = FALSE;
    BOOLEAN         FcbDerefDeferred = FALSE;

    __try {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_SUCCESS;
            Vcb = NULL;
            __leave;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        if (!Fcb) {
            Status = STATUS_SUCCESS;
            __leave;
        }
        ASSERT(Fcb != NULL);
        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        DEBUG(DL_INF, ( "Ext2Close: (VCB) Vcb = %p ReferCount = %d\n",
                         Vcb, Vcb->ReferenceCount));

        /*
         * WARNING: don't release Vcb resource lock here.
         *
         *  CcPurgeCacheSection will lead a recursive irp: IRP_MJ_CLOSE
         *  which would cause revrese order of lock acquirision:
         *  1) IRP_MJ_CLEANUP: a) Vcb lock -> b) Fcb lock
         *  2) IRP_MJ_CLOSE:   c) Vcb lock -> d) Fcb lock
         */

        if (Fcb->Identifier.Type == EXT2VCB) {

            ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                TRUE);
            VcbResourceAcquired = TRUE;

            if (Ccb) {

                Ext2DerefXcb(&Vcb->ReferenceCount);
                Ext2FreeCcb(Vcb, Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }

            Status = STATUS_SUCCESS;
            __leave;
        }

        if ( Fcb->Identifier.Type != EXT2FCB ||
             Fcb->Identifier.Size != sizeof(EXT2_FCB)) {
            __leave;
        }

        ExAcquireResourceExclusiveLite(
            &Fcb->MainResource,
            TRUE);
        FcbResourceAcquired = TRUE;

        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

        if (Ccb == NULL ||
            Ccb->Identifier.Type != EXT2CCB ||
            Ccb->Identifier.Size != sizeof(EXT2_CCB)) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        DEBUG(DL_INF, ( "Ext2Close: Fcb = %p OpenHandleCount= %u ReferenceCount=%u NonCachedCount=%u %wZ\n",
                        Fcb, Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount, &Fcb->Mcb->FullName ));

        Ext2FreeCcb(Vcb, Ccb);
        if (FileObject) {
            FileObject->FsContext2 = Ccb = NULL;
        }

        /* only deref fcb, Ext2ReleaseFcb might lead deadlock */
        FcbDerefDeferred = TRUE;
        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING) ||
            NULL == Fcb->Mcb ||
            IsFileDeleted(Fcb->Mcb)) {
            Fcb->TsDrop.QuadPart = 0;
        } else {
            KeQuerySystemTime(&Fcb->TsDrop);
        }
        Ext2DerefXcb(&Vcb->ReferenceCount);

        if (FileObject) {
            FileObject->FsContext = NULL;
        }

        Status = STATUS_SUCCESS;

    } __finally {

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            Ext2CompleteIrpContext(IrpContext, Status);
        }

        if (FcbDerefDeferred)
            Ext2DerefXcb(&Fcb->ReferenceCount);
    }

    return Status;
}
