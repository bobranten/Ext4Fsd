#include <Ext2Srv.h>

SERVICE_STATUS          ServiceStatus;
SERVICE_STATUS_HANDLE   ServiceHandle;
HDEVNOTIFY              ServiceNotify;


BOOL
Ext2ReportStatus(
        DWORD           State,
		DWORD           Exitcode,
		DWORD           Timeout
    )
{
	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
    ServiceStatus.dwControlsAccepted =      SERVICE_ACCEPT_STOP;
    if (State == SERVICE_RUNNING) {
		ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN |
                                            SERVICE_ACCEPT_POWEREVENT |
                                            SERVICE_ACCEPT_SESSIONCHANGE ;
    }

	// Save the new status we've been given
	ServiceStatus.dwCurrentState = State;
	ServiceStatus.dwWin32ExitCode = Exitcode;
	ServiceStatus.dwWaitHint = Timeout;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if ((State == SERVICE_RUNNING) || (State == SERVICE_STOPPED)) {
		ServiceStatus.dwCheckPoint = 0;
	} else {
        ServiceStatus.dwCheckPoint++;
    }

	// Tell the SCM our new status
	return SetServiceStatus(ServiceHandle, &ServiceStatus);
}

void Ext2StopService()
{
    Ext2ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    if (ServiceNotify) {
        UnregisterDeviceNotification(ServiceNotify);
        ServiceNotify = NULL;
    }

    /* issue stop event to main thread */
    Ext2StopPipeSrv();

    Ext2ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

VOID Ext2DrivesChangeNotify(BOOLEAN bArrival)
{
    return;
}

VOID Ext2CleanupSession(DWORD sid)
{
}

DWORD WINAPI
Ext2CtrlService(
    DWORD ctrlcode, DWORD dwEventType,
    LPVOID lpEventData, LPVOID lpContext
    )
{
    switch(ctrlcode)
    {

	case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
		// STOP : The service must stop
        Ext2StopService();
        break;

    case SERVICE_CONTROL_INTERROGATE:
		// QUERY : Service control manager just wants to know our state
		break;

    case SERVICE_CONTROL_DEVICEEVENT:
    {
        PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpEventData;
        PDEV_BROADCAST_DEVICEINTERFACE lpdbi = (PDEV_BROADCAST_DEVICEINTERFACE) lpEventData;

        if (lpdbv->dbcv_devicetype != DBT_DEVTYP_VOLUME &&
            lpdbi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE
            ) {
            break;
        }

        if (dwEventType == DBT_DEVICEREMOVECOMPLETE) {
            Ext2DrivesChangeNotify(FALSE);
        } else if (dwEventType == DBT_DEVICEARRIVAL) {
            Ext2DrivesChangeNotify(TRUE);
        }

        break;
    }

    case SERVICE_CONTROL_POWEREVENT:

    switch (dwEventType) {

        case PBT_APMQUERYSUSPEND:
            MsgLog("Power event: PBT_APMQUERYSUSPEND.\n");
            //Ext2StartFlushThread();
            // Ext2FlushVolume(ASSDDISK_VOLUME);
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            MsgLog("Power event: APMQUERYSUSPENDFAILED.\n");
            break;

        case PBT_APMSUSPEND:
            MsgLog("Power event: PBT_APMSUSPEND.\n");
            break;

        case PBT_APMRESUMESUSPEND:
            MsgLog("Power event: PBT_APMRESUMESUSPEND.\n");
            Ext2DrivesChangeNotify(TRUE);
            break;

        case PBT_APMQUERYSTANDBY:
            MsgLog("Power event: PBT_APMQUERYSTANDBY.\n");
            break;

        case PBT_APMQUERYSTANDBYFAILED:
            MsgLog("Power event: APMQUERYSTANDBYFAILED.\n");
            break;

        case PBT_APMSTANDBY:
            MsgLog("Power event: PBT_APMSTANDBY.\n");
            break;

        case PBT_APMRESUMESTANDBY:
            MsgLog("Power event: PBT_APMRESUMESTANDBY.\n");
            break;

        case PBT_APMRESUMECRITICAL:
            MsgLog("Power event: PBT_APMRESUMECRITICAL.\n");
            break;

        case PBT_APMPOWERSTATUSCHANGE:
            MsgLog("Power event: PBT_APMPOWERSTATUSCHANGE\n");
            break;

        case PBT_APMRESUMEAUTOMATIC:
            MsgLog("Power event: PBT_APMRESUMEAUTOMATIC\n");
            break;

        default:
            MsgLog("Power event: unknown value %u\n", dwEventType);
            break;            
    }
    break;


    case SERVICE_CONTROL_SESSIONCHANGE:

    switch (dwEventType) {

        case WTS_CONSOLE_CONNECT:
            MsgLog("Session event: Console connected.\n");
            break;

        case WTS_CONSOLE_DISCONNECT:
            MsgLog("Session event: Console disconnect.\n");
            break;

        case WTS_REMOTE_CONNECT:
            MsgLog("Session event: Remote connected.\n");
            break;

        case WTS_REMOTE_DISCONNECT:
            MsgLog("Session event: Remote disconnect.\n");
            break;

        case WTS_SESSION_LOCK:
            MsgLog("Session event: Session locked.\n");
            break;
        case WTS_SESSION_UNLOCK:
            MsgLog("Session event: Session unlocked.\n");
            break;

        case WTS_SESSION_LOGON:
        {
            PWTSSESSION_NOTIFICATION pwn = (PWTSSESSION_NOTIFICATION)lpEventData;
            MsgLog("Session event: Session logon.\n");
            Ext2StartUserTask(NULL, _T("/startmgr"), pwn->dwSessionId, FALSE);
            break;
        }

        case WTS_SESSION_LOGOFF:
        {
            PWTSSESSION_NOTIFICATION pwn = (PWTSSESSION_NOTIFICATION)lpEventData;
            MsgLog("Session event: Session logoff.\n");
            if (pwn && pwn->cbSize >= sizeof(WTSSESSION_NOTIFICATION)) {
                Ext2CleanupSession(pwn->dwSessionId);
            }
            break;
        }

        default:
            MsgLog("Session event: unknown value: %u.\n", dwEventType);
            break;
    }
    break;

    case SERVICE_ACCEPT_HARDWAREPROFILECHANGE:

	default:
		// Control code not recognised
		break;

    }

    // Tell the control manager what we're up to.
    Ext2ReportStatus(ServiceStatus.dwCurrentState, NO_ERROR, 0);

    return NO_ERROR;
}

VOID
Ext2StartMain(VOID * arg)
{
    Ext2StartPipeSrv();
    Ext2ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
}


#define EXT2_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
EXT2_GUID(GUID_DEVINTERFACE_DISK, 0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, \
          0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

VOID
Ext2RegisterDeviceInterface(SERVICE_STATUS_HANDLE handle,
                            GUID guid, HDEVNOTIFY *notify)
{
    DEV_BROADCAST_DEVICEINTERFACE bd;

    ZeroMemory (&bd, sizeof(bd));
    bd.dbcc_size = sizeof(bd);
    bd.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    bd.dbcc_classguid = guid; 
    *notify = RegisterDeviceNotification(handle, &bd,
                       DEVICE_NOTIFY_SERVICE_HANDLE);
}


void WINAPI
Ext2ServiceEntry(DWORD argc, char**argv)
{
	// register the service control handler
    ServiceHandle = RegisterServiceCtrlHandlerEx(_T("Ext2Srv"), Ext2CtrlService, NULL);
    if (ServiceHandle == 0) {
		return;
    }

    // setup standard service state values
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwServiceSpecificExitCode = 0;

	// report our status to the SCM
    Ext2ReportStatus(SERVICE_START_PENDING, NO_ERROR, 600);

    Ext2RegisterDeviceInterface(ServiceHandle, GUID_DEVINTERFACE_DISK,
                                &ServiceNotify);

	// Now start the service for real
    Ext2StartMain(NULL);
    return;
}


VOID
Ext2StartService(VOID *arg)
{
    SERVICE_TABLE_ENTRY Ext2SeriveTable[] =
    {
      {_T("Ext2Srv"), (LPSERVICE_MAIN_FUNCTION)Ext2ServiceEntry},
      {NULL, NULL}
    };

    // let service control dispatcher start Ext2Mgr
    StartServiceCtrlDispatcher(Ext2SeriveTable);
}

#define SERVICE_CMD_LENGTH MAX_PATH

int
Ext2SetupService(BOOL bInstall)
{
    TCHAR Target[SERVICE_CMD_LENGTH];
    SC_HANDLE   hService;
    SC_HANDLE   hManager;

    // get the filename of this executable
    if (GetModuleFileName(NULL, Target, SERVICE_CMD_LENGTH - 20) == 0) {
        MessageBox(NULL, _T("Ext2Srv: Unable to install as service"), NULL,
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // open Service Control Manager
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL) {
        MessageBox(NULL, _T("Ext2Srv: cannot open Service Control Manager"),
                   NULL, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (bInstall) {

        // now create service entry for Ext2Mgr
        hService = CreateService(
                hManager,                   // SCManager database
                _T("Ext2Srv"),                 // name of service
                _T("Ext2Fsd Service Manager"), // name to display
                SERVICE_ALL_ACCESS,	        // desired access
                SERVICE_WIN32_OWN_PROCESS | // service type
                SERVICE_INTERACTIVE_PROCESS,
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
                MessageBox(NULL, _T("Ext2Srv is already registered."), NULL,
                           MB_OK | MB_ICONERROR);
            } else {
                MessageBox(NULL, _T("Ext2Srv couldn't be registered."), NULL,
                           MB_OK | MB_ICONERROR);
            }
        } else {

            CloseServiceHandle(hService);

			// got Ext2Mgr installed as a service
            MessageBox(NULL, _T("Ext2Srv service was successfully registered. \n\n"
				"You can modify the default settings and start/stop it from Control Panel.\n"
				"The service will automatically run the next time when system is restarted.\n"),
                NULL, MB_OK | MB_ICONINFORMATION);
        }

    } else {

        /* open the service of Pipe Event Engine */
        hService = OpenService(hManager, _T("Ext2Srv"), SERVICE_ALL_ACCESS);

        if (hService != NULL) {

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
                    MessageBox(NULL, _T("Ext2Srv: service couldn't be stopped !"),
                               NULL, MB_OK | MB_ICONERROR);
                }
            }

            // remove the service from the SCM
            if (DeleteService(hService)) {
                MessageBox(NULL, _T("Ext2Srv: service has been unregistered."),
                           NULL, MB_OK | MB_ICONINFORMATION);
            } else {
                DWORD error = GetLastError();
                if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
                    MessageBox(NULL, _T("Ext2Srv: service is already unregistered"),
                               NULL, MB_OK | MB_ICONEXCLAMATION);
                } else {
                    MessageBox(NULL, _T("Ext2Srv: service could not be unregistered"),
                               NULL, MB_OK | MB_ICONERROR);
                }
            }

            CloseServiceHandle(hService);
        }
    }

    CloseServiceHandle(hManager);

	return TRUE;
}


/*
 * parameter process engine
 */

int     opterr = 1, /* if error message should be printed */
        optind = 1, /* index into parent argv vector */
        optopt,     /* character checked for validity */
        optreset;   /* reset getopt */
char   *optarg;     /* argument associated with option */

#define    BADCH    (int)'?'
#define    BADARG    (int)':'
#define    EMSG    ""

/*
 * getopt --
 *    Parse argc/argv argument vector.
 */
int getopt(int argc, char * const argv[], const char *optstring)
{
    static char *place = EMSG;        /* option letter processing */
    char *oli;                /* option letter list index */

    if (optreset || *place == 0) {        /* update scanning pointer */
        optreset = 0;
        place = argv[optind];
        if (optind >= argc || *place++ != '-') {
            /* Argument is absent or is not an option */
            place = EMSG;
            return (-1);
        }
        optopt = *place++;
        if (optopt == '-' && *place == 0) {
            /* "--" => end of options */
            ++optind;
            place = EMSG;
            return (-1);
        }
        if (optopt == 0) {
            /* Solitary '-', treat as a '-' option
               if the program (eg su) is looking for it. */
            place = EMSG;
            if (strchr(optstring, '-') == NULL)
                return -1;
            optopt = '-';
        }
    } else
        optopt = *place++;

    /* See if option letter is one the caller wanted */
    if (optopt == ':' || (oli = strchr((char *)optstring, optopt)) == NULL) {
        if (*place == 0)
            ++optind;
        if (opterr && *optstring != ':')
            (void)fprintf(stderr, "unknown option -- %c\n", optopt);
        return (BADCH);
    }

    /* Does this option need an argument? */
    if (oli[1] != ':') {
        /* don't need argument */
        optarg = NULL;
        if (*place == 0)
            ++optind;
    } else {
        /* Option-argument is either the rest of this argument or the
           entire next argument. */
        if (*place)
            optarg = place;
        else if (argc > ++optind)
            optarg = argv[optind];
        else {
            /* option-argument absent */
            place = EMSG;
            if (*optstring == ':')
                return (BADARG);
            if (opterr)
                (void)fprintf(stderr,"option requires an argument - %c\n", optopt);
            return (BADCH);
        }
        place = EMSG;
        ++optind;
    }
    return (optopt);            /* return option letter */
}


void Ext2Log(DWORD ll, char *fn, int ln, char *format, ... )
{
    va_list             ap = NULL;
    SYSTEMTIME          st = {0};
    DWORD               w = 0;
    int                 i = 0;
    CHAR                s[260] = {0};

    if (ll == EXT2_LOG_DUMP) {
        /* do nothing for MsgDump */
    } else {
        GetLocalTime(&st);
        sprintf_s(s, 256, "%2.2u:%2.2u:%2.2u (%x): ", st.wHour,
                  st.wMinute, st.wSecond, GetCurrentThreadId());
        i = (int)strlen(s);
    }

    /* write user message to buffer s */
    va_start(ap, format);
    _vsnprintf(&s[i], 259 - i, format, ap);
    va_end(ap);

    printf("%s", s);
}


INT __cdecl _tmain(INT argc, TCHAR *argv[])
{
    DWORD   rc = 0;
    BOOL    console = FALSE;

    if (argc >= 2) {

        if (0 == _tcsicmp(argv[1], _T("/installasservice"))) {
            return Ext2SetupService(TRUE);
        }

        if (0 == _tcsicmp(argv[1], _T("/removeservice"))) {
            return Ext2SetupService(FALSE);
        }

        if (0 == _tcsicmp(argv[1], _T("/startmgr"))) {
            return Ext2StartMgrAsUser();
        }

        if (argc >= 4) {

            if (0 == _tcsicmp(argv[1], _T("/mount"))) {
                Ext2AssignDrvLetter(argv[3], argv[2][0]);
                return 0;
            }

        } else if (argc >= 3) {

            if (0 == _tcsicmp(argv[1], _T("/umount"))) {
                Ext2RemoveDrvLetter(argv[2][0]);
                return 0;
            }

            if (0 == _tcsicmp(argv[1], _T("/add"))) {
                Ext2DrvNotify(argv[2][0], TRUE);
                Sleep(1000);
                Ext2DrvNotify(argv[2][0], TRUE);
                return 0;
            }

            if (0 == _tcsicmp(argv[1], _T("/del"))) {
                Ext2DrvNotify(argv[2][0], FALSE);
                Sleep(1000);
                Ext2DrvNotify(argv[2][0], FALSE);
                return 0;
            }
        }
    }

    /* enable SE_TCB_NAME privilege */
    Ext2EnablePrivilege(SE_TCB_NAME);


    /* service mode */
    Ext2StartService(NULL);
 
    return 0;
}