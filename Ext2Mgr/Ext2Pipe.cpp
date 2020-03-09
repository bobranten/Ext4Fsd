
#include "stdafx.h"
#include <dbt.h>
#include <tchar.h>
#include < tlhelp32.h>
#include "..\Ext2Srv\Ext2Pipe.h"


/*
 * global defintions
 */

#define CL_ASSERT(cond) do {switch('x') {case (cond): case 0: break;}} while (0)


/*
 * glboal variables
 */

/* pipe handles */

HANDLE g_hPipe = NULL;


/*
 * function body
 */

int Ext2StartSrv();

HANDLE Ext2OpenPipe(CHAR *PipeName)
{
    DWORD   rc;

    if (g_hPipe != NULL && g_hPipe != INVALID_HANDLE_VALUE) {
        return g_hPipe;
    }

retry:

    // open pipe (created by Ext2Srv)
    g_hPipe = CreateFile(PipeName,
                         GENERIC_READ | GENERIC_WRITE, 
                         0, NULL, OPEN_EXISTING, 0, NULL);
 
    // exit if the pipe handle is valid. 
    if (g_hPipe == INVALID_HANDLE_VALUE) {

         // exit if an error other than ERROR_PIPE_BUSY occurs. 
        rc = GetLastError(); 
        if (rc != ERROR_PIPE_BUSY) {
            return NULL;
        }
 
        // all pipe instances are busy, so wait for 20 seconds. 
        if (!WaitNamedPipe(PipeName, 20000)) { 
            return NULL;
        }

        Ext2StartSrv();

        goto retry;
    }

    return g_hPipe;
}

void Ext2ClosePipe()
{
    if (g_hPipe != NULL && g_hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hPipe);
        g_hPipe = NULL;
    }
}

BOOL Ext2ReadPipe(HANDLE p, PVOID b, DWORD c, PDWORD r)
{
    DWORD bytes = 0, total = 0;
    BOOL  rc = FALSE;

    while (total < c) {
        rc = ReadFile(p,(PCHAR)b + total, c - total, &bytes, NULL);
        if (rc) {
            total += bytes;
        } else {
            break;
        }
    }

    if (r)
        *r = total;
    return rc;
}


BOOL Ext2WritePipe(HANDLE p, PVOID b, DWORD c, PDWORD w)
{
    DWORD bytes = 0, total = 0;
    BOOL  rc = FALSE;

    while (total < c) {
        rc = WriteFile(p, (PCHAR)b + total, c - total, &bytes, NULL);
        if (rc) {
            total += bytes;
        } else {
            break;
        }
    }

    if (w)
        *w = total;
    return rc;
}


DWORD Ext2PipeControl(PPIPE_REQ *pr, ULONG *len)
{
    PPIPE_REQ       p;
    PREQ_QUERY_DRV  q;
    PIPE_REQ        ac;
    DWORD           le;
    DWORD           bytes = 0;
    DWORD           rc = 0;
    int             tries = 0;

    p = *pr;
    q = (PREQ_QUERY_DRV)p->data;

Reopen_try:

    /* just try to connect pipe anyway */
    if (NULL == Ext2OpenPipe(EXT2_MGR_SRV)) {
        printf("failed to connect to server.\n");
        if (Ext2StartExt2Srv()) {
			goto Reopen_try;
        }
        goto errorout;
    }
    
    // send a message to srv
    rc = Ext2WritePipe(g_hPipe, p, p->len, &bytes);
    if (!rc || bytes != p->len) {
        le = GetLastError();
		if (le == ERROR_NO_DATA && tries++ < 10) {
			Ext2ClosePipe();
			Sleep(500 * tries);
			goto Reopen_try;
		}
        goto errorout;
    }

    bytes = 0;
    memset(&ac, 0, sizeof(ac));
    rc = Ext2ReadPipe(g_hPipe, &ac, sizeof(ac), &bytes);
    if (!rc || bytes != sizeof(ac)) {
        le = GetLastError();
        goto errorout;
    }
    if (*len < ac.len) {
        if (p) {
            delete [] p;
        }
        *len = ac.len;
        p = (PIPE_REQ *) new CHAR[*len];
        *pr = p;
        if (!p) {
            goto errorout;
        }
    }

    memset(p, 0, *len);
    memcpy(p, &ac, sizeof(ac));
    rc = Ext2ReadPipe(g_hPipe, &p->data[0],
                      ac.len - bytes, &bytes);
    if (!rc || bytes != (ac.len - sizeof(ac))) {
        le = GetLastError();
        goto errorout;
    }

errorout:

    return rc;
}


BOOL Ext2DefineDosDevicePipe(DWORD flags,  CHAR *dos,  CHAR *symlink)
{
    PPIPE_REQ       p = NULL;
    ULONG           len = REQ_BODY_SIZE;
    BOOL            rc = 0;
 
    p = (PPIPE_REQ) new CHAR[len];
    if (NULL == p)
        goto errorout;

    if (flags & DDD_REMOVE_DEFINITION) {

        /* do dismount */
        PREQ_REMOVE_DRV  q;
        memset(p, 0, len);
        p->magic = PIPE_REQ_MAGIC;
        p->len = sizeof(PIPE_REQ) + sizeof(REQ_REMOVE_DRV);
        p->cmd = CMD_REMOVE_DRV;
        q = (PREQ_REMOVE_DRV)&p->data[0];
        q->pid = GetCurrentProcessId();
        q->drive = (UCHAR)toupper(dos[0]);
        q->flags = flags;
        strcpy(&q->name[0], symlink);
        p->len += (int)strlen(symlink) + 1;

        rc = Ext2PipeControl(&p, &len);
        if (!rc) {
            printf("pipe communication failed.\n");
            goto errorout;
        }

        if (q->result) {
            printf("%C: removed.\n", q->drive);
            rc = TRUE;
        } else {
            printf("failed to remove %C:.\n", q->drive);
            goto errorout;
        }

    } else {

        /* do mount */
        PREQ_DEFINE_DRV q;
        memset(p, 0, len);
        p->magic = PIPE_REQ_MAGIC;
        p->len = sizeof(PIPE_REQ) + sizeof(REQ_DEFINE_DRV);
        p->cmd = CMD_DEFINE_DRV;
        q = (PREQ_DEFINE_DRV)&p->data[0];
        q->pid = GetCurrentProcessId();
        q->drive = (UCHAR)toupper(dos[0]);
        q->flags = flags;
        strcpy(&q->name[0], symlink);
        p->len += (int)strlen(symlink) + 1;

        rc = Ext2PipeControl(&p, &len);
        if ( !rc) {
            printf("pipe communication failed.\n");
            goto errorout;
        }

        if (q->result) {
            rc = TRUE;
            printf("%C: assigned to %s\n", q->drive, &q->name[0]);
        } else {
            printf("failed to assign %C: to %s.\n", q->drive, &q->name[0]);
            goto errorout;
        }
    }

errorout:

    if (p)
        delete []p;

    return rc;
}

BOOL Ext2DefineDosDeviceLocal(DWORD flags,  CHAR *dos,  CHAR *symlink)
{
    CHAR dosPath[] = "A:\0";
    BOOL rc;

    dosPath[0] = (CHAR)toupper(dos[0]);
    rc = DefineDosDevice(flags, dosPath, symlink);
    if (rc) {
        if (flags & DDD_REMOVE_DEFINITION) {
            Ext2DrvNotify(dosPath[0], FALSE);
        } else {
            Ext2DrvNotify(dosPath[0], TRUE);
        }
    }

    return rc;
}

BOOL Ext2DefineDosDevice(DWORD flags,  CHAR *dos,  CHAR *symlink)
{
    if (CanDoLocalMount()) {
        return Ext2DefineDosDeviceLocal(flags, dos, symlink);
    }

    return Ext2DefineDosDevicePipe(flags, dos, symlink);
}


DWORD Ext2QueryDrivePipe(CHAR drive, CHAR *symlink)
{
    PPIPE_REQ       p = NULL;
    PREQ_QUERY_DRV  q;
    ULONG           len = REQ_BODY_SIZE;
    DWORD           rc = 0;
 
    p = (PPIPE_REQ) new CHAR[len];
    if (NULL == p)
        goto errorout;

     /* do query */
    memset(p, 0, len);
    p->magic = PIPE_REQ_MAGIC;
    p->len = sizeof(PIPE_REQ) + sizeof(REQ_QUERY_DRV);
    p->cmd = CMD_QUERY_DRV;
    q = (PREQ_QUERY_DRV)&p->data[0];
    q->drive = (UCHAR)toupper(drive);
    rc = Ext2PipeControl(&p, &len);
    if ( !rc) {
        printf("pipe communication failed.\n");
        goto errorout;
    }

    rc = q->type;
    if (q->result) {
        printf("%C: -> %s\n", q->drive, &q->name[0]);
        strcpy(symlink, &q->name[0]);
    } else {
        printf("failed to queyr %C:\n", q->drive);
        goto errorout;
    }

errorout:

    if (p)
        delete []p;

    return rc;
}

DWORD Ext2QueryDriveLocal(CHAR drive, CHAR *symlink)
{
    DWORD           type;
    CHAR            devPath[] = "A:";

    devPath[0] = drive;
    type = GetDriveType(devPath);
    if (type != DRIVE_NO_ROOT_DIR) {
        QueryDosDevice(devPath, symlink, MAX_PATH);
    }

    return type;
}

DWORD Ext2QueryDrive(CHAR drive, CHAR *symlink)
{
    if (CanDoLocalMount()) {
        return Ext2QueryDriveLocal(drive, symlink);
    }

    return Ext2QueryDrivePipe(drive, symlink);
}

int Ext2CreateToken(DWORD pid, DWORD *sid, HANDLE *token)
{
    HANDLE  h = NULL, token_user = NULL;
    DWORD dwTokenRights;
    PSID IntegrityLevelSid = NULL;
    CString SIDvalue;
    TOKEN_MANDATORY_LABEL IntegrityLevelToken = {0};

    int     rc = -1;

    rc = ProcessIdToSessionId(pid, sid);
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    rc = OpenProcessToken(h, TOKEN_DUPLICATE, &token_user);
    if(!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    dwTokenRights = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY |
                    TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT |
                    TOKEN_ADJUST_SESSIONID;
    rc = DuplicateTokenEx(token_user, dwTokenRights, NULL,
                          SecurityImpersonation /*SecurityIdentification*/,
                          TokenPrimary, token);
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    //  S-revision-authority-rid
    //  S-1-16-?
    //  16 represents
    //     SECURITY_MANDATORY_LABEL_AUTHORITY

    SIDvalue.Format("S-1-16-%d", SECURITY_MANDATORY_MEDIUM_RID);
    if(!ConvertStringSidToSidA(SIDvalue, &IntegrityLevelSid)) {
        /* not critical, ignore this error */
        goto errorout;
    }

    IntegrityLevelToken.Label.Attributes = SE_GROUP_INTEGRITY;
    IntegrityLevelToken.Label.Sid = IntegrityLevelSid;

    SetTokenInformation(*token,
                        TokenIntegrityLevel,
                        &IntegrityLevelToken,
                        sizeof(TOKEN_MANDATORY_LABEL) +
                        GetLengthSid(IntegrityLevelSid));
    LocalFree(IntegrityLevelSid);

errorout:

    if (token_user && token_user != INVALID_HANDLE_VALUE)
        CloseHandle(token_user);

    if (h && h != INVALID_HANDLE_VALUE)
        CloseHandle(h);

    return rc;
}


DWORD Ext2QuerySrv(TCHAR *srv, DWORD *pids, DWORD as)
{
    DWORD  total = 0;
    HANDLE p = NULL;
    PROCESSENTRY32 r = {0};

    p = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == p)
        return 0;

    r.dwSize=sizeof(PROCESSENTRY32);
    if (!Process32First(p, &r)) {
        goto errorout;
    }

    do {
        TCHAR *n = _tcsrchr(&r.szExeFile[0], _T('\\'));
        if (!n)
            n = &r.szExeFile[0];
        if (_tcsicmp(n, srv) == 0) {
            pids[total++] = r.th32ProcessID;
            if (total >= as)
                break;
        }
        
    } while(Process32Next(p, &r));

errorout:

    CloseHandle(p);

    return total;
}

TCHAR *Ext2StrLastA(TCHAR *t, TCHAR *s)
{
    int lt = (int)strlen(t), ls = (int)strlen(s), i;

    for (i = lt - ls; i >= 0; i--) {
        if (0 == _strnicmp(&t[i], s, ls))
            return &t[i];
    }

    return NULL;
}

TCHAR * Ext2BuildCMDA()
{
    TCHAR  cmd[512] = {0}, *p,  *refresh = NULL;
    int    len = 0;

    if (GetModuleFileName(NULL, cmd, 510)) {
    } else { 
        strcpy(cmd, GetCommandLine());
    }

    len = (int)_tcslen(cmd) + 2;
    refresh = new TCHAR[len];
    if (!refresh)
        goto errorout;
    memset(refresh, 0, sizeof(TCHAR)*len);
    _tcscpy(refresh, cmd);
    p = _tcsstr(refresh, _T("/"));
    if (p)
        *p = 0;

    p = Ext2StrLastA(refresh, "Ext2Mgr");
    if (p) {
        p[4] = 'S';
        p[5] = 'r';
        p[6] = 'v';
    } else {
        delete [] refresh;
        refresh = NULL;
    }

errorout:
    return refresh;
}

WCHAR *Ext2StrLastW(WCHAR *t, WCHAR *s)
{
    int lt = (int)wcslen(t), ls = (int)wcslen(s), i;

    for (i = lt - ls; i >= 0; i--) {
        if (0 == _wcsnicmp(&t[i], s, ls))
            return &t[i];
    }

    return NULL;
}

WCHAR *Ext2BuildCMDW()
{
    WCHAR  cmd[512] = {0}, *p, *refresh = NULL;
    int    len = 0;

    if (GetModuleFileNameW(NULL, cmd, 510)) {
    } else { 
        wcscpy(cmd, GetCommandLineW());
    }

    len = (int)wcslen(cmd) + 2;
    refresh = new WCHAR[len];
    if (!refresh)
        goto errorout;
    memset(refresh, 0, sizeof(WCHAR)*len);
    wcscpy(refresh, cmd);
    p = wcsstr(refresh, L"/");
    if (p)
        *p = 0;

    p = Ext2StrLastW(refresh, L"Ext2Mgr");
    if (p) {
        p[4] = 'S';
        p[5] = 'r';
        p[6] = 'v';
    } else {
        delete [] refresh;
        refresh = NULL;
    }

errorout:
    return refresh;
}

PROCESS_INFORMATION g_Ext2Srv;


typedef BOOL
(WINAPI *lpfnCreateProcessWithTokenW)(
    HANDLE hToken,
    DWORD dwLogonFlags,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

lpfnCreateProcessWithTokenW g_CreateProcessWithTokenW;

int Ext2StartSrvAsUser(HANDLE token)
{
    LPWSTR  cmd = NULL;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    int     rc = -1;

    cmd = Ext2BuildCMDW();
    if (!cmd) {
        rc = -1;
        goto errorout;
    }

    si.cb = sizeof( STARTUPINFO );
    if (token && g_CreateProcessWithTokenW) {
        rc = g_CreateProcessWithTokenW(
                             token, 0, NULL, cmd,
                             NORMAL_PRIORITY_CLASS |
                             CREATE_NO_WINDOW, 
                             NULL, NULL,
                             &si, &pi );
    } else {
        rc = CreateProcessW(  NULL, cmd, NULL, NULL,
                             FALSE, NORMAL_PRIORITY_CLASS |
                             CREATE_NO_WINDOW, NULL, NULL,
                             &si, &pi );
    }

    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    g_Ext2Srv = pi;

errorout:

    if (cmd)
        delete []cmd;

    return rc;
}

int Ext2StartSrvAsElevated()
{
    LPTSTR  cmd = NULL;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    int     rc = -1;

    cmd = Ext2BuildCMDA();
    if (!cmd) {
        rc = -1;
        goto errorout;
    }

    si.cb = sizeof( STARTUPINFO );
    rc = CreateProcess(  NULL, cmd, NULL, NULL,
                         FALSE, NORMAL_PRIORITY_CLASS |
                         CREATE_NO_WINDOW, NULL, NULL,
                         &si, &pi );

    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    g_Ext2Srv = pi;

errorout:

    if (cmd)
        delete []cmd;

    return rc;
}

int Ext2StartSrv()
{
    DWORD   pid[12] = {0}, num;
    int     rc = -1;

    num = Ext2QuerySrv(_T("Ext2Srv.exe"), pid, 10);
    if (num) {
        /* already started */
        rc = num;
        goto errorout;
    }

    if (IsVistaOrAbove()) {

        DWORD   sid = 0, mysid = 0;
        HANDLE  token = 0;

        num = Ext2QuerySrv(_T("Explorer.exe"), &pid[0], 10);
        if (!num) {
            goto errorout;
        }

        pid[num] = GetCurrentProcessId();
        num++;

        rc = ProcessIdToSessionId(GetCurrentProcessId(), &mysid);
        if (!rc) {
            rc = -1 * GetLastError();
            goto errorout;
        }

        while (num > 0 && pid[num - 1]) {

            rc = Ext2CreateToken(pid[--num], &sid, &token);
            if (rc > 0) {
                if (sid == mysid)  {
                    rc = Ext2StartSrvAsUser(token);
                } else {
                    rc = 0;
                }
                if (token && token != INVALID_HANDLE_VALUE) {
                    CloseHandle(token);
                    token = NULL;
                }
                if (rc == 1) {
                    break;
                }
            }
        }

    } else {

        Ext2StartSrvAsElevated();
    }

errorout:

    return rc;
}

void Ext2StopSrv()
{
    PROCESS_INFORMATION pi = g_Ext2Srv;

    memset(&g_Ext2Srv, 0, sizeof(pi));

    /* wait until process exits or timeouts */

    if (pi.hProcess != INVALID_HANDLE_VALUE) { 
        TerminateProcess(pi.hProcess, -1);
        CloseHandle(pi.hProcess); 
    } 
    if (pi.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(pi.hThread); 
    }
}

/* Ext2EnablePrivilege(SE_TCB_NAME) */

BOOL Ext2EnablePrivilege(LPCTSTR lpszPrivilegeName)
{
    TOKEN_PRIVILEGES tp = {0};
    HANDLE  token;
    LUID    luid;
    BOOL    rc;

    rc = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES |
                          TOKEN_QUERY | TOKEN_READ, &token);
    if(!rc)
        goto errorout;

    rc = LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid);
    if(!rc)
        goto errorout;

    /* initialize token privilege */
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    rc = AdjustTokenPrivileges(token, FALSE, &tp, NULL, NULL, NULL);
    CloseHandle(token);

errorout:

    return rc;
}

#define KEY_WOW64_64KEY         (0x0100)

BOOL KdpQueryRegDWORD(CHAR *service, CHAR *value, DWORD *data)
{
    int     rc = TRUE;
    HKEY    key;
    CHAR   keyPath[MAX_PATH] = "SYSTEM\\CurrentControlSet\\Services\\";
    LONG    status, type, len = 0;

    strcat(keyPath, service);
    status = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                            &key) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    len = sizeof(DWORD);
    status = RegQueryValueEx( key,
                              value,
                              0,
                              (LPDWORD)&type,
                              (BYTE *)data,
                              (LPDWORD)&len);
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
    }
    RegCloseKey(key);

errorout:

    return rc;
}

BOOL KdpSetRegDWORD(CHAR *service, CHAR *value, DWORD data)
{
    int     rc = TRUE;
    HKEY    key;
    CHAR   keyPath[MAX_PATH] = "SYSTEM\\CurrentControlSet\\Services\\";
    LONG    status;

    strcat(keyPath, service);
    status = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            keyPath,
                            0,
                            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                            &key) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    status = RegSetValueEx( key,
                              value,
                              0,
                              REG_DWORD,
                              (BYTE *)&data,
                              sizeof(DWORD));
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
    }
    RegCloseKey(key);

errorout:

    return rc;
}

BOOL Ext2StartPipeSrv()
{
    HMODULE hAdvapi32;
    BOOL rc = FALSE;

    if (CanDoLocalMount()) {
        return TRUE;
    }

    /* query advapi32!CreateProcessWithToken */
    hAdvapi32 = GetModuleHandle("Advapi32.DLL");
    g_CreateProcessWithTokenW = (lpfnCreateProcessWithTokenW)
        GetProcAddress(hAdvapi32, "CreateProcessWithTokenW");

    if (!g_CreateProcessWithTokenW) {
        rc = FALSE;
        goto errorout;
    }

    /* add more privilege for toke duplication */
    Ext2EnablePrivilege(SE_INCREASE_QUOTA_NAME);
    Ext2EnablePrivilege(SE_TCB_NAME);

    /* start Ext2Srv.exe */
    rc = Ext2StartSrv();

errorout:

    return rc;
}

VOID Ext2StopPipeSrv()
{
    /* close pipe handles */
    Ext2ClosePipe();

    /* terminate process: Ext2Srv */
    Ext2StopSrv();
}