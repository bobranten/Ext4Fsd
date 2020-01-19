/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             read.c
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
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadFile (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext);

/* FUNCTIONS *************************************************************/

NTSTATUS
Ext2CompleteIrpContext (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status )
{
    PIRP    Irp = NULL;
    BOOLEAN bPrint;

    Irp = IrpContext->Irp;

    if (Irp != NULL) {

        if (NT_ERROR(Status)) {
            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;
        bPrint = !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

        Ext2CompleteRequest(
            Irp, bPrint, (CCHAR)(NT_SUCCESS(Status)?
                                 IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;
    }

    Ext2FreeIrpContext(IrpContext);

    return Status;
}


NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_CCB           Ccb = NULL;
    PEXT2_FCBVCB        FcbOrVcb = NULL;
    PFILE_OBJECT        FileObject = NULL;

    PDEVICE_OBJECT      DeviceObject = NULL;

    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IoStackLocation = NULL;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;

    PUCHAR              Buffer = NULL;
    EXT2_EXTENT         BlockArray;

    __try {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;
        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        ASSERT(FcbOrVcb);

        if (!(FcbOrVcb->Identifier.Type == EXT2VCB && (PVOID)FcbOrVcb == (PVOID)Vcb)) {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        Irp = IrpContext->Irp;
        Irp->IoStatus.Information = 0;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE) || (Ccb != NULL);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }

        if (ByteOffset.QuadPart >=
                Vcb->PartitionInformation.PartitionLength.QuadPart  ) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            __leave;
        }

        if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
            Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
        }

        /*
         *  User direct volume access
         */

        if (Ccb != NULL && !PagingIo) {

            if (!ExAcquireResourceExclusiveLite(
                        &Vcb->MainResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                __leave;
            }
            MainResourceAcquired = TRUE;

            if (!FlagOn(Ccb->Flags, CCB_VOLUME_DASD_PURGE)) {

                if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
                    Ext2FlushVolume(IrpContext, Vcb, FALSE);
                }

                SetFlag(Ccb->Flags, CCB_VOLUME_DASD_PURGE);
            }

            ExReleaseResourceLite(&Vcb->MainResource);
            MainResourceAcquired = FALSE;

            /* will do Nocache i/o */
        }

        /*
         *  I/O to volume StreamObject
         */

        if (!Nocache) {

            if (IsFlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcMdlRead(
                    Vcb->Volume,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

            } else {

                Buffer = Ext2GetUserBuffer(Irp);
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    __leave;
                }

                if (!CcCopyRead(
                            Vcb->Volume,
                            &ByteOffset,
                            Length,
                            Ext2CanIWait(),
                            Buffer,
                            &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    __leave;
                }

                Status = Irp->IoStatus.Status;
            }

        } else {

            Length &= ~((ULONG)SECTOR_SIZE - 1);
            Status = Ext2LockUserBuffer(
                         IrpContext->Irp,
                         Length,
                         IoWriteAccess );

            if (!NT_SUCCESS(Status)) {
                __leave;
            }

            BlockArray.Irp = NULL;
            BlockArray.Lba = ByteOffset.QuadPart;
            BlockArray.Offset = 0;
            BlockArray.Length = Length;
            BlockArray.Next = NULL;

            Status = Ext2ReadWriteBlocks(IrpContext,
                                         Vcb,
                                         &BlockArray,
                                         Length );

            Irp = IrpContext->Irp;
            if (!Irp) {
                __leave;
            }
        }

    } __finally {

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            if (Irp) {

                if (Status == STATUS_PENDING &&
                    !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED)) {

                    Status = Ext2LockUserBuffer(
                                 IrpContext->Irp,
                                 Length,
                                 IoWriteAccess );

                    if (NT_SUCCESS(Status)) {
                        Status = Ext2QueueRequest(IrpContext);
                    } else {
                        Ext2CompleteIrpContext(IrpContext, Status);
                    }

                } else {

                    if (NT_SUCCESS(Status)) {

                        if (!PagingIo) {

                            if (SynchronousIo) {

                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }

                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);;
                }

            } else {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }

    return Status;
}


#define SafeZeroMemory(AT,BYTE_COUNT) {                                 \
    __try {                                                             \
        if (AT)                                                         \
            RtlZeroMemory((AT), (BYTE_COUNT));                          \
    } __except(EXCEPTION_EXECUTE_HANDLER) {                             \
         Ext2RaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER );     \
    }                                                                   \
}

NTSTATUS
Ext2ReadInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bDirectIo,
    OUT PULONG              BytesRead
)
{
    PEXT2_EXTENT    Chain = NULL;
    PEXT2_EXTENT    Extent = NULL, Prev = NULL;

    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    ULONG           RealSize ;

    if (BytesRead) {
        *BytesRead = 0;
    }

    __try {

        Ext2ReferMcb(Mcb);

        ASSERT((Mcb->Identifier.Type == EXT2MCB) &&
               (Mcb->Identifier.Size == sizeof(EXT2_MCB)));

        if ((Mcb->Identifier.Type != EXT2MCB) ||
                (Mcb->Identifier.Size != sizeof(EXT2_MCB))) {
            __leave;
        }

        if (Buffer == NULL && IrpContext != NULL)
            Buffer = Ext2GetUserBuffer(IrpContext->Irp);


        /* handle fast symlinks */
        if (S_ISLNK(Mcb->Inode.i_mode) && 0 == Mcb->Inode.i_blocks) {

            PUCHAR Data = (PUCHAR) (&Mcb->Inode.i_block[0]);
            if (!Buffer) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            if (Offset < EXT2_LINKLEN_IN_INODE) {
                if ((ULONG)Offset + Size >= EXT2_LINKLEN_IN_INODE)
                    Size = EXT2_LINKLEN_IN_INODE - (ULONG)Offset - 1;
                RtlCopyMemory(Buffer, Data + (ULONG)Offset, Size);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_END_OF_FILE;
            }
            __leave;
        }

        //
        // Build the scatterred block ranges to be read
        //

        if (bDirectIo) {
            RealSize = CEILING_ALIGNED(ULONG, Size, SECTOR_SIZE - 1);
        } else {
            RealSize = Size;
        }

        Status = Ext2BuildExtents(
                     IrpContext,
                     Vcb,
                     Mcb,
                     Offset,
                     RealSize,
                     FALSE,
                     &Chain
                 );

        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        if (Chain == NULL) {
            SafeZeroMemory((PCHAR)Buffer, Size);
            Status = STATUS_SUCCESS;
            __leave;
        }

        /* for sparse file, we need zero the gaps */
        for (Extent = Chain; Buffer != NULL && Extent != NULL; Extent = Extent->Next) {
            if (NULL == Prev) {
                ASSERT(Extent == Chain);
                if (Extent->Offset) {
                    SafeZeroMemory((PCHAR)Buffer, Extent->Offset);
                }
            } else if (Extent->Offset > (Prev->Offset + Prev->Length)) {
                SafeZeroMemory((PCHAR)Buffer + Prev->Offset + Prev->Length,
                               Extent->Offset - Prev->Offset - Prev->Length);
            }
            if (NULL == Extent->Next) {
                if (Extent->Offset + Extent->Length < Size) {
                    SafeZeroMemory((PCHAR)Buffer + Extent->Offset + Extent->Length,
                                   Size - Extent->Offset - Extent->Length);
                }
            }
            Prev = Extent;
        }

        if (bDirectIo) {

            ASSERT(IrpContext != NULL);

            // Offset should be SECTOR_SIZE aligned ...
            Status = Ext2ReadWriteBlocks(
                         IrpContext,
                         Vcb,
                         Chain,
                         Size
                     );
        } else {

            for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {

                if (!CcCopyRead(
                            Vcb->Volume,
                            (PLARGE_INTEGER)(&(Extent->Lba)),
                            Extent->Length,
                            PIN_WAIT,
                            (PVOID)((PUCHAR)Buffer + Extent->Offset),
                            &IoStatus
                        )) {
                    Status = STATUS_CANT_WAIT;
                } else {
                    Status = IoStatus.Status;
                }

                if (!NT_SUCCESS(Status)) {
                    break;
                }
            }
        }

    } __finally {

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        Ext2DerefMcb(Mcb);
    }

    if (NT_SUCCESS(Status)) {
        if (BytesRead)
            *BytesRead = Size;
    }

    return Status;
}

NTSTATUS
Ext2ReadFile(IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb = NULL;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_CCB           Ccb = NULL;
    PFILE_OBJECT        FileObject = NULL;

    PDEVICE_OBJECT      DeviceObject = NULL;

    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IoStackLocation = NULL;

    ULONG               Length;
    ULONG               ReturnedLength = 0;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             OpPostIrp = FALSE;
    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;

    __try {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        ASSERT(Fcb);
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        DEBUG(DL_INF, ("Ext2ReadFile: reading %wZ Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                       &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (IsSpecialFile(Fcb) || IsInodeSymLink(Fcb->Inode) ) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        if ((IsSymLink(Fcb) && IsFileDeleted(Fcb->Mcb->Target)) ||
            IsFileDeleted(Fcb->Mcb)) {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
            ByteOffset.HighPart == -1) {
            ByteOffset = FileObject->CurrentByteOffset;
        }

        if (Nocache && (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
                        Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            DbgBreak();
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            DbgBreak();
            __leave;
        }

        ReturnedLength = Length;

        if (PagingIo) {

            if (!ExAcquireResourceSharedLite(
                        &Fcb->PagingIoResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                __leave;
            }
            PagingIoResourceAcquired = TRUE;

        } else {

            if (Nocache && Ccb != NULL && Fcb->SectionObject.DataSectionObject) {

                if (!ExAcquireResourceExclusiveLite(
                            &Fcb->MainResource,
                            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    __leave;
                }
                MainResourceAcquired = TRUE;

                CcFlushCache(&Fcb->SectionObject,
                             &ByteOffset,
                              Length,
                             &Irp->IoStatus );
                if (!NT_SUCCESS(Irp->IoStatus.Status))
                    __leave;
                ClearLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);

                if (ExAcquireResourceExclusiveLite(&(Fcb->PagingIoResource), TRUE)) {
                    ExReleaseResourceLite(&(Fcb->PagingIoResource));
                }
                CcPurgeCacheSection( &Fcb->SectionObject,
                                     NULL,
                                     0,
                                     FALSE );

                ExConvertExclusiveToShared(&Fcb->MainResource);

            } else {

                if (!ExAcquireResourceSharedLite(
                            &Fcb->MainResource,
                            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    __leave;
                }
                MainResourceAcquired = TRUE;
            }

            if (!FsRtlCheckLockForReadAccess(
                        &Fcb->FileLockAnchor,
                        Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                __leave;
            }
        }

        if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->Header.FileSize.QuadPart) {
            if (ByteOffset.QuadPart >= Fcb->Header.FileSize.QuadPart) {
                Irp->IoStatus.Information = 0;
                Status = STATUS_END_OF_FILE;
                __leave;
            }
            ReturnedLength = (ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
        }


        if (!IsDirectory(Fcb) && Ccb != NULL) {

            Status = FsRtlCheckOplock( &Fcb->Oplock,
                                       Irp,
                                       IrpContext,
                                       Ext2OplockComplete,
                                       Ext2LockIrp );

            if (Status != STATUS_SUCCESS) {
                OpPostIrp = TRUE;
                __leave;
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
        }

        if (!Nocache) {

            if (IsDirectory(Fcb)) {
                __leave;
            }

            if (FileObject->PrivateCacheMap == NULL) {
                CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
                        FALSE,
                        &Ext2Global->CacheManagerCallbacks,
                        Fcb );
                CcSetReadAheadGranularity(
                        FileObject,
                        READ_AHEAD_GRANULARITY );
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {
                CcMdlRead(
                    FileObject,
                    (&ByteOffset),
                    ReturnedLength,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

            } else {

                Buffer = Ext2GetUserBuffer(Irp);
                if (Buffer == NULL) {
                    Status = STATUS_INVALID_USER_BUFFER;
                    DbgBreak();
                    __leave;
                }

                if (!CcCopyRead(FileObject, &ByteOffset, ReturnedLength,
                                Ext2CanIWait(), Buffer, &Irp->IoStatus)) {

                    if (Ext2CanIWait() || !CcCopyRead(FileObject, &ByteOffset,
                                                      ReturnedLength, TRUE,
                                                      Buffer, &Irp->IoStatus)) {
                        Status = STATUS_PENDING;
                        DbgBreak();
                        __leave;
                    }
                }
                Status = Irp->IoStatus.Status;
            }

        } else {

            ULONG   BytesRead = ReturnedLength;
            PUCHAR  SystemVA  = Ext2GetUserBuffer(IrpContext->Irp);

            if (ByteOffset.QuadPart + BytesRead > Fcb->Header.ValidDataLength.QuadPart) {

                if (ByteOffset.QuadPart >= Fcb->Header.ValidDataLength.QuadPart) {
                    if (SystemVA) {
                        SafeZeroMemory(SystemVA, Length);
                    }
                    Irp->IoStatus.Information = ReturnedLength;
                    Status = STATUS_SUCCESS;
                    __leave;
                } else {
                    BytesRead = (ULONG)(Fcb->Header.ValidDataLength.QuadPart - ByteOffset.QuadPart);
                    if (SystemVA) {
                        SafeZeroMemory(SystemVA + BytesRead, Length - BytesRead);
                    }
                }
            }

            Status = Ext2LockUserBuffer(
                         IrpContext->Irp,
                         BytesRead,
                         IoReadAccess );

            if (!NT_SUCCESS(Status)) {
                __leave;
            }

            Status = Ext2ReadInode(
                         IrpContext,
                         Vcb,
                         Fcb->Mcb,
                         ByteOffset.QuadPart,
                         NULL,
                         BytesRead,
                         TRUE,
                         NULL );

            /* we need re-queue this request in case STATUS_CANT_WAIT
               and fail it in other failure cases  */
            if (!NT_SUCCESS(Status)) {
                __leave;
            }

            /* pended by low level device */
            if (Status == STATUS_PENDING) {
                IrpContext->Irp = Irp = NULL;
                __leave;
            }

            Irp = IrpContext->Irp;
            ASSERT(Irp);
            Status = Irp->IoStatus.Status;

            if (!NT_SUCCESS(Status)) {
                Ext2NormalizeAndRaiseStatus(IrpContext, Status);
            }
        }

        Irp->IoStatus.Information = ReturnedLength;

    } __finally {

        if (Irp) {
            if (PagingIoResourceAcquired) {
                ExReleaseResourceLite(&Fcb->PagingIoResource);
            }

            if (MainResourceAcquired) {
                ExReleaseResourceLite(&Fcb->MainResource);
            }
        }

        if (!OpPostIrp && !IrpContext->ExceptionInProgress) {

            if (Irp) {
                if ( Status == STATUS_PENDING ||
                     Status == STATUS_CANT_WAIT) {

                    Status = Ext2LockUserBuffer(
                                 IrpContext->Irp,
                                 Length,
                                 IoWriteAccess );

                    if (NT_SUCCESS(Status)) {
                        Status = Ext2QueueRequest(IrpContext);
                    } else {
                        Ext2CompleteIrpContext(IrpContext, Status);
                    }
                } else {
                    if (NT_SUCCESS(Status)) {
                        if (!PagingIo) {
                            if (SynchronousIo) {
                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }
                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);
                }

            } else {

                Ext2FreeIrpContext(IrpContext);
            }
        }
    }

    DEBUG(DL_IO, ("Ext2ReadFile: %wZ fetch at Off=%I64xh Len=%xh Paging=%xh Nocache=%xh Returned=%xh Status=%xh\n",
                  &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache, ReturnedLength, Status));
    return Status;

}

NTSTATUS
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;

    __try {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        FileObject = IrpContext->FileObject;
        Irp = IrpContext->Irp;

        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        Status = STATUS_SUCCESS;

    } __finally {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }

    return Status;
}


NTSTATUS
Ext2Read (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PEXT2_VCB           Vcb;
    PEXT2_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    BOOLEAN             bCompleteRequest;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    __try {

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {

            Status =  Ext2ReadComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (IsExt2FsDevice(DeviceObject)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;
                __leave;
            }

            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
            if (Vcb->Identifier.Type != EXT2VCB ||
                    Vcb->Identifier.Size != sizeof(EXT2_VCB) ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;

                __leave;
            }

            FileObject = IrpContext->FileObject;

            if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
                Vcb->LockFile != FileObject ) {
                Status = STATUS_ACCESS_DENIED;
                __leave;
            }

            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB) {

                Status = Ext2ReadVolume(IrpContext);
                bCompleteRequest = FALSE;

            } else if (FcbOrVcb->Identifier.Type == EXT2FCB) {

                if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                    Status = STATUS_TOO_LATE;
                    bCompleteRequest = TRUE;
                    __leave;
                }

                Status = Ext2ReadFile(IrpContext);
                bCompleteRequest = FALSE;
            } else {
                DEBUG(DL_ERR, ( "Ext2Read: Inavlid FileObject (Vcb or Fcb corrupted)\n"));
                DbgBreak();

                Status = STATUS_INVALID_PARAMETER;
                bCompleteRequest = TRUE;
            }
        }

    } __finally {
        if (bCompleteRequest) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }

    return Status;
}
