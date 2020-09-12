/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             cmcb.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#define CMCB_DEBUG_LEVEL DL_NVR

BOOLEAN
Ext2AcquireForLazyWrite (
    IN PVOID    Context,
    IN BOOLEAN  Wait)
{
    PEXT2_FCB    Fcb;

    Fcb = (PEXT2_FCB) Context;
    ASSERT(Fcb != NULL);
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if EXT2_DEBUG
    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireForLazyWrite: %s %s Fcb=%p\n",
                             Ext2GetCurrentProcessName(), "ACQUIRE_FOR_LAZY_WRITE", Fcb));
#endif
    if (!ExAcquireResourceExclusiveLite(Fcb->Header.Resource, Wait)) {
        return FALSE;
    }

    ASSERT(Fcb->LazyWriterThread == NULL);
    Fcb->LazyWriterThread = PsGetCurrentThread();

    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
Ext2ReleaseFromLazyWrite (IN PVOID Context)
{
    PEXT2_FCB Fcb = (PEXT2_FCB) Context;

    ASSERT(Fcb != NULL);

    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if EXT2_DEBUG
    DEBUG(CMCB_DEBUG_LEVEL, ( "Ext2ReleaseFromLazyWrite: %s %s Fcb=%p\n",
                              Ext2GetCurrentProcessName(), "RELEASE_FROM_LAZY_WRITE", Fcb));
#endif
    ASSERT(Fcb->LazyWriterThread == PsGetCurrentThread());
    Fcb->LazyWriterThread = NULL;

    ExReleaseResourceLite(Fcb->Header.Resource);

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp( NULL );
}

BOOLEAN
Ext2AcquireForReadAhead (IN PVOID    Context,
                         IN BOOLEAN  Wait)
{
    PEXT2_FCB    Fcb = (PEXT2_FCB) Context;

    ASSERT(Fcb != NULL);
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireForReadAhead: i=%xh Fcb=%p\n",
                             Fcb->Mcb->Inode.i_ino, Fcb));

    if (!ExAcquireResourceSharedLite(Fcb->Header.Resource, Wait))
        return FALSE;
    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
Ext2ReleaseFromReadAhead (IN PVOID Context)
{
    PEXT2_FCB Fcb = (PEXT2_FCB) Context;

    ASSERT(Fcb != NULL);

    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2ReleaseFromReadAhead: i=%xh Fcb=%p\n",
                             Fcb->Mcb->Inode.i_ino, Fcb));

    IoSetTopLevelIrp(NULL);
    ExReleaseResourceLite(Fcb->Header.Resource);
}

BOOLEAN
Ext2NoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
)
{
    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    return TRUE;
}

VOID
Ext2NoOpRelease (
    IN PVOID Fcb
)
{
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp( NULL );

    return;
}

