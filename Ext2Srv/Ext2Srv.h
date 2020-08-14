#ifndef _EXT2_SRV_INCLUDE_
#define _EXT2_SRV_INCLUDE_

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <crtdbg.h>
#include <process.h>
#include <WtsApi32.h>
#include <dbt.h>
#include <shellapi.h>

/*
 * global defintions
 */

#define CL_ASSERT(cond) do {switch('x') {case (cond): case 0: break;}} while (0)

#define DEBUG(...)  do {} while(0)

/*
 * resources
 */

#define IDI_MAINFRAME  101

/*
 * global defintions
 */

#ifdef _DEBUG

#define EXT2_BREAK()    DebugBreak()
#define EXT2_ASSERT(X)  _ASSERT(X)

#else

#define EXT2_BREAK()    do {} while(0)
#define EXT2_ASSERT(X)  do {} while(0)
#endif

/*
 * log filter level
 */


#define EXT2_LOG_CONSOLE        (0x80000000)	/* print to screen */
#define EXT2_LOG_ERROR          (0x00000001)	/* critical messagre */
#define EXT2_LOG_MESSAGE        (0x00000002)	/* information */

#define EXT2_LOG_DUMP           (0x40000002)	/* dump and message*/


void Ext2Log(DWORD ll, char *fn, int ln, char *format, ... );

/*
 * message / error log
 */
#define MsgDump(F, ...)                                             \
    do {                                                            \
        Ext2Log(EXT2_LOG_DUMP, __FUNCTION__, __LINE__,              \
                ##F, ##__VA_ARGS__);                                \
    } while(0)

#define MsgLog(F, ...)                                              \
    do {                                                            \
        Ext2Log(EXT2_LOG_MESSAGE, __FUNCTION__, __LINE__,           \
                ##F, ##__VA_ARGS__);                                \
    } while(0)


#define ErrLog(F, ...)                                              \
    do {                                                            \
        Ext2Log(EXT2_LOG_ERROR, __FUNCTION__, __LINE__,             \
                ##F, ##__VA_ARGS__);                                \
    } while(0)



/*
 * structure definitions
 */


typedef struct _EXT2_PIPE {
    struct _EXT2_PIPE  *l;          /* next pipe handle */
    HANDLE              p;          /* pipe handle */
    HANDLE              e;          /* event handle */
    HANDLE              q;          /* quiting */
    OVERLAPPED          o;          /* overlap info */
    volatile BOOL       s;          /* stop flag */
} EXT2_PIPE, *PEXT2_PIPE;


/*
 * Ext2Pipe.cpp
 */


DWORD Ext2StartPipeSrv();
VOID  Ext2StopPipeSrv();

INT   Ext2NotifyUser(TCHAR *task, ULONG pid);

/*
 * Mount.cpp (native API)
 */

BOOL Ext2EnablePrivilege(LPCTSTR lpszPrivilegeName);

VOID
Ext2DrvNotify(TCHAR drive, int add);

BOOL Ext2AssignDrvLetter(TCHAR *dev, TCHAR drv);
BOOL Ext2RemoveDrvLetter(TCHAR drive);

int Ext2StartUserTask(TCHAR *usr, TCHAR *srv, DWORD sid, BOOL);
INT Ext2StartMgrAsUser();

/*
 * Ext2Srv.cpp
 */

VOID Ext2DrivesChangeNotify(BOOLEAN bArrival);

#endif /* _EXT2_SRV_INCLUDE_ */
