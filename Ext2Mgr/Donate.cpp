// Donate.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "Donate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDonate dialog


CDonate::CDonate(CWnd* pParent /*=NULL*/)
	: CDialog(CDonate::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDonate)
	//}}AFX_DATA_INIT
}


void CDonate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDonate)
	DDX_Control(pDX, IDC_VIA_SOURCEFORGE, m_SF);
	DDX_Control(pDX, IDC_VIA_PAYPAL, m_Paypal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDonate, CDialog)
	//{{AFX_MSG_MAP(CDonate)
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDonate message handlers

BOOL CDonate::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    //Set the target URL 
    m_SF.SetLinkUrl("http://sourceforge.net/project/project_donations.php?group_id=43775");
    //Enable showing the Tooltip
    m_SF.ActiveToolTip(TRUE);
    //Set the Tooltiptext
    m_SF.SetTootTipText("Donate to Ext2Fsd group via SourceForge.");


    //Set the target URL 
    m_Paypal.SetLinkUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=m@ext2fsd.com&item_name=Donation&return=https://sourceforge.net/projects/ext2fsd");
    //Enable showing the Tooltip
    m_Paypal.ActiveToolTip(TRUE);
    //Set the Tooltiptext
    m_Paypal.SetTootTipText("Donate to Ext2Fsd group via Paypal.");

#if 0
    CHAR Declaration [] =
    "\r\nExt2Fsd is an open source software. It acts as a bridge between Windows and Linux, making life easier to access Linux partitions under Windows systems.\r\n"
    "\r\nCurrently there are still lots of jobs left to make a fully functional file system driver, such as complete ext3 support, Linux LVM, Windows Vista, Longhorn. Especially Vista and Longhorn will need driver signing.\r\n"
    "\r\nNow I'm the only part-time developer. It needs about two full time developers to implement that job and also it needs some hardwares such SMP system, SCSI array for debugging and testing.\r\n"
    "\r\nI'll try my best to make it out. I'm dreaming of that day.\r\n"
    "\r\nAny help will be highly appreciated. Thanks and best wishes.\r\n"
    "\r\n\r\nYours sincerely,\r\n"
    "Matt";
#endif

    CString Declaration;
    Declaration.LoadString(IDS_DONATE_DECLARE);

    GetDlgItem(IDC_DECLARE)->SetWindowText(Declaration);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDonate::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDonate::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);

}
