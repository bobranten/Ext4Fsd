#ifndef EXT2_SRV_PIPE_H
#define EXT2_SRV_PIPE_H


#define EXT2_MGR_SRV    "\\\\.\\pipe\\EXT2MGR_PSRV"

#define PIPE_REQ_MAGIC  0xBAD0BAD8
#define REQ_BODY_SIZE   (4096)

#pragma pack(1)
#pragma warning(disable: 4200)

typedef struct _PIPE_REQ {

    ULONG           magic;
    ULONG           flag;
    ULONG           cmd;
    ULONG           len;
    CHAR            data[0];
} PIPE_REQ, *PPIPE_REQ;

#define CMD_QUERY_DRV   0xBAD00001
#define CMD_DEFINE_DRV  0xBAD00002
#define CMD_REMOVE_DRV  0xBAD00003


typedef struct _REQ_QUERY_DRV {
    ULONG           type;
    UCHAR           drive;
    UCHAR           result;
    USHORT          symlink;
    char            name[0];
} REQ_QUERY_DRV, *PREQ_QUERY_DRV;


typedef struct _REQ_DEFINE_DRV {
    ULONG           pid;
    ULONG           flags;
    UCHAR           drive;
    UCHAR           result;
    USHORT          symlink;
    char            name[0];
} REQ_DEFINE_DRV, *PREQ_DEFINE_DRV,
  REQ_REMOVE_DRV, *PREQ_REMOVE_DRV;

#pragma warning(default: 4200)
#pragma pack()

#endif // EXT2_SRV_PIPE_H