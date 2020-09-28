// Ext2Mgr.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Ext2Mgr.h"
#include "Ext2MgrDlg.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrApp

BEGIN_MESSAGE_MAP(CExt2MgrApp, CWinApp)
	//{{AFX_MSG_MAP(CExt2MgrApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrApp construction

CExt2MgrApp::CExt2MgrApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CExt2MgrApp object

CExt2MgrApp theApp;

/////////////////////////////////////////////////////////////////////////////
// globals

DWORD Checkpoint = 1;

SERVICE_STATUS          ServiceStatus;
SERVICE_STATUS_HANDLE   ServiceHandle;

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrApp initialization

#if 0
void WINAPI ManagerServiceEntry(DWORD argc, char **argv);
void ManagerServiceThread(void *arg);
void ManagerStopService();
void WINAPI ManagerServiceCtrl(DWORD ctrlcode);
#endif


BOOL
ManagerReportStatus(
        SERVICE_STATUS_HANDLE Handle,
        SERVICE_STATUS* Status,
        DWORD           State,
		DWORD           Exitcode,
		DWORD           Timeout
    )
{
	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
    if (State == SERVICE_START_PENDING) {
		Status->dwControlsAccepted = 0;
	} else {
		Status->dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

	// Save the new status we've been given
	Status->dwCurrentState = State;
	Status->dwWin32ExitCode = Exitcode;
	Status->dwWaitHint = Timeout;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if ((State == SERVICE_RUNNING) || (State == SERVICE_STOPPED)) {
		Status->dwCheckPoint = 0;
	} else {
        Status->dwCheckPoint = Checkpoint++;
    }

	// Tell the SCM our new status
	return SetServiceStatus(Handle, Status);
}

void ManagerStopService()
{
    ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    theApp.m_pMainWnd->SendMessage(WM_TERMINATE_PROGRAM, 0, 0x1234);
}

void WINAPI ManagerCtrlService(DWORD ctrlcode)
{
    switch(ctrlcode)
    {

	case SERVICE_CONTROL_STOP:
		// STOP : The service must stop
        ManagerStopService();
        break;

    case SERVICE_CONTROL_INTERROGATE:
		// QUERY : Service control manager just wants to know our state
		break;

	default:
		// Control code not recognised
		break;

    }

	// Tell the control manager what we're up to.
    ManagerReportStatus(
        ServiceHandle,
        &ServiceStatus,
        ServiceStatus.dwCurrentState,
        NO_ERROR, 0);
}

VOID __cdecl
ManagerStartMain(VOID * arg)
{
    BOOL isService = arg != 0;
    CExt2MgrDlg* dlg = (CExt2MgrDlg*)theApp.m_pMainWnd;

    if (dlg) {

        /* always be quiet ! */
        if (0 && !dlg->m_bQuiet) {
            CSplash* splash = new CSplash(IDB_ABOUT_SCREEN, RGB(128, 128, 128));
            splash->ShowSplash();
            dlg->m_splash = splash;
        }

        if (isService) {
            ManagerReportStatus(
                ServiceHandle,
                &ServiceStatus,
                SERVICE_RUNNING,
                NO_ERROR, 0   );
        }

        dlg->DoModal();

        Ext2StopPipeSrv();
    }

    if (isService) {
        ManagerReportStatus(
                ServiceHandle,
                &ServiceStatus,
                SERVICE_STOPPED,
                NO_ERROR, 0   );
    }
}

void NTAPI
ManagerServiceEntry(DWORD argc, char**argv)
{
	// register the service control handler
    ServiceHandle = 
            RegisterServiceCtrlHandler(
                "Ext2Mgr",
                ManagerCtrlService   );

    if (ServiceHandle == 0) {
		return;
    }

    // setup standard service state values
    ServiceStatus.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
    ServiceStatus.dwServiceSpecificExitCode = 0;

	// report our status to the SCM
    if (!ManagerReportStatus(
        ServiceHandle,
        &ServiceStatus,
        SERVICE_START_PENDING,
        NO_ERROR,
        600000
        ))
	{
        ManagerReportStatus(
            ServiceHandle,
            &ServiceStatus,
			SERVICE_STOPPED,
			NO_ERROR,
            0);
		return;
	}

	// Now start the service for real
    _beginthread(ManagerStartMain, 0, (PVOID)TRUE);
    return;
}


VOID
ManagerStartService(VOID *arg)
{
    SERVICE_TABLE_ENTRY ManagerSeriveTable[] =
    {
      {"Ext2Mgr", (LPSERVICE_MAIN_FUNCTION)ManagerServiceEntry},
      {NULL, NULL}
    };

    // let service control dispatcher start Ext2Mgr
    StartServiceCtrlDispatcher(ManagerSeriveTable);
}

BOOL CExt2MgrApp::InitInstance()
{
	AfxEnableControlContainer();

    HWND hWnd = ::FindWindow(NULL, "Ext2 Volume Manager");
    if (hWnd) {
        if (::GetWindowLongPtr(hWnd, DWLP_USER) == EXT2_DIALOG_MAGIC) {
            ::ShowWindow(hWnd, SW_SHOW);
            ::SetForegroundWindow(hWnd);
            return FALSE;
        }
    }

    BOOL bHide = FALSE;
    BOOL bQuiet = FALSE;
    BOOL bService = FALSE;
    BOOL bInstall = FALSE;
    BOOL bRemove = FALSE;
    BOOL bStat = FALSE;

    char * cmds = GetCommandLine();
    int i = (int)strlen(cmds);
    while (--i > 0) {
        if (cmds[i] == (char)' ') {
            cmds[i] = 0;
            if ( strlen(&cmds[i+1]) == 5 &&
                 _stricmp(&cmds[i+2], "hide") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bHide = TRUE;
            } else if (strlen(&cmds[i+1]) == 6 &&
                 _stricmp(&cmds[i+2], "quiet") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bHide = bQuiet = TRUE;
            } else if (strlen(&cmds[i+1]) == 8 &&
                 _stricmp(&cmds[i+2], "install") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bInstall = TRUE;
            } else if (strlen(&cmds[i+1]) == 7 &&
                 _stricmp(&cmds[i+2], "remove") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bRemove = TRUE;
            } else if  (strlen(&cmds[i+1]) == 8 &&
                 _stricmp(&cmds[i+2], "service") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bHide = bService = TRUE;
            } else if  (strlen(&cmds[i+1]) == 5 &&
                 _stricmp(&cmds[i+2], "stat") == 0 &&
                (cmds[i+1] == '/' || cmds[i+1] == '-')) {
                bHide = bStat = TRUE;
            }
        }
    }

    if (bInstall) {
        Ext2SetManagerAsService(TRUE);
        return FALSE;
    }

    if (bRemove) {
        Ext2SetManagerAsService(FALSE);
        return FALSE;
    }

    Ext2IsX64System();

    CExt2MgrDlg theDlg;

    theDlg.m_bHide = bHide;
    theDlg.m_bQuiet = bQuiet = bHide;
    theDlg.m_bService = bService;
    theDlg.m_bStat = bStat;

	m_pMainWnd = &theDlg;

    if (bService) {
        ManagerStartService(NULL);
    } else {
        ManagerStartMain(NULL);
    }

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
