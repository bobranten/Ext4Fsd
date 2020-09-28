// PerfStatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "PerfStatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerfStatDlg dialog


CPerfStatDlg::CPerfStatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPerfStatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPerfStatDlg)
	m_Interval = 30;
    m_Handle = 0;
	//}}AFX_DATA_INIT
	m_IrpList = NULL;
	m_MemList = NULL;
}

void CPerfStatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPerfStatDlg)
	DDX_Text(pDX, IDC_PERFSTAT_INTERVAL, m_Interval);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPerfStatDlg, CDialog)
	//{{AFX_MSG_MAP(CPerfStatDlg)
	ON_WM_TIMER()
	ON_WM_DESTROY()
    ON_COMMAND(ID_QUERYPERF, OnQueryPerf)
	ON_EN_CHANGE(IDC_PERFSTAT_INTERVAL, OnChangePerfstatInterval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerfStatDlg message handlers

BOOL CPerfStatDlg::OnInitDialog() 
{
    LONG_PTR dwStyle = 0;
    int     i;
    CString s;

	CDialog::OnInitDialog();


 	m_IrpList = (CListCtrl *)GetDlgItem(IDC_IRP_LIST);
	m_MemList = (CListCtrl *)GetDlgItem(IDC_MEMORY_LIST);;

    if (m_IrpList == NULL || m_MemList == NULL) {
        return FALSE;
    }

    /* initialize the memory list */
    dwStyle=GetWindowLongPtr(m_MemList->GetSafeHwnd(), GWL_STYLE);
	dwStyle&=~LVS_TYPEMASK;
	dwStyle|= (LVS_REPORT | LVS_AUTOARRANGE);
	SetWindowLongPtr(m_MemList->GetSafeHwnd(),GWL_STYLE,dwStyle);
	m_MemList->SetExtendedStyle(LVS_EX_GRIDLINES);
    ListView_SetExtendedListViewStyleEx ( 
        m_MemList->GetSafeHwnd(), 
        LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

    m_MemList->DeleteAllItems();

    s.LoadString(IDS_PERFSTAT_NAME);
    m_MemList->InsertColumn(0, (LPCSTR)s, LVCFMT_RIGHT, 100);

    s.LoadString(IDS_PERFSTAT_UNIT);
    m_MemList->InsertColumn(1, (LPCSTR)s, LVCFMT_RIGHT, 40);

    s.LoadString(IDS_PERFSTAT_CURRENT);
    m_MemList->InsertColumn(2, (LPCSTR)s, LVCFMT_RIGHT, 56);

    s.LoadString(IDS_PERFSTAT_SIZE);
    m_MemList->InsertColumn(3, (LPCSTR)s, LVCFMT_RIGHT, 80);

    s.LoadString(IDS_PERFSTAT_TOTAL);
    m_MemList->InsertColumn(4, (LPCSTR)s, LVCFMT_RIGHT, 120);
    for (i = 0; PerfStatStrings[i] != NULL ; i++) {
        m_MemList->InsertItem(i, PerfStatStrings[i]);
    } 

    /* initialize the irp list */
    dwStyle=GetWindowLongPtr(m_IrpList->GetSafeHwnd(), GWL_STYLE);
	dwStyle&=~LVS_TYPEMASK;
	dwStyle|= (LVS_REPORT | LVS_AUTOARRANGE);
	SetWindowLongPtr(m_IrpList->GetSafeHwnd(),GWL_STYLE,dwStyle);
	m_IrpList->SetExtendedStyle(LVS_EX_GRIDLINES);
    ListView_SetExtendedListViewStyleEx ( 
        m_IrpList->GetSafeHwnd(), 
        LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

    m_IrpList->DeleteAllItems();

    s.LoadString(IDS_PERFSTAT_NAME);
    m_IrpList->InsertColumn(0, (LPCSTR)s, LVCFMT_RIGHT, 180);

    s.LoadString(IDS_PERFSTAT_PROCESSING);
    m_IrpList->InsertColumn(1, (LPCSTR)s, LVCFMT_RIGHT, 110);

    s.LoadString(IDS_PERFSTAT_PROCESSED);
    m_IrpList->InsertColumn(2, (LPCSTR)s, LVCFMT_RIGHT, 100);
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        m_IrpList->InsertItem(i, IrpMjStrings[i]);
    } 

    RefreshPerfStat();

    SetTimer('STAT', m_Interval * 1000, NULL);
    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPerfStatDlg::RefreshPerfStat()
{
    int i;
    CString s;

    PEXT2_PERF_STATISTICS_V1   PerfV1 = NULL;
    PEXT2_PERF_STATISTICS_V2   PerfV2 = NULL;

    NT::NTSTATUS status;

    if (m_Handle == 0) {
        status = Ext2Open("\\DosDevices\\Ext2Fsd", &m_Handle,
                          EXT2_DESIRED_ACCESS );
        if (!NT_SUCCESS(status)) {
            goto errorout;
        }
    }

    if (Ext2QueryPerfStat(m_Handle, &m_PerfStat, &PerfV1, &PerfV2)) {

        if (PerfV2) {

            if (m_MemList) {
                for (i = 0; i < PS_MAX_TYPE_V2; i++) {
                    if (PerfV2->Unit.Slot[i] == 0)
                        break;
                    else if (PerfV2->Unit.Slot[i] == 1)
                        s = "*";
                    else
                        s.Format("%u", PerfV2->Unit.Slot[i]);
                    m_MemList->SetItem(i, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
      
                    s.Format("%u", PerfV2->Current.Slot[i]);
                    m_MemList->SetItem(i, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);

                    s.Format("%u", PerfV2->Size.Slot[i]);
                    m_MemList->SetItem(i, 3, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);

                    s.Format("%u", PerfV2->Total.Slot[i]);
                    m_MemList->SetItem(i, 4, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                }
            }

            if (m_IrpList) {
                for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
                    s.Format("%u", PerfV2->Irps[i].Current);
                    m_IrpList->SetItem(i, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                    s.Format("%u", PerfV2->Irps[i].Processed);
                    m_IrpList->SetItem(i, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                }
            }

        } else if (PerfV1) {

            if (m_MemList) {
                for (i = 0; i < PS_MAX_TYPE_V1; i++) {

                    if (PerfV1->Unit.Slot[i] == 0)
                        s = "*";
                    else
                        s.Format("%u", PerfV1->Unit.Slot[i]);
                    m_MemList->SetItem(i, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
      
                    s.Format("%u", PerfV1->Current.Slot[i]);
                    m_MemList->SetItem(i, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);

                    s.Format("%u", PerfV1->Size.Slot[i]);
                    m_MemList->SetItem(i, 3, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);

                    s.Format("%u", PerfV1->Total.Slot[i]);
                    m_MemList->SetItem(i, 4, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                }
            }

            if (m_IrpList) {
                for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
                    s.Format("%u", PerfV1->Irps[i].Current);
                    m_IrpList->SetItem(i, 1, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                    s.Format("%u", PerfV1->Irps[i].Processed);
                    m_IrpList->SetItem(i, 2, LVIF_TEXT, (LPCSTR)s, 0, 0, 0, 0);
                }
            }
        }

    } else {
        Ext2Close(&m_Handle);
    }

errorout:

    return;
}

void CPerfStatDlg::OnTimer(UINT_PTR nIDEvent) 
{
    RefreshPerfStat();
	
	CDialog::OnTimer(nIDEvent);
}

void CPerfStatDlg::OnDestroy() 
{
	KillTimer('STAT');
    Ext2Close(&m_Handle);
	CDialog::OnDestroy();
}

void CPerfStatDlg::OnChangePerfstatInterval() 
{
    UpdateData(TRUE);

    if (m_Interval == 0) {
        m_Interval = 1;
    }
    SetTimer('STAT', m_Interval * 1000, NULL);
}

void CPerfStatDlg::OnOK() 
{
	// TODO: Add extra validation here
	KillTimer('STAT');
    Ext2Close(&m_Handle);
    GetParent()->PostMessage(WM_COMMAND, ID_PERFSTOP, 0);
	CDialog::OnOK();
}

void CPerfStatDlg::OnCancel() 
{
	// TODO: Add extra validation here
	KillTimer('STAT');
    Ext2Close(&m_Handle);
    GetParent()->PostMessage(WM_COMMAND, ID_PERFSTOP, 0);
	CDialog::OnCancel();
}

void CPerfStatDlg::OnQueryPerf() 
{
    RefreshPerfStat();
}
