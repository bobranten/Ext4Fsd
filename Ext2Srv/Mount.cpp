#include <Ext2Srv.h>
#include <tlhelp32.h>
#include <Sddl.h>


BOOL Ext2SetPrivilege(HANDLE token, LPCTSTR lpszPrivilegeName)
{
    TOKEN_PRIVILEGES tp = {0};
    LUID             luid;
    DWORD            le;
    BOOL             rc;

    rc = LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid);
    if(!rc)
        goto errorout;

    /* initialize token privilege */
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    rc = AdjustTokenPrivileges(token, FALSE, &tp, NULL, NULL, NULL);
    if (!rc)
        le = GetLastError();

errorout:

    return rc;
}


BOOL Ext2EnablePrivilege(LPCTSTR lpszPrivilegeName)
{
    HANDLE           token;
    BOOL             rc;

    rc = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES |
                          TOKEN_QUERY | TOKEN_READ, &token);
    if(!rc)
        goto errorout;

    rc = Ext2SetPrivilege(token, lpszPrivilegeName);
    CloseHandle(token);

errorout:

    return rc;
}

VOID
Ext2DrvNotify(TCHAR drive, int add)
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
                           &target, WM_DEVICECHANGE, add ?
                           DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE,
                           (LPARAM)(DEV_BROADCAST_HDR *)&dbv );
}


DWORD Ext2QueryMgr(TCHAR *Auth, DWORD *pids, DWORD as)
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
        if (_tcsicmp(n, Auth) == 0) {
            pids[total++] = r.th32ProcessID;
            if (total >= as)
                break;
        }
        
    } while(Process32Next(p, &r));

errorout:

    CloseHandle(p);

    return total;
}

TCHAR * Ext2BuildSrvCMD(TCHAR *task)
{
    TCHAR  cmd[258]= {0}, *p, *refresh = NULL;
    int    len = 0;

    if (GetModuleFileName(NULL, cmd, 256)) {
    } else { 
        _tcscpy(cmd, GetCommandLine());
        p = _tcsstr(cmd, _T("/"));
        if (p)
            *p = 0;
    }

    len = (int)_tcslen(cmd) + 40;
    refresh = new TCHAR[len];
    if (!refresh)
        goto errorout;
    memset(refresh, 0, sizeof(TCHAR)*len);
    _tcscpy_s(refresh, len - 1, cmd);
    _tcscat_s(refresh, len, _T(" "));
    _tcscat_s(refresh, len, task);

errorout:
    return refresh;
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

TCHAR * Ext2BuildUsrCMD(TCHAR *task)
{
    TCHAR  cmd[258]= {0}, *p, *refresh = NULL;
    int    len = 0;

    if (GetModuleFileName(NULL, cmd, 256)) {
    } else {
        _tcscpy(cmd, GetCommandLine());
    }

    p = Ext2StrStr(cmd, _T("Ext2Srv"));
    if (p)
        *p = 0;

    len = (int)_tcslen(cmd) + 10 + (int)_tcslen(task);
    refresh = new TCHAR[len];
    if (!refresh)
        goto errorout;
    memset(refresh, 0, sizeof(TCHAR)*len);
    _tcscpy_s(refresh, len - 1, cmd);
    _tcscat_s(refresh, len, task);

errorout:
    return refresh;
}

int Ext2AdjustTokenGroups(HANDLE token, LPCTSTR group)
{
    TOKEN_GROUPS tg = {0};
    PSID         sid = NULL;
    DWORD        le;
    BOOL         rc;


    rc = ConvertStringSidToSid(group, &sid);
    if(!rc)
        goto errorout;

    /* initialize token groups */
    tg.GroupCount = 1;
    tg.Groups[0].Sid = sid;
    tg.Groups[0].Attributes = SE_GROUP_OWNER;
    rc = AdjustTokenGroups(token, FALSE, &tg, sizeof(tg), NULL, NULL);
    if (!rc)
        le = GetLastError();

errorout:

    return rc;
}

int Ext2CreateUserToken(DWORD sid, HANDLE *token, BOOL bElevated)
{
    HANDLE  token_user = NULL;
    int     rc = -1;

    if (!token)
        goto errorout;

    rc = WTSQueryUserToken(sid, &token_user);
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    rc = DuplicateTokenEx(token_user, MAXIMUM_ALLOWED, NULL,
                          SecurityIdentification, TokenPrimary,
                          token);
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    if (bElevated) {
        Ext2SetPrivilege(token, SE_IMPERSONATE_NAME);
        Ext2SetPrivilege(token, SE_CHANGE_NOTIFY_NAME);
        Ext2SetPrivilege(token, SE_CREATE_GLOBAL_NAME);

        Ext2AdjustTokenGroups(token, _T("S-1-5-32-544"));
    }
errorout:

    if (token_user && token_user != INVALID_HANDLE_VALUE)
        CloseHandle(token_user);

    return rc;
}


int Ext2StartUserTask(TCHAR *usr, TCHAR *srv, DWORD sid, BOOL bElevated)
{
    LPTSTR  cmd = NULL;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    HANDLE  token = 0;
    int     rc = -1;

    if (usr)
        cmd = Ext2BuildUsrCMD(usr);
    else
        cmd = Ext2BuildSrvCMD(srv);
    if (!cmd) {
        rc = -1;
        goto errorout;
    }

    rc = Ext2CreateUserToken(sid, &token, bElevated);
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    si.cb = sizeof( STARTUPINFO );
    rc = CreateProcessAsUser(token, NULL, cmd, NULL, NULL,
                             FALSE, NORMAL_PRIORITY_CLASS |
                             CREATE_NO_WINDOW, NULL, NULL,
                             &si, &pi );
    if (!rc) {
        rc = -1 * GetLastError();
        goto errorout;
    }

    if (!GetExitCodeProcess(pi.hProcess, (LPDWORD)&rc)) {
        rc = -2;
    }

    if (pi.hProcess != INVALID_HANDLE_VALUE) { 
        CloseHandle(pi.hProcess); 
    } 
    if (pi.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(pi.hThread); 
    }

errorout:

    if (cmd)
        delete []cmd;

    if (token)
        CloseHandle(token);

    return rc;
}

INT Ext2NotifyUser(TCHAR *task, ULONG mgr)
{
    DWORD   pid[64] = {0}, n, i, sid = 0;
    INT     rc = -1;

    n = Ext2QueryMgr(_T("Ext2Mgr.exe"), pid, 63);
    if (mgr)
        pid[n++] = mgr;

    for (i = 0; i < n && pid[i]; i++) {

        rc = ProcessIdToSessionId(pid[i], &sid);
        if (!rc) {
            continue;
        }

        if (!mgr && mgr == pid[i]) {
            n--;
            rc = Ext2StartUserTask(0, task, sid, FALSE);
        } else {
            Ext2StartUserTask(0, task, sid, FALSE);
        }
    }

    return rc;
}


BOOL Ext2AssignDrvLetter(TCHAR *dev, TCHAR drv)
{
	TCHAR	dos[8];

	_stprintf_s(dos, 8, _T("%C:"), drv);
	if (!DefineDosDevice(DDD_RAW_TARGET_PATH, dos, dev)) {
        ErrLog("mount: failed to assigned drive letter %C:.\n", drv);
        return 0;
	}

    Ext2DrvNotify(drv, TRUE);

	return TRUE;
}

BOOL Ext2RemoveDrvLetter(TCHAR drive)
{
	TCHAR	dosDev[MAX_PATH];

    /* remove drive letter */
	_stprintf_s(dosDev, MAX_PATH, _T("%C:"), drive);
	DefineDosDevice(DDD_REMOVE_DEFINITION,
                    dosDev, NULL);
    Ext2DrvNotify(drive, FALSE);

	return TRUE;
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
                            KEY_READ | KEY_WOW64_64KEY,
                            &hKey) ;
    if (status != ERROR_SUCCESS) {
        rc = FALSE;
        goto errorout;
    }

    /* query autorun user list */
    len = MAX_PATH - 1;
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


BOOL Ext2RunMgrForCurrentUserVista()
{
    CHAR *userList = NULL, *user, e;
    CHAR  userName[MAX_PATH] = {0};
    DWORD userLen = MAX_PATH - 1;
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

INT Ext2StartMgrAsUser()
{
    SHELLEXECUTEINFO sei = {0};
    TCHAR   cmd[258] = {0}, *t;
    DWORD   rc = 0;

    if (!Ext2RunMgrForCurrentUserVista()) {
        return 0;
    }

    /* sleeping 1 second before proceeding */
    Sleep(1000);

    /* building task CMD path */
    rc = GetModuleFileName(NULL, cmd, 256);
    if (!rc) {
        return 0;
    }

    t = Ext2StrStr(cmd, "Ext2Srv.exe");
    if (!t) {
        return 0;
    }
    t[4] = _T('M');
    t[5] = _T('g');
    t[6] = _T('r');

    /* starting Ext2Mgr as elevated */
    sei.cbSize       = sizeof(SHELLEXECUTEINFO);
    sei.fMask        = SEE_MASK_FLAG_DDEWAIT |
	                   SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd         = NULL;
    sei.lpFile       = cmd;
    sei.lpParameters = _T("/quiet");
    sei.lpVerb       = _T("runas");
    sei.nShow        = SW_SHOW;

    return ShellExecuteEx(&sei);
}
