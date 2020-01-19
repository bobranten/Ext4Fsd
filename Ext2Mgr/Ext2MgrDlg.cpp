// Ext2MgrDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ext2Mgr.h"
#include "PartitionType.h"
#include "DelDeadLetter.h"
#include "Ext2MgrDlg.h"
#include "DlgView.h"
#include <dbt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

    HBITMAP     m_hBitmap;
    HDC         m_hMemDC;
    HBITMAP     m_hOldBmp;

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
    // CMyHyperLink m_lMail;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnExt2fsd();
	afx_msg void OnDonate();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	// DDX_Control(pDX, IDC_AUTHOR, m_lMail);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_EXT2FSD, OnExt2fsd)
	ON_BN_CLICKED(ID_DONATE, OnDonate)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_hBitmap) {

        if (m_hMemDC) {
            ::SelectObject(m_hMemDC, m_hOldBmp);
            ::DeleteDC(m_hMemDC);
        }

        ::DeleteObject (m_hBitmap);
        m_hBitmap = NULL;
    }

	return CDialog::DestroyWindow();
}

BOOL CAboutDlg::OnInitDialog() 
{
    CString s;
    CHAR    Version[0x20];
    CHAR    Date[0x20];
    CHAR    Time[0x20];

	CDialog::OnInitDialog();
	
    m_hBitmap = (HBITMAP)::LoadImage(GetModuleHandle(NULL), 
                             MAKEINTRESOURCE(IDB_ABOUT_SMALL),
                             IMAGE_BITMAP, 0, 0, 0);

    if (m_hBitmap) {
        m_hMemDC  = ::CreateCompatibleDC(this->GetDC()->m_hDC);
        m_hOldBmp = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);
    }

    INT rc = Ext2QueryDrvVersion(Version, Date, Time);

    if (rc < 0) {
        s.Format("Ext2Fsd: NOT started !\0");
    } else if (rc > 0) {
        s.Format("Ext2Fsd: %s (%s)\0", Version, Date);
    } else {
        s.Format("Ext2Fsd: < 0.42 (Dec 2007)\0");
    }
    SET_TEXT(IDC_DRIVER, s);
    s  = "Ext2Mgr: 3.00 (";
    s += __DATE__;
    s += ")\0";
    SET_TEXT(IDC_PROGRAM, s);

    // Set the target URL 
    // m_lMail.SetLinkUrl("mailto:Matt Wu<matt@ext2fsd.com>?subject=Ext2Fsd Support");
    // Enable showing the Tooltip
    // m_lMail.ActiveToolTip(TRUE);
    // Set the Tooltiptext
    // m_lMail.SetTootTipText("Write a mail to Ext2Fsd group.");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
    int     rc;
    BITMAP  cs;

    rc = ::GetObject(m_hBitmap, sizeof(cs), &cs);
    if (rc == 0) {
        return;
    }

    CRect  rect;
	CWnd *pAboutWnd = GetDlgItem(IDC_ABOUT_SHOW);
	pAboutWnd->GetWindowRect(rect);
	ScreenToClient(rect);
  
    ::StretchBlt(dc.m_hDC, rect.left, rect.top, rect.Width(), rect.Height(),
                 m_hMemDC, 0, 0, cs.bmWidth, cs.bmHeight, SRCCOPY);

	// Do not call CDialog::OnPaint() for painting messages
}


void CAboutDlg::OnExt2fsd() 
{
	// TODO: Add your control notification handler code here
    ShellExecute(this->GetSafeHwnd(), "open", 
                 "http://www.ext2fsd.com", 
                 NULL, NULL, SW_SHOW );	
}

void CAboutDlg::OnDonate() 
{
	// TODO: Add your control notification handler code here
	GetParent()->SendMessage(WM_COMMAND, ID_DONATE);
}

void CAboutDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
// CExt2List message handlers

CExt2List::CExt2List()
{
}

CExt2List::~CExt2List()
{
}


BEGIN_MESSAGE_MAP(CExt2List, CListCtrl)
	//{{AFX_MSG_MAP(CExt2List)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CExt2List::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_Point = point;
	CListCtrl::OnRButtonDown(nFlags, point);
}

int CExt2List::QuerySubItemText(int item, CHAR *Data, int length)
{
    LV_COLUMN lvc;
    LV_ITEM lvi;
    int     ncol;
    CRect   rect;

    ::ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_WIDTH |LVCF_FMT;

    for (ncol=0; GetColumn(ncol, &lvc); ncol++) {

        if (ncol > 0) {
            GetSubItemRect(item, ncol,LVIR_BOUNDS, rect);
        } else {
            GetItemRect(item, rect, LVIR_BOUNDS);
            rect.right = GetColumnWidth(0);
            rect.left = 0;
        }

        if (rect.PtInRect(m_Point)) {

            ::ZeroMemory(Data, length);
            ::ZeroMemory(&lvi, sizeof(lvi));

            lvi.iItem = item;
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = ncol;
            lvi.pszText = Data;
            lvi.cchTextMax = length;

            return GetItem(&lvi);
            break;
        }
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CExt2MgrDlg dialog

CExt2MgrDlg::CExt2MgrDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExt2MgrDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExt2MgrDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_bHide = FALSE;
    m_bQuiet = FALSE;
    m_bService = FALSE;
    m_bStat = FALSE;

    m_splash = NULL;

    m_bFocusVolume = FALSE;
    m_IndexVolume = 0;
    m_bFocusDisk = FALSE;
    m_IndexDisk = 0;

    m_type = 0;
    m_sdev = NULL;

    m_PerfDlg = NULL;

    m_hUsbNotify = NULL;
    m_bHandleChange = FALSE;

    m_hAccel = NULL;
    m_bFsStarted = FALSE;

    m_nStartmode = 0;
    m_bAutoMount = FALSE;
	m_bExt3Writable = FALSE;
	m_bReadonly = FALSE;
}

void CExt2MgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExt2MgrDlg)
	DDX_Control(pDX, IDC_VOLUME_LIST, m_VolumeList);
	DDX_Control(pDX, IDC_DISK_LIST, m_DiskView);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CExt2MgrDlg, CDialog)
	//{{AFX_MSG_MAP(CExt2MgrDlg)
    ON_WM_WINDOWPOSCHANGING()
	ON_WM_SYSCOMMAND()
    ON_WM_DEVICECHANGE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_COMMAND(ID_CHANGE, OnChangeProperty)
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_COMMAND(ID_FORMAT, OnFormat)
	ON_COMMAND(ID_SERVICE, OnService)
	ON_COMMAND(ID_ABOUT, OnAbout)
	ON_COMMAND(ID_EXIT, OnExit)
	ON_WM_MEASUREITEM()
	ON_NOTIFY(NM_DBLCLK, IDC_DISK_LIST, OnDblclkDiskList)
	ON_NOTIFY(NM_KILLFOCUS, IDC_DISK_LIST, OnKillfocusDiskList)
	ON_NOTIFY(NM_RCLICK, IDC_DISK_LIST, OnRclickDiskList)
	ON_NOTIFY(NM_DBLCLK, IDC_VOLUME_LIST, OnDblclkVolumeList)
	ON_NOTIFY(NM_KILLFOCUS, IDC_VOLUME_LIST, OnKillfocusVolumeList)
	ON_NOTIFY(NM_RCLICK, IDC_VOLUME_LIST, OnRclickVolumeList)
	ON_NOTIFY(NM_CLICK, IDC_DISK_LIST, OnClickDiskList)
	ON_NOTIFY(NM_CLICK, IDC_VOLUME_LIST, OnClickVolumeList)
	ON_NOTIFY(NM_SETFOCUS, IDC_DISK_LIST, OnSetfocusDiskList)
	ON_NOTIFY(NM_SETFOCUS, IDC_VOLUME_LIST, OnSetfocusVolumeList)
	ON_COMMAND(ID_PROPERTY, OnProperty)
	ON_COMMAND(ID_DONATE, OnDonate)
	ON_COMMAND(ID_COPY,   OnCopy)
	ON_COMMAND(ID_COPYALL, OnCopyAll)
	ON_WM_TIMER()
	ON_COMMAND(ID_DRV_LETTER, OnDrvLetter)
	ON_COMMAND(ID_DRV_QUICK_MOUNT, OnDrvQuickMount)
	ON_COMMAND(ID_INSTALL_SERVICE, OnInstallService)
	ON_COMMAND(ID_REMOVE_SERVICE, OnRemoveService)
	ON_COMMAND(ID_ENABLE_AUTOSTART, OnEnableAutorun)
	ON_COMMAND(ID_DISABLE_AUTOSTART, OnDisableAutorun)
	ON_COMMAND(ID_SHOW_MAIN, OnShowMain)
	ON_MESSAGE(WM_TRAY_ICON_NOTIFY,  OnTrayNotification)
    ON_MESSAGE(WM_TERMINATE_PROGRAM, OnTerminate)
    ON_MESSAGE(WM_MOUNTPOINT_NOTIFY, OnMountPointNotify)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_PERFSTAT, OnPerfStat)
	ON_COMMAND(ID_PERFSTOP, OnPerfStop)
	ON_COMMAND(ID_FLUSH_BUFFER, OnFlush)
	ON_COMMAND(ID_CHANGE_PARTTYPE, OnPartType)
	ON_COMMAND(ID_REMOVE_DEAD_LETTER, OnRemoveDeadLetter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrDlg message handlers


static UINT BASED_CODE indicators[] =
{
    ID_INDICATOR_MESSAGE,
    ID_INDICATOR_TIME,
    ID_INDICATOR_EXTRA,
};

/* A5DCBF10-6530-11D2-901F-00C04FB951ED */
DEFINE_GUID(GUID_CLASS_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);


BOOL CExt2MgrDlg::OnInitDialog()
{
    CString    str;
    LONG_PTR   dwStyle = 0;

	CDialog::OnInitDialog();

    /* set windows identifier */
    SetWindowLongPtr(this->GetSafeHwnd(), DWLP_USER, EXT2_DIALOG_MAGIC);

	/* minimize the dialog during startup */
    if (m_bHide) {
        PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

    /* F1 - F12 key */
    m_hAccel = ::LoadAccelerators(AfxGetInstanceHandle(), 
                        MAKEINTRESOURCE(IDR_AKEY_EXT2MGR));

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);         // Set large icon
	SetIcon(m_hIcon, FALSE);        // Set small icon

    /* create new font for views */
    m_MSSanS.CreatePointFont(80, "MS Sans Serif");
    m_DiskView.SetFont(&m_MSSanS);
    m_VolumeList.SetFont(&m_MSSanS);

    /* initialize the disk view */
    dwStyle=GetWindowLongPtr(m_DiskView.GetSafeHwnd(),GWL_STYLE);
	dwStyle&=~LVS_TYPEMASK;
	dwStyle|=(LVS_REPORT | LVS_OWNERDRAWFIXED);
	SetWindowLongPtr(m_DiskView.GetSafeHwnd(),GWL_STYLE,dwStyle);

    m_DiskView.InsertColumn(0, (LPCSTR)"", LVCFMT_CENTER, 80);
    str.LoadString(IDS_LISTITEM_TYPE);
    m_DiskView.InsertColumn(1, (LPCSTR)str, LVCFMT_LEFT, 60);

    str.LoadString(IDS_LISTITEM_FILESYSTEM);
    m_DiskView.InsertColumn(2, (LPCSTR)str, LVCFMT_LEFT, 80);

    str.LoadString(IDS_LISTITEM_TOTALSIZE);
    m_DiskView.InsertColumn(3, (LPCSTR)str, LVCFMT_RIGHT, 80);

    str.LoadString(IDS_LISTITEM_USEDSIZE);
    m_DiskView.InsertColumn(4, (LPCSTR)str, LVCFMT_RIGHT, 70);

    str.LoadString(IDS_LISTITEM_CODEPAGE);
    m_DiskView.InsertColumn(5, (LPCSTR)str, LVCFMT_LEFT, 70);

    str.LoadString(IDS_LISTITEM_PARTID);
    m_DiskView.InsertColumn(6, (LPCSTR)str, LVCFMT_LEFT, 120);

    /* initialize volume list */
    dwStyle=GetWindowLongPtr(m_VolumeList.GetSafeHwnd(),GWL_STYLE);
	dwStyle&=~LVS_TYPEMASK;
	dwStyle|= (LVS_REPORT | LVS_AUTOARRANGE);
	SetWindowLongPtr(m_VolumeList.GetSafeHwnd(),GWL_STYLE,dwStyle);
	m_VolumeList.SetExtendedStyle(LVS_EX_GRIDLINES);

    m_VolumeList.InsertColumn(0, NULL, LVCFMT_CENTER, 20);
    str.LoadString(IDS_LISTITEM_VOLUME);
    m_VolumeList.InsertColumn(1, (LPCSTR)str, LVCFMT_LEFT, 60);

    str.LoadString(IDS_LISTITEM_TYPE);
    m_VolumeList.InsertColumn(2, (LPCSTR)str, LVCFMT_LEFT, 60);

    str.LoadString(IDS_LISTITEM_FILESYSTEM);
    m_VolumeList.InsertColumn(3, (LPCSTR)str, LVCFMT_LEFT, 80);

    str.LoadString(IDS_LISTITEM_TOTALSIZE);
    m_VolumeList.InsertColumn(4, (LPCSTR)str, LVCFMT_RIGHT, 80);

    str.LoadString(IDS_LISTITEM_USEDSIZE);
    m_VolumeList.InsertColumn(5, (LPCSTR)str, LVCFMT_RIGHT, 70);

    str.LoadString(IDS_LISTITEM_CODEPAGE);
    m_VolumeList.InsertColumn(6, (LPCSTR)str, LVCFMT_LEFT, 70);

    str.LoadString(IDS_LISTITEM_DEVOBJ);
    m_VolumeList.InsertColumn(7, (LPCSTR)str, LVCFMT_LEFT, 200);

    ListView_SetExtendedListViewStyleEx ( 
        m_VolumeList.GetSafeHwnd(), 
        LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

    /* initialize ImageList */
	m_ImageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 5, 5);
	for (UINT nID = IDI_FLOPPY; nID <= IDI_DYNAMIC; nID++) {
      	CBitmap     bitmap;
        if (bitmap.LoadBitmap(nID)) {
		    m_ImageList.Add(&bitmap, RGB(0,0,0));
		    bitmap.DeleteObject();
        }
	}
	m_VolumeList.SetImageList(&m_ImageList, LVSIL_SMALL);

    /* Status Bar Initialization */
    m_bar.Create(this); //We create the status bar
    m_bar.SetIndicators(indicators, 3);

    CRect rect;
    GetClientRect(&rect);
    m_bar.SetPaneInfo(0,ID_INDICATOR_MESSAGE, SBPS_NORMAL,rect.Width()-160);
    m_bar.SetPaneInfo(1,ID_INDICATOR_TIME,SBPS_NORMAL ,132);
    m_bar.SetPaneInfo(2,ID_INDICATOR_EXTRA,SBPS_STRETCH ,0);
    /* m_bar.GetStatusBarCtrl().SetBkColor(RGB(180,180,180)); */
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,
                   ID_INDICATOR_EXTRA);

    CTime t1;
    t1 = CTime::GetCurrentTime();

    CString s;
    s.Format("%s", "Ready");
    m_bar.SetPaneText(0, s);

    s.Format(" %3.3s", t1.Format("%B"));
    s += t1.Format(" %d,%Y %H:%M:%S");
    m_bar.SetPaneText(1, s);

    SetTimer(ID_INDICATOR_TIME,1000,NULL);

    /* close the splash window */
    PostMessage(WM_SYSCOMMAND, IDM_CLOSE_SPLASH, 0);

    /* loading system configurations */
    if (Ext2QuerySysConfig()) {
        Ext2LoadDisks();
        Ext2LoadCdroms();
        if (Ext2LoadVolumes()) {
            Ext2LoadRemovableVolumes();
            Ext2LoadDrvLetters();
            Ext2LoadCdromDrvLetters();
            Ext2LoadAllVolumeDrvLetters();
            Ext2LoadAllDiskPartitions();
            if (g_bAutoRemoveDeadLetters) {
                Ext2AutoRemoveDeadLetters();
            }
            if (Ext2ProcessExt2Volumes()) {
            }
        }
    } else {
        return FALSE;
    }

    m_bHandleChange = TRUE;

    /* updating the volume list */
    Ext2RefreshVolumeList(&m_VolumeList);

    /* updating the disk list */    
    Ext2RefreshDiskList(&m_DiskView);

    CMenu* pMenu = AfxGetMainWnd()->GetMenu();
    CMenu* pSubFile = pMenu->GetSubMenu(0);
    if (pSubFile) {
        if (Ext2RunMgrForCurrentUser()) {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
        } else {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
        }
    }

    m_Menu.CreatePopupMenu();

    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_Tray.Create(this, WM_TRAY_ICON_NOTIFY,"Ext2 Volume Manager",hIcon, IDR_TRAY);

    /* start Ext2Fsd statistics window */
    if (m_bStat) {
        PostMessage(WM_COMMAND, ID_PERFSTAT, 0);
    }

    /* query global parameters */
    Ext2QueryGlobalProperty(
            &m_nStartmode,
            (BOOL *)&m_bReadonly,
            (BOOL *)&m_bExt3Writable,
            (CHAR *)m_Codepage.GetBuffer(CODEPAGE_MAXLEN),
            (CHAR *)m_sPrefix.GetBuffer(HIDINGPAT_LEN),
            (CHAR *)m_sSuffix.GetBuffer(HIDINGPAT_LEN),
            (BOOL *)&m_bAutoMount
            );
    g_bAutoMount = m_bAutoMount;
    m_Codepage.ReleaseBuffer(-1);
    m_sPrefix.ReleaseBuffer(-1);
    m_sSuffix.ReleaseBuffer(-1);

    RegisterDeviceInterface(DiskClassGuid, &m_hUsbNotify);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CExt2MgrDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else if ((nID & 0xFFF0) ==  SC_MINIMIZE) {
		m_Tray.ShowIcon();
		ShowWindow(SW_HIDE);
	} else if ((nID & 0xFFF0) ==  SC_CLOSE) {
        SendMessage(WM_COMMAND, ID_EXIT, 0);
	} else if ((nID & 0xFFF0) == IDM_CLOSE_SPLASH) {
        if (m_splash) {
            m_splash->CloseSplash();
            delete m_splash;
            m_splash = NULL;
        }
	} else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

BOOL CExt2MgrDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
    PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;
    PDEV_BROADCAST_DEVICEINTERFACE pdbch = (PDEV_BROADCAST_DEVICEINTERFACE)dwData;
    if (pdbch && pdbch->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {

        if (nEventType == DBT_DEVICEARRIVAL || DBT_DEVICEREMOVECOMPLETE) {
            KillTimer('REFR');
            SetTimer('REFR', 1000, NULL);
        }

        return TRUE;
    }

    switch (nEventType) {

        case DBT_DEVICEARRIVAL:

            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                DriversChangeNotify(lpdbv->dbcv_unitmask, TRUE);
                Ext2AddLetterMask((ULONGLONG)(lpdbv->dbcv_unitmask));
            }

            break;
        case DBT_DEVICEQUERYREMOVE:
            break;
        case DBT_DEVICEQUERYREMOVEFAILED:
            break;
        case DBT_DEVICEREMOVEPENDING:
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                DriversChangeNotify(lpdbv->dbcv_unitmask, FALSE);
            }
            break;
        case DBT_DEVICETYPESPECIFIC:
            break;
        case DBT_CONFIGCHANGED:
            break;
        default:
        break;
    }

    return TRUE;
}

void CExt2MgrDlg::OnDestroy()
{
    KillTimer(ID_INDICATOR_TIME);

    Ext2CleanupDisks();
    Ext2CleanupCdroms();
    Ext2CleanupVolumes();
    Ext2CleanupDrvLetters();

    OnPerfStop();

    if (m_hUsbNotify) {
        UnregisterDeviceNotification(m_hUsbNotify);
        m_hUsbNotify = NULL;
    }

    if (m_hAccel) {
        DestroyAcceleratorTable(m_hAccel);
        m_hAccel = NULL;
    }

	CDialog::OnDestroy();
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CExt2MgrDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CExt2MgrDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CExt2MgrDlg::OnSize(UINT nType, int cx, int cy) 
{
    int i = 0;
    int ctlId[] = {IDC_VOLUME_LIST, IDC_DISK_LIST, 0};

    return;

    // create an instance of the CRect object
    CRect crw, cri;

    do {

        CWnd *pWnd = GetDlgItem(ctlId[i]);
        if (pWnd) {
            pWnd->GetWindowRect(&cri);
            ScreenToClient(&cri);
            pWnd->SetWindowPos(pWnd,
                               cri.left * cx / crw.Width(),
                               cri.top * cy / crw.Height(),
                               cri.Width() * cx / crw.Width(),
                               cri.Height() * cy / crw.Height(),
                               SWP_NOZORDER | SWP_SHOWWINDOW);
        }

    } while (ctlId[++i]);
}

LRESULT CExt2MgrDlg::OnTrayNotification(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(lParam))
	{
	case WM_LBUTTONUP:
        m_bHide = FALSE;
		ShowWindow(SW_SHOW);
		SendMessage(WM_SYSCOMMAND, SC_RESTORE);
		break;
	}

	return m_Tray.OnTrayNotification(wParam,lParam);
}

void CExt2MgrDlg::OnAbout() 
{
	// TODO: Add your command handler code here
    SendMessage(WM_SYSCOMMAND, IDM_ABOUTBOX);
}

void CExt2MgrDlg::OnExit() 
{
    if (TRUE) {
        /* AfxMessageBox("Are you sure to exit ? ",MB_YESNO,0) == IDYES */
        EndDialog(0);
    }
}

void CExt2MgrDlg::OnOK() 
{
}

void CExt2MgrDlg::OnCancel() 
{
}

void CExt2MgrDlg::OnRefresh() 
{
    m_bHandleChange = FALSE;
    m_bFocusVolume = FALSE;
    m_bFocusDisk = FALSE;

    Ext2AutoRemoveDeadLetters();

    /* cleanup all the disk/volume structures */
    Ext2CleanupDisks();
    Ext2CleanupCdroms();
    Ext2CleanupVolumes();
    Ext2CleanupDrvLetters();

    /* loading system configurations */
    if (Ext2QuerySysConfig()) {
        Ext2LoadDisks();
        Ext2LoadCdroms();
        if (Ext2LoadVolumes()) {
            Ext2LoadRemovableVolumes();
            Ext2LoadDrvLetters();
            Ext2LoadCdromDrvLetters();
            Ext2LoadAllVolumeDrvLetters();
            Ext2LoadAllDiskPartitions();
            if (g_bAutoRemoveDeadLetters) {
                Ext2AutoRemoveDeadLetters();
            }
            if (Ext2ProcessExt2Volumes()) {
            }
        }
    } else {
        return;
    }

    /* updating the volume list */
    Ext2RefreshVolumeList(&m_VolumeList);

    /* updating the disk list */    
    Ext2RefreshDiskList(&m_DiskView);
	
    m_bHandleChange = TRUE;
}

void CExt2MgrDlg::OnService() 
{
	// TODO: Add your command handler code here
    CServiceManage  SrvDlg;
    if (!SrvDlg.m_bInited) {
        AfxMessageBox("Cannot query Ext2Fsd service !", MB_OK|MB_ICONSTOP);
        return;
    }

    /* query global parameters */
    Ext2QueryGlobalProperty(
            &m_nStartmode,
            (BOOL *)&m_bReadonly,
            (BOOL *)&m_bExt3Writable,
            (CHAR *)m_Codepage.GetBuffer(CODEPAGE_MAXLEN),
            (CHAR *)m_sPrefix.GetBuffer(HIDINGPAT_LEN),
            (CHAR *)m_sSuffix.GetBuffer(HIDINGPAT_LEN),
            (BOOL *)&m_bAutoMount
            );
    g_bAutoMount = m_bAutoMount;
    m_Codepage.ReleaseBuffer(-1);
    m_sPrefix.ReleaseBuffer(-1);
    m_sSuffix.ReleaseBuffer(-1);

    SrvDlg.m_nStartmode     = m_nStartmode;
    SrvDlg.m_bReadonly      = m_bReadonly;
    SrvDlg.m_bExt3Writable  = m_bExt3Writable;
    SrvDlg.m_bAutoMount     = m_bAutoMount;
    SrvDlg.m_Codepage       = m_Codepage;
    SrvDlg.m_sPrefix        = m_sPrefix;
    SrvDlg.m_sSuffix        = m_sSuffix;
    if (IDOK == SrvDlg.DoModal()) {
        /* query global parameters */
        Ext2QueryGlobalProperty(
                &m_nStartmode,
                (BOOL *)&m_bReadonly,
                (BOOL *)&m_bExt3Writable,
                (CHAR *)m_Codepage.GetBuffer(CODEPAGE_MAXLEN),
                (CHAR *)m_sPrefix.GetBuffer(HIDINGPAT_LEN),
                (CHAR *)m_sSuffix.GetBuffer(HIDINGPAT_LEN),
                (BOOL *)&m_bAutoMount
                );
        g_bAutoMount = m_bAutoMount;
    }
}


void CExt2MgrDlg::OnChangeProperty() 
{
    CExt2Attribute EA;

    PEXT2_VOLUME_PROPERTY3 EVP = NULL;

    PEXT2_VOLUME   volume = NULL;
    PEXT2_CDROM    cdrom = NULL;

    if (m_bFocusVolume) {
        if (m_type == EXT2_VOLUME_MAGIC) {
            volume = (PEXT2_VOLUME) m_sdev;
            EVP = &volume->EVP;
        } else if (m_type == EXT2_CDROM_DEVICE_MAGIC) {
            cdrom = (PEXT2_CDROM) m_sdev;
            EVP = &cdrom->EVP;
        }
    } else {
        if (m_type == EXT2_PART_MAGIC) {
            PEXT2_PARTITION part = (PEXT2_PARTITION) m_sdev;
            volume = part->Volume;
            EVP = &volume->EVP;
        } else if (m_type == EXT2_CDROM_VOLUME_MAGIC ||
                   m_type == EXT2_CDROM_DEVICE_MAGIC ) {
            cdrom = (PEXT2_CDROM) m_sdev;
            EVP = &cdrom->EVP;
        }
    }

    if (EVP) {

        NT::NTSTATUS status;
        HANDLE  Handle = NULL;
        CString s;

        EA.m_EVP = EVP;
        if (volume) {
            if (volume->Part)
                EA.m_DevName = volume->Part->Name;
            else
                EA.m_DevName = volume->Name;
        } else {
            EA.m_bCdrom = TRUE;
            EA.m_DevName = cdrom->Name;
        }

        status = Ext2Open(EA.m_DevName.GetBuffer(EA.m_DevName.GetLength()),
                          &Handle, EXT2_DESIRED_ACCESS);

        if (!NT_SUCCESS(status)) {

            s.Format("Ext2Fsd service isn't started.\n");
            AfxMessageBox(s, MB_OK | MB_ICONSTOP);

        } else {

            if (!Ext2QueryExt2Property(Handle, EVP)) {
                Ext2Close(&Handle);
                return;
            }

            Ext2Close(&Handle);
        }

        EA.m_MainDlg = (CWnd *)this;
        if (EA.DoModal() == IDOK) {
        }
        if (volume) {
            UpdateVolume(volume);
        } else if (cdrom) {
            UpdateCdrom(cdrom);
        }
    }
}

void CExt2MgrDlg::OnFormat() 
{
    CString str;
	// TODO: Add your command handler code here
    if (m_bFocusVolume) {
        str.Format("Formating volume item: %d", m_IndexVolume);
    }
    if (m_bFocusDisk) {
        str.Format("Formating disk item: %d", m_IndexDisk);
    }

    if (!str.IsEmpty())
        AfxMessageBox(str);
}

void CExt2MgrDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if(m_bHide)
        lpwndpos->flags &= ~SWP_SHOWWINDOW;

    CDialog::OnWindowPosChanging(lpwndpos);
}

BOOL CExt2MgrDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	// TODO: Add your specialized code here and/or call the base class

	LPNMHDR pNmhdr = (LPNMHDR)lParam;

	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CExt2MgrDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
    if (nIDCtl == IDC_DISK_LIST) {
        m_DiskView.MeasureItem(lpMeasureItemStruct);
    } else {
	    CDialog::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
    }
}

void CExt2MgrDlg::OnDblclkDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    BOOL IsExt2 = FALSE;
    m_bFocusVolume = FALSE;
    m_bFocusDisk = TRUE;
    m_IndexDisk = m_DiskView.GetSelectionMark();

    if (!m_bFsStarted) {
        m_bFsStarted = Ext2IsServiceStarted();
    }

    if (QuerySelectedItem(&IsExt2)) {
        if (IsExt2 && m_bFsStarted) {
            SendMessage(WM_COMMAND, ID_CHANGE, 0);
        } else {
            SendMessage(WM_COMMAND, ID_PROPERTY, 0);
        }
    }

	*pResult = 0;
}

void CExt2MgrDlg::OnSetfocusDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	m_bFocusDisk = TRUE;
    m_bFocusVolume = FALSE;
    //  m_bar.SetPaneText(0, CString("Disk: Set Focus"));

    QuerySelectedItem(NULL);
	*pResult = 0;
}

void CExt2MgrDlg::OnKillfocusDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    m_bFocusDisk = FALSE;
    // m_bar.SetPaneText(0, CString("Disk: Focus Lost"));

    QuerySelectedItem(NULL);
	*pResult = 0;
}

void CExt2MgrDlg::OnRclickDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    RECT    rect;
    CString s;

    m_bFocusVolume = FALSE;
    m_bFocusDisk = TRUE;
    m_IndexDisk = m_DiskView.GetSelectionMark();

    if (!QuerySelectedItem(NULL)) {
        return;
    }

    while (m_Menu.DeleteMenu(0, MF_BYPOSITION));

    if (m_bFocusDisk) {

        if (m_type == EXT2_CDROM_DEVICE_MAGIC ||
            m_type == EXT2_CDROM_VOLUME_MAGIC ) {

            PEXT2_CDROM cdrom = (PEXT2_CDROM) m_sdev;

            s.LoadString(IDS_DRV_QUICK_MOUNT);
            if (0 == cdrom->DrvLetters)
                m_Menu.AppendMenu(MF_STRING, ID_DRV_QUICK_MOUNT, (LPCTSTR)s);

            s.LoadString(IDS_CHANGE_DRVLETTER);
            m_Menu.AppendMenu(MF_STRING, ID_DRV_LETTER, (LPCTSTR)s);

            m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
            if (cdrom->bLoaded && !cdrom->bEjected && 
                (cdrom->EVP.bExt2 || cdrom->EVP.bExt3)) {

                s.LoadString(IDS_EXT2_MANAGEMENT);
                m_Menu.AppendMenu(MF_STRING, ID_CHANGE, (LPCTSTR)s);
            }

            m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

            s.LoadString(IDS_COPY_ITEM_TO_CLIP);
            m_Menu.AppendMenu(MF_STRING, ID_COPY, (LPCTSTR)s);
            m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

        } else if (m_type == EXT2_PART_MAGIC) {

            PEXT2_PARTITION part = (PEXT2_PARTITION) m_sdev;
            PEXT2_VOLUME volume = part->Volume;

            if (volume && !volume->bDynamic) {
                s.LoadString(IDS_DRV_QUICK_MOUNT);
                if (0 == volume->DrvLetters)
                    m_Menu.AppendMenu(MF_STRING, ID_DRV_QUICK_MOUNT, (LPCTSTR)s);

                s.LoadString(IDS_CHANGE_DRVLETTER);
                m_Menu.AppendMenu(MF_STRING, ID_DRV_LETTER, (LPCTSTR)s);
                m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
            }

            if (volume && volume->bRecognized && (volume->EVP.bExt2 || volume->EVP.bExt3)) {
                s.LoadString(IDS_EXT2_MANAGEMENT);
                m_Menu.AppendMenu(MF_STRING, ID_CHANGE, (LPCTSTR)s);
                m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
            }

            if (part->Entry) {
                s.LoadString(IDS_FLUSH_BUFFER);
                m_Menu.AppendMenu(MF_STRING, ID_FLUSH_BUFFER, (LPCTSTR)s);

                if (part->PartType == PARTITION_STYLE_MBR) {
                    s.LoadString(IDS_CHANGE_PARTID);
                    m_Menu.AppendMenu(MF_STRING, ID_CHANGE_PARTTYPE, (LPCTSTR)s);
                }
                m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
            }

            s.LoadString(IDS_COPY_ITEM_TO_CLIP);
            m_Menu.AppendMenu(MF_STRING, ID_COPY, (LPCTSTR)s);

            m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

        } else {
            ASSERT(m_type == EXT2_DISK_MAGIC);
        }
    }

    s.LoadString(IDS_RELOAD_REFRESH);
    m_Menu.AppendMenu(MF_STRING, ID_REFRESH, (LPCTSTR)s);
    m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

    s.LoadString(IDS_SHOW_PROPERTY);
    m_Menu.AppendMenu(MF_STRING, ID_PROPERTY, (LPCTSTR)s);
    m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

    s.LoadString(IDS_SERVICE_MANAGE);
    m_Menu.AppendMenu(MF_STRING, ID_SERVICE, (LPCTSTR)s);

    s.LoadString(IDS_PERFMEM_STAT);
    m_Menu.AppendMenu(MF_STRING, ID_PERFSTAT, (LPCTSTR)s);

    s.LoadString(IDS_REMOVE_DEAD_LETTER);
    m_Menu.AppendMenu(MF_STRING, ID_REMOVE_DEAD_LETTER, (LPCTSTR)s);

    m_DiskView.GetWindowRect(&rect);
    m_Menu.TrackPopupMenu( TPM_LEFTALIGN,
                           rect.left + m_DiskView.m_Point.x,
                           rect.top + m_DiskView.m_Point.y,
                           m_DiskView.GetSafeOwner(),
                           NULL); 

	*pResult = 0;
}


void CExt2MgrDlg::OnClickDiskList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    m_bFocusVolume = FALSE;
    m_bFocusDisk = TRUE;
    m_IndexDisk = m_DiskView.GetSelectionMark();

    QuerySelectedItem(NULL);
	*pResult = 0;
}

void CExt2MgrDlg::OnDblclkVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    BOOL bIsExt2 = FALSE;

    if (!m_bFsStarted) {
        m_bFsStarted = Ext2IsServiceStarted();
    }

    m_bFocusVolume = TRUE;
    m_IndexVolume = m_VolumeList.GetSelectionMark();;
    m_bFocusDisk = FALSE;
    if (QuerySelectedItem(&bIsExt2)) {
        if (bIsExt2 && m_bFsStarted) {
            SendMessage(WM_COMMAND, ID_CHANGE, 0);
        } else {
            SendMessage(WM_COMMAND, ID_PROPERTY, 0);
        }
    }
	*pResult = 0;
}

void CExt2MgrDlg::OnSetfocusVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    //m_bar.SetPaneText(0, CString("Volume: Set Focus"));
    m_bFocusVolume = TRUE;
	*pResult = 0;
    QuerySelectedItem(NULL);
}

void CExt2MgrDlg::OnKillfocusVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    m_bFocusVolume = FALSE;
    //m_bar.SetPaneText(0, CString("Volume: Focus Lost"));

	// TODO: Add your control notification handler code here
	*pResult = 0;
    QuerySelectedItem(NULL);
}

void CExt2MgrDlg::OnRclickVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    RECT    rect;
    CString s;

    m_bFocusVolume = TRUE;
    m_IndexVolume = m_VolumeList.GetSelectionMark();;
    m_bFocusDisk = FALSE;

    if (!QuerySelectedItem(NULL)) {
        return;
    }

    while (m_Menu.DeleteMenu(0, MF_BYPOSITION));

    if (m_bFocusVolume) {

        if (m_type == EXT2_VOLUME_MAGIC) {

            PEXT2_VOLUME volume = (PEXT2_VOLUME) m_sdev;
            PEXT2_PARTITION part;

            s.LoadString(IDS_DRV_QUICK_MOUNT);
            if (0 == volume->DrvLetters)
                m_Menu.AppendMenu(MF_STRING, ID_DRV_QUICK_MOUNT, (LPCTSTR)s);

            s.LoadString(IDS_CHANGE_DRVLETTER);
            m_Menu.AppendMenu(MF_STRING, ID_DRV_LETTER, (LPCTSTR)s);
            m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

            if (volume->bRecognized && (volume->EVP.bExt2 || volume->EVP.bExt3)) {
                s.LoadString(IDS_EXT2_MANAGEMENT);
                m_Menu.AppendMenu(MF_STRING, ID_CHANGE, (LPCTSTR)s);
                m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
            }

            s.LoadString(IDS_FLUSH_BUFFER);
            m_Menu.AppendMenu(MF_STRING, ID_FLUSH_BUFFER, (LPCTSTR)s);

            part = Ext2QueryVolumePartition(volume);
            if (part && part->PartType == PARTITION_STYLE_MBR) {
                s.LoadString(IDS_CHANGE_PARTID);
                m_Menu.AppendMenu(MF_STRING, ID_CHANGE_PARTTYPE, (LPCTSTR)s);
            }

        } else if (m_type == EXT2_CDROM_DEVICE_MAGIC ||
                   m_type == EXT2_CDROM_VOLUME_MAGIC ) {

            PEXT2_CDROM cdrom = (PEXT2_CDROM)m_sdev;

            s.LoadString(IDS_DRV_QUICK_MOUNT);
            if (0 == cdrom->DrvLetters)
                m_Menu.AppendMenu(MF_STRING, ID_DRV_QUICK_MOUNT, (LPCTSTR)s);

            s.LoadString(IDS_CHANGE_DRVLETTER);
            m_Menu.AppendMenu(MF_STRING, ID_DRV_LETTER, (LPCTSTR)s);

            if (cdrom->bLoaded && !cdrom->bEjected && 
                (cdrom->EVP.bExt2 || cdrom->EVP.bExt3)) {
                m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
                s.LoadString(IDS_EXT2_MANAGEMENT);
                m_Menu.AppendMenu(MF_STRING, ID_CHANGE, (LPCTSTR)s);
            }
       }

        m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
        s.LoadString(IDS_COPY_ITEM_TO_CLIP);
        m_Menu.AppendMenu(MF_STRING, ID_COPY, (LPCTSTR)s);
        m_Menu.AppendMenu(MF_SEPARATOR, 0, "");
    }

    s.LoadString(IDS_RELOAD_REFRESH);
    m_Menu.AppendMenu(MF_STRING, ID_REFRESH, (LPCTSTR)s);
    m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

    s.LoadString(IDS_SHOW_PROPERTY);
    m_Menu.AppendMenu(MF_STRING, ID_PROPERTY, (LPCTSTR)s);
    m_Menu.AppendMenu(MF_SEPARATOR, 0, "");

    s.LoadString(IDS_SERVICE_MANAGE);
    m_Menu.AppendMenu(MF_STRING, ID_SERVICE, (LPCTSTR)s);

    s.LoadString(IDS_PERFMEM_STAT);
    m_Menu.AppendMenu(MF_STRING, ID_PERFSTAT, (LPCTSTR)s);

    s.LoadString(IDS_REMOVE_DEAD_LETTER);
    m_Menu.AppendMenu(MF_STRING, ID_REMOVE_DEAD_LETTER, (LPCTSTR)s);

    m_VolumeList.GetWindowRect(&rect);
    m_Menu.TrackPopupMenu(TPM_LEFTALIGN,
                          rect.left + m_VolumeList.m_Point.x ,
                          rect.top + m_VolumeList.m_Point.y,
                          m_VolumeList.GetSafeOwner(), NULL); 
 
	*pResult = 0;
}


void CExt2MgrDlg::OnClickVolumeList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    m_bFocusVolume = TRUE;
    m_IndexVolume = m_VolumeList.GetSelectionMark();;
    m_bFocusDisk = FALSE;

    QuerySelectedItem(NULL);

	*pResult = 0;
}


void CExt2MgrDlg::OnProperty() 
{
	PVOID sdev =  QuerySelectedItem(NULL);
    if (sdev) {
        CProperties PPD;
        PPD.m_bdisk = m_bFocusDisk;
        PPD.m_type  = m_type;
        PPD.m_sdev  = sdev;
        PPD.DoModal();
    }
}

VOID
CExt2MgrDlg::DriversChangeNotify(
    ULONG           drvsMask,
    BOOL            bArrival
    )
{
    for (ULONG i=0; i < 26; i++) {

        PEXT2_LETTER drvLetter = &drvLetters[i];

        if (drvsMask & (1 << i)) {
            DriverChangeNotify(drvLetter, bArrival);
        }
    }
}

VOID
CExt2MgrDlg::DriverLetterChangeNotify(
    CHAR            cLetter,
    BOOL         bArrival
    )
{
    PEXT2_LETTER drvLetter = NULL;

    if (cLetter >= '0' && cLetter <= '9') {
        drvLetter = &drvDigits[cLetter - '0'];
    } else if (cLetter >= 'A' && cLetter <= 'Z') {
        drvLetter = &drvLetters[cLetter - 'A'];
    }

    if (drvLetter) {
        DriverChangeNotify(drvLetter, bArrival);
    }
}

VOID
CExt2MgrDlg::DriverChangeNotify(
    PEXT2_LETTER    drvLetter,
    BOOL         bArrival
    )
{
    ULONGLONG LetterMask = 0;
    ULONG     i;

    if (!m_bHandleChange) {
        return;
    }

    if (bArrival) {
        if (!drvLetter->bUsed) {
            Ext2CleanDrvLetter(drvLetter);
            Ext2QueryDrvLetter(drvLetter);
        }
    }

    if (drvLetter->Letter >= '0' && drvLetter->Letter <= '9') {
        LetterMask =  ((ULONGLONG) 1) << (drvLetter->Letter - '0' + 32);
    } else if (drvLetter->Letter >= 'A' && drvLetter->Letter <= 'Z') {
        LetterMask =  ((ULONGLONG) 1) << (drvLetter->Letter - 'A');
    }

    if (drvLetter->bUsed) {

        if (NULL != drvLetter->Extent) {

            PEXT2_VOLUME  Volume = &gVols[0];
            for (i=0; i < g_nVols && Volume != NULL; i++) {
                if (Ext2CompareExtents(drvLetter->Extent, Volume->Extent)) {
                    if (bArrival) {
                        Volume->DrvLetters |= LetterMask;
                    } else {
                        Volume->DrvLetters &= ~LetterMask;
                    }
                    UpdateVolume(Volume);
                    break;
                }
                Volume = Volume->Next;
            }
        }

        for (i=0; i < g_nCdroms; i++) {
            PEXT2_CDROM  Cdrom = &gCdroms[i];
            if (!_stricmp(drvLetters->SymLink, Cdrom->Name)) {
                if (bArrival) {
                    Cdrom->DrvLetters |= LetterMask;
                } else {
                    Cdrom->DrvLetters &= ~LetterMask;
                }
                UpdateCdrom(Cdrom);
                break;
            }
        }
    }

    if (!bArrival) {

        //Ext2SymLinkRemoval(drvLetter->Letter);
        //Ext2RemoveDosSymLink(drvLetter->Letter);

        if (drvLetter->bUsed) {
            //Ext2DismountVolume(drvLetter->SymLink);
            Ext2RemoveMountPoint(drvLetter, FALSE);
        }
    }
}

void CExt2MgrDlg::UpdateVolume(PEXT2_VOLUME volume) 
{
    for (int i = 0; i < m_VolumeList.GetItemCount(); i++) {
        PULONG data = (PULONG)m_VolumeList.GetItemData(i);
        if (!data) {
            continue;
        }
        if (*data == EXT2_VOLUME_MAGIC &&
            data == (PULONG)volume) {
            Ext2RefreshVLVI(&m_VolumeList, volume, i);
        }
    }

   for (int i = 0; i < m_DiskView.GetItemCount(); i++) {
        PEXT2_PARTITION part;
        part = (PEXT2_PARTITION)m_DiskView.GetItemData(i);
        if (!part) {
            continue;
        }
        if (part->Magic == EXT2_PART_MAGIC &&
            part->Volume == volume) {
            part->DrvLetters = volume->DrvLetters;
            Ext2RefreshDVPT(&m_DiskView, part, i);
        }
    }
}

void CExt2MgrDlg::UpdateCdrom(PEXT2_CDROM cdrom) 
{
    for (int i = 0; i < m_VolumeList.GetItemCount(); i++) {
        PULONG data = (PULONG)m_VolumeList.GetItemData(i);
        if (!data) {
            continue;
        }
        if (*data == EXT2_CDROM_DEVICE_MAGIC &&
            data == (PULONG)cdrom) {
            Ext2RefreshVLCD(&m_VolumeList, cdrom, i);
        }
    }

    for (int i = 0; i < m_DiskView.GetItemCount(); i++) {
        PULONG data = (PULONG)m_DiskView.GetItemData(i);
        if (!data) {
            continue;
        }
        if (*data == EXT2_CDROM_VOLUME_MAGIC &&
            (PUCHAR)data == ((PUCHAR)cdrom + 4)) {
            Ext2RefreshDVCM(&m_DiskView, cdrom, i);
        }
    }
}

void CExt2MgrDlg::UpdatePartition(PEXT2_PARTITION part)
{
    int i;

    if (part->Volume) {
        part->Volume->DrvLetters = part->DrvLetters;
        UpdateVolume(part->Volume);
        return;
    }

    for (i = 0; i < m_DiskView.GetItemCount(); i++) {

        PULONG data = (PULONG)m_DiskView.GetItemData(i);
        if (!data) {
            continue;
        }
        if (*data == EXT2_PART_MAGIC &&
            data == (PULONG)part) {
            Ext2RefreshDVPT(&m_DiskView, part, i);
        }
    }
} 

void CExt2MgrDlg::OnDrvLetter() 
{
	// TODO: Add your command handler code here
    CMountPoints    mntPoint;
    if (m_bFocusVolume) {
        if (m_type == EXT2_VOLUME_MAGIC) {
            mntPoint.m_Volume = (PEXT2_VOLUME) m_sdev;
        } else {
            mntPoint.m_Cdrom = (PEXT2_CDROM) m_sdev;
        }
    } else {
        if (m_type == EXT2_CDROM_DEVICE_MAGIC ||
            m_type == EXT2_CDROM_VOLUME_MAGIC ) {
            mntPoint.m_Cdrom = (PEXT2_CDROM) m_sdev;
        } else {
            mntPoint.m_Part = (PEXT2_PARTITION) m_sdev;
        }
    }

    mntPoint.m_MainDlg = this;
    mntPoint.DoModal();

    /* to be done in OnDeviceChange */

    /*
    if (mntPoint.m_bUpdated) {

        if (mntPoint.m_Volume) {
            UpdateVolume(mntPoint.m_Volume);
        }
        if (mntPoint.m_Cdrom) {
            UpdateCdrom(mntPoint.m_Cdrom);
        }
        if (mntPoint.m_Part) {
            UpdatePartition(mntPoint.m_Part);
        }
    }
    */
}

void CExt2MgrDlg::OnDrvQuickMount() 
{
    CHAR    drv = 0;

    if (m_bFocusVolume) {
        if (m_type == EXT2_VOLUME_MAGIC) {
            PEXT2_VOLUME v = (PEXT2_VOLUME)m_sdev;
            if (0 == v->DrvLetters) {
                drv = Ext2MountVolume(v->Name);
                if (drv) {
                    v->DrvLetters |= ((ULONGLONG) 1) << (drv - 'A'); 
                    UpdateVolume(v);
                }
            }
        } else {
            PEXT2_CDROM c = (PEXT2_CDROM) m_sdev;
            if (0 == c->DrvLetters) {
                drv = Ext2MountVolume(c->Name);
                if (drv) {
                    c->DrvLetters |= ((ULONGLONG) 1) << (drv - 'A'); 
                    UpdateCdrom(c);
                }
            }
        }
    } else {
        if (m_type == EXT2_CDROM_DEVICE_MAGIC ||
            m_type == EXT2_CDROM_VOLUME_MAGIC ) {
            PEXT2_CDROM c = (PEXT2_CDROM) m_sdev;
            if (0 == c->DrvLetters) {
                drv = Ext2MountVolume(c->Name);
                if (drv) {
                    c->DrvLetters |= ((ULONGLONG) 1) << (drv - 'A'); 
                    UpdateCdrom(c);
                }
            }
        } else {
            PEXT2_PARTITION p = (PEXT2_PARTITION) m_sdev;
            if (0 == p->DrvLetters) {
                drv = Ext2MountVolume(p->Name);
                if (drv) {
                    p->DrvLetters |= ((ULONGLONG) 1) << (drv - 'A'); 
                    UpdatePartition(p);
                }
            }
        }
    }
}


void CExt2MgrDlg::OnDonate() 
{
	// TODO: Add your command handler code here
    CDonate Donate;
    Donate.DoModal();
}

void CExt2MgrDlg::OnCopy() 
{
    CHAR Data[256];
    BOOL rc = FALSE;

	// TODO: Add your command handler code here
    if (m_bFocusVolume) {
	    rc = m_VolumeList.QuerySubItemText(m_IndexVolume, Data, 256);
    } else if (m_bFocusDisk) {
	    rc = m_DiskView.QuerySubItemText(m_IndexDisk, Data, 256);
    }

    if (rc && OpenClipboard()) {

        HGLOBAL clipbuffer;
        char * buffer;
        clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(Data) + 1);
        if (clipbuffer) {
            buffer = (char*)GlobalLock(clipbuffer);
            if (buffer) {
                EmptyClipboard();
                memset(buffer, 0, strlen(Data) + 1);
                memcpy(buffer, Data, strlen(Data) + 1);
                GlobalUnlock(clipbuffer);
                SetClipboardData(CF_TEXT,clipbuffer);
           } else {
               GlobalFree(clipbuffer);
           }
        }
        CloseClipboard();
    }
}

void CExt2MgrDlg::OnCopyAll() 
{
    BOOL rc = FALSE;
    CString s;

    s = Ext2SysInformation();

    if (!s.IsEmpty() && OpenClipboard()) {

        HGLOBAL clipbuffer;
        char * buffer;
        clipbuffer = GlobalAlloc(GMEM_DDESHARE, s.GetLength() + 1);
        if (clipbuffer) {
            buffer = (char*)GlobalLock(clipbuffer);
            if (buffer) {
                EmptyClipboard();
                memcpy(buffer, (LPCSTR)s, s.GetLength() + 1);
                GlobalUnlock(clipbuffer);
                SetClipboardData(CF_TEXT,clipbuffer);
           } else {
               GlobalFree(clipbuffer);
           }
        }
        CloseClipboard();
    }
}

void CExt2MgrDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default

    if (nIDEvent == ID_INDICATOR_TIME) {
        CString s;
        CTime t1;
        t1 = CTime::GetCurrentTime();
        s.Format(" %3.3s", t1.Format("%B"));
        s += t1.Format(" %d,%Y %H:%M:%S");
        m_bar.SetPaneText(1, s);
    } else if (nIDEvent == 'REFR') {
        if (m_bHandleChange)
            PostMessage(WM_COMMAND, ID_REFRESH, 0);
        KillTimer('REFR');
    } else if (nIDEvent == 'REFF') {
        PostMessage(WM_COMMAND, ID_REFRESH, 0);
        KillTimer('REFF');
    }
	
	CDialog::OnTimer(nIDEvent);
}

PVOID CExt2MgrDlg::QuerySelectedItem(PBOOL bIsExt2)
{
    CString str = "Ready";
    PVOID   List = NULL;

    PEXT2_CDROM  Cdrom = NULL;
    PEXT2_DISK   Disk = NULL;
    PEXT2_VOLUME Volume = NULL;
    PEXT2_PARTITION Part = NULL;

    LVITEM lvItem;
    memset(&lvItem, 0, sizeof(LVITEM));
    lvItem.mask = LVIF_PARAM | LVIF_IMAGE;

    m_sdev = NULL;
    m_type = 0;

    CMenu* pMenu = AfxGetMainWnd()->GetMenu();
    CMenu* pTools = NULL;
    pTools = pMenu->GetSubMenu(1);

    if (bIsExt2) {
        *bIsExt2 = FALSE;
    }

    pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
    pTools->EnableMenuItem(ID_CHANGE, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
    pTools->EnableMenuItem(ID_FORMAT, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
    pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);

    if (m_bFocusVolume) {

        if (m_IndexVolume == -1 || 
            m_IndexVolume >= m_VolumeList.GetItemCount()) {
            goto errorout;
        }

        lvItem.iItem = m_IndexVolume;
        m_VolumeList.GetItem(&lvItem);

        PEXT2_VOLUME Volume = (PEXT2_VOLUME) lvItem.lParam;
        if (!Volume) {
            goto errorout;
        }
        Cdrom = (PEXT2_CDROM) Volume;
        m_type = Volume->Magic;

        if (Volume->Magic == EXT2_VOLUME_MAGIC) {
            str.Format("VOLUME: %s %s %s", 
                    Ext2QueryVolumeLetterStrings(
                                Volume->DrvLetters, NULL),
                    Volume->FileSystem, Volume->Name);
            m_sdev = (PVOID) Volume;

            if (Volume->bRecognized && (Volume->EVP.bExt2 || Volume->EVP.bExt3)) {
                pTools->EnableMenuItem(ID_CHANGE, MF_BYCOMMAND|MF_ENABLED);
            }
            pTools->EnableMenuItem(ID_FORMAT, MF_BYCOMMAND|MF_ENABLED);
            pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);

            if (bIsExt2 && Volume->bRecognized) {
                    *bIsExt2 = Volume->EVP.bExt2 || Volume->EVP.bExt3;
            }

        } else if (Cdrom->Magic[0] == EXT2_CDROM_DEVICE_MAGIC) {
            str.Format("CDROM %d: %s", Cdrom->OrderNo,
                            Ext2QueryVolumeLetterStrings(
                                Cdrom->DrvLetters, NULL));
            if (Cdrom->bIsDVD) {
                str += " DVD";
            }
            m_sdev = (PVOID) Cdrom;

            pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);

            if (bIsExt2) {
                *bIsExt2 = Cdrom->EVP.bExt2 || Cdrom->EVP.bExt3;
            }

        } else {
            m_IndexVolume = -1;
        }
    }

    if (m_bFocusDisk) {

        if (m_IndexDisk == -1 || 
            m_IndexDisk >= m_DiskView.GetItemCount()) {
            goto errorout;
        }

        lvItem.iItem = m_IndexDisk;
        m_DiskView.GetItem(&lvItem);

        PEXT2_DISK Disk = (PEXT2_DISK) lvItem.lParam;
        if (!Disk) {
            goto errorout;
        }

        m_type = Disk->Magic;

        if (Disk->Magic == EXT2_DISK_MAGIC) {
            str.Format("DISK %d: %s", Disk->OrderNo, Disk->Name);
            m_sdev = (PVOID) Disk;
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);
        } else if (Disk->Magic == EXT2_PART_MAGIC) {
            BOOL     bDynamic = FALSE;
            Part = (PEXT2_PARTITION) Disk;
            Disk = Part->Disk;
            Volume = Part->Volume;
            if (!Volume) {
                if (Disk->SDD.RemovableMedia) {
                    if (Disk->bEjected) {
                        str.Format("DISK %d: No media", Disk->OrderNo);
                    } else {
                        str.Format("DISK %d: RAW", Disk->OrderNo);
                    }
                } else {
                    if (Disk->Layout) {
                        str.Format("DISK %d: Not recognized", Disk->OrderNo);
                    } else {
                        str.Format("DISK %d: RAW", Disk->OrderNo);
                    }
                }
            } else {
                str.Format("DISK %d PARTITION %d: %s %s",
                            Disk->OrderNo, Part->Number,
                            Ext2QueryVolumeLetterStrings(
                                Part->DrvLetters, NULL),
                            Part->Volume->FileSystem
                            );
            }
            m_sdev = (PVOID) Part;
            if (Volume) {
                pTools->EnableMenuItem(ID_FORMAT, MF_BYCOMMAND|MF_ENABLED);
                if (Volume->bRecognized && (Volume->EVP.bExt2 || Volume->EVP.bExt3))
                    pTools->EnableMenuItem(ID_CHANGE, MF_BYCOMMAND|MF_ENABLED);
                if (!Volume->bDynamic) {
                    pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
                }
                if (bIsExt2 && Volume->bRecognized) {
                    *bIsExt2 = Volume->EVP.bExt2 || Volume->EVP.bExt3;
                }
            } else {
                if (Disk->SDD.RemovableMedia) {
                    pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
                }
            }
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);
        } else if (Disk->Magic == EXT2_DISK_NULL_MAGIC) {
            Disk = (PEXT2_DISK)((PUCHAR)Disk - sizeof(ULONG));
            str.Format("DISK %d: No media", Disk->OrderNo);
            m_sdev = (PVOID) Disk;
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);
        } else if (Disk->Magic == EXT2_CDROM_VOLUME_MAGIC) {
            Cdrom = (PEXT2_CDROM)((PUCHAR)Disk - sizeof(ULONG));
            str.Format("CDROM %d: %s", Cdrom->OrderNo,
                            Ext2QueryVolumeLetterStrings(
                                Cdrom->DrvLetters, NULL));
            if (Cdrom->bIsDVD) {
                str += " DVD";
            }
            m_sdev = (PVOID) Cdrom;
            pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);
        } else if (Disk->Magic == EXT2_CDROM_DEVICE_MAGIC){
            Cdrom = (PEXT2_CDROM)Disk;
            str.Format("CDROM %d: %s", Cdrom->OrderNo, Cdrom->Name);
            pTools->EnableMenuItem(ID_DRV_LETTER, MF_BYCOMMAND|MF_ENABLED);
            pTools->EnableMenuItem(ID_PROPERTY, MF_BYCOMMAND|MF_ENABLED);
            m_sdev = (PVOID) Cdrom;
        } else {
            m_IndexDisk = -1;
        }
    }

errorout:

    m_bar.SetPaneText(0, str);

    return m_sdev;   
}

void CExt2MgrDlg::OnKeyupVolumeList() 
{
    m_bFocusDisk = FALSE;
    m_bFocusVolume = TRUE;

    int item = m_VolumeList.GetSelectionMark();
    if (item != -1 && item != m_IndexVolume) {
        m_IndexVolume = item;
        QuerySelectedItem(NULL);
    }
}

void CExt2MgrDlg::OnKeyupDiskList() 
{
    int item = m_DiskView.GetSelectionMark();

    if (item != -1 &&  item != m_IndexDisk) {
        m_DiskView.SetSelectionMark(item);
        m_IndexDisk = item;
        QuerySelectedItem(NULL);
    }

    if (!m_bFocusDisk) {
        m_DiskView.Redraw();
    }

    m_bFocusDisk = TRUE;
    m_bFocusVolume = FALSE;
}

BOOL CExt2MgrDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class

    if (pMsg->message==WM_KEYDOWN) {
        if (pMsg->wParam == VK_ESCAPE) {
            pMsg->wParam = NULL;
            PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);           
        } else if (pMsg->wParam == VK_RETURN) {
            pMsg->wParam = NULL;
            PostMessage(WM_COMMAND, ID_PROPERTY, 0);            
        }
    } 

    if (pMsg->message==WM_SYSKEYDOWN) {
        if (pMsg->wParam == VK_RETURN) {
            pMsg->wParam = NULL;
            PostMessage(WM_COMMAND, ID_CHANGE, 0);            
        }
    }

    if (m_hAccel != NULL) {
        if (TranslateAccelerator(m_hWnd, m_hAccel, pMsg)) {
            return TRUE; 
        }
    }   

    if (pMsg->message == WM_KEYUP) {

        if (GetFocus() == (CWnd *)&m_DiskView) {
            OnKeyupDiskList();
        }

        if (GetFocus() == (CWnd *)&m_VolumeList) {
            OnKeyupVolumeList();
        }
    }

	return CDialog::PreTranslateMessage(pMsg);
}

void CExt2MgrDlg::OnShowMain() 
{
    // TODO: Add your command handler code here
    m_bHide = FALSE;
    ShowWindow(SW_SHOW);	
}

LRESULT CExt2MgrDlg::OnTerminate(WPARAM wParam,LPARAM lParam)
{
	if (lParam == 0x1234) {
        EndDialog(0);
	}

    return TRUE;
}

LRESULT CExt2MgrDlg::OnMountPointNotify(WPARAM wParam,LPARAM lParam)
{
    if (wParam == 'DA') {
       DriverLetterChangeNotify((CHAR)lParam, TRUE);
    } else if (wParam == 'DR'){
       DriverLetterChangeNotify((CHAR)lParam, FALSE);
    }

    return TRUE;
}

void CExt2MgrDlg::OnHelp() 
{
	// TODO: Add your command handler code here
    CHAR	szFullPathName	[MAX_PATH];
    CHAR	szDrive			[MAX_PATH];
    CHAR	szDir			[MAX_PATH];

    GetModuleFileName(NULL, szFullPathName, MAX_PATH);
	_splitpath(szFullPathName, szDrive, szDir, NULL, NULL); 
	sprintf(szFullPathName, "%s%sDocuments\\FAQ.txt", szDrive, szDir);

    ShellExecute(this->GetSafeHwnd(), "open", szFullPathName, NULL, NULL, SW_SHOW);
}

void CExt2MgrDlg::OnInstallService()
{
    Ext2SetManagerAsService(TRUE);
} 

void CExt2MgrDlg::OnRemoveService()
{
    Ext2SetManagerAsService(FALSE);
} 

void CExt2MgrDlg::OnEnableAutorun()
{
    Ext2SetAppAutorun(TRUE);
    // Ext2SetManagerAsService(FALSE);

    CMenu* pMenu = AfxGetMainWnd()->GetMenu();
    CMenu* pSubFile = pMenu->GetSubMenu(0);
    if (pSubFile) {
        if (Ext2RunMgrForCurrentUser()) {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
        } else {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
        }
    }

} 

void CExt2MgrDlg::OnDisableAutorun()
{
    Ext2SetAppAutorun(FALSE);

    CMenu* pMenu = AfxGetMainWnd()->GetMenu();
    CMenu* pSubFile = pMenu->GetSubMenu(0);
    if (pSubFile) {
        if (Ext2RunMgrForCurrentUser()) {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
        } else {
            pSubFile->EnableMenuItem(ID_ENABLE_AUTOSTART, MF_BYCOMMAND|MF_ENABLED);
            pSubFile->EnableMenuItem(ID_DISABLE_AUTOSTART, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
        }
    }
} 

void CExt2MgrDlg::OnPerfStat() 
{
    if (!m_PerfDlg) {
        m_PerfDlg = new CPerfStatDlg;
        if (m_PerfDlg) {
            m_PerfDlg->Create(IDD_PERFSTAT_DIALOG,  this);
        }
    }

    if (m_PerfDlg) {
        m_PerfDlg->ShowWindow(SW_SHOW);
        m_PerfDlg->SetForegroundWindow();
    }
}

void CExt2MgrDlg::OnPerfStop() 
{
    if (m_PerfDlg) {
        Sleep(100);
        delete m_PerfDlg;
        m_PerfDlg = NULL;
    }
}


void
CExt2MgrDlg::RegisterDeviceInterface(
    GUID            InterfaceClassGuid,
    PHDEVNOTIFY     hDevNotify
    )
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory (&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size =
        sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = InterfaceClassGuid; 

    *hDevNotify = RegisterDeviceNotification(
        m_hWnd,
        &NotificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE
    );
}


void CExt2MgrDlg::OnFlush() 
{
    PEXT2_VOLUME   volume = NULL;
    PEXT2_PARTITION part = NULL;

    if (m_bFocusVolume) {
        if (m_type == EXT2_VOLUME_MAGIC) {
            volume = (PEXT2_VOLUME) m_sdev;
        }
    } else {
        if (m_type == EXT2_PART_MAGIC) {
            part = (PEXT2_PARTITION) m_sdev;
            volume = part->Volume;
        }
    }

    if (volume) {
        Ext2FlushVolume(volume->Name);
    } 
}

void CExt2MgrDlg::OnPartType() 
{
    CPartitionType  PartType;
    PEXT2_VOLUME   volume = NULL;
    PEXT2_PARTITION part = NULL;
    CHAR  devPath[MAX_PATH];

    if (m_bFocusVolume) {
        if (m_type == EXT2_VOLUME_MAGIC) {
            volume = (PEXT2_VOLUME) m_sdev;
        }
    } else {
        if (m_type == EXT2_PART_MAGIC) {
            part = (PEXT2_PARTITION) m_sdev;
        }
    }

    if (volume) {
        part = volume->Part;
        if (!part) {
            return;
        }
        strcpy(devPath, volume->Name);
    } else if (part) {
        sprintf(devPath, "\\Device\\Harddisk%u\\Partition%u",
                    part->Disk->OrderNo, part->Number);
    }

    if (part && part->Entry && 
        part->Entry->PartitionStyle == PARTITION_STYLE_MBR) {
        PartType.m_Part = part;
        PartType.m_sDevice = devPath;
        PartType.DoModal();

        /* update new partition type */
        if (PartType.m_cPartType) {

            for (int i=0; i < m_DiskView.GetItemCount(); i++) {
                if ((ULONG)part == m_DiskView.GetItemData(i)) {
                    Ext2RefreshDVPT(&m_DiskView, part, i);
                    break;
                }
            }
        }
    }
}   

void CExt2MgrDlg::OnRemoveDeadLetter() 
{
    CDelDeadLetter DelDeadLetter;
    DelDeadLetter.DoModal();
}
