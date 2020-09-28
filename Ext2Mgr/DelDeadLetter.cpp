// DelDeadLetter.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "DelDeadLetter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDelDeadLetter dialog


CDelDeadLetter::CDelDeadLetter(CWnd* pParent /*=NULL*/)
	: CDialog(CDelDeadLetter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDelDeadLetter)
	m_sDrvLetter = _T("");
	m_bAutoRemoval = g_bAutoRemoveDeadLetters;
	m_bKeepIt = TRUE;
	//}}AFX_DATA_INIT
}


void CDelDeadLetter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDelDeadLetter)
    DDX_CBString(pDX, IDC_DEAD_LETTER_LIST, m_sDrvLetter);
	DDX_Check(pDX, IDC_AUTO_REMOVAL, m_bAutoRemoval);
	DDX_Check(pDX, IDC_REMOVAL_CURRENT, m_bKeepIt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDelDeadLetter, CDialog)
	//{{AFX_MSG_MAP(CDelDeadLetter)
	ON_BN_CLICKED(ID_RELOAD_DL, OnReloadDl)
	ON_BN_CLICKED(IDC_AUTO_REMOVAL, OnAutoRemoval)
	ON_BN_CLICKED(IDC_AUTOREMOVALTEXT, OnAutoremovaltext)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDelDeadLetter message handlers

void CDelDeadLetter::OnOK() 
{
    CHAR            drvChar;
    PEXT2_LETTER    drvLetter = NULL;
    UpdateData(TRUE);

    drvChar = m_sDrvLetter.GetAt(0);

    if ((drvChar >= '0') && (drvChar <= '9')) {
        drvLetter = &drvDigits[drvChar - '0'];
    }

    if ((drvChar >= 'A') && (drvChar <= 'Z')) {
        drvLetter = &drvLetters[drvChar - 'A'];
    }

    if (drvLetter) {
        if (AfxMessageBox("Warning: the driver letter might be still used. Are you\r\n"
                          "         sure that make it's a real dead driver letter ?", MB_YESNO) == IDYES) {

            Ext2RemoveMountPoint(drvLetter, !m_bKeepIt);
            Ext2RemoveDosSymLink(drvLetter->Letter);
            UpdateDeadLetterList();
        }
    }
}

BOOL CDelDeadLetter::OnInitDialog() 
{
	CDialog::OnInitDialog();

    UpdateDeadLetterList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

VOID
CDelDeadLetter::UpdateDeadLetterList()
{
    ULONGLONG   LetterMask = -1;
    DWORD       i, j;
    PEXT2_VOLUME    volume;
    PEXT2_DISK      disk;
    PEXT2_PARTITION part;
    PEXT2_CDROM     cdrom;
    CString         str;

    CComboBox *cbList = (CComboBox *)GetDlgItem(IDC_DEAD_LETTER_LIST);

    cbList->ResetContent();

    for (volume = gVols; volume != NULL; volume = volume->Next) {
        LetterMask &= ~(volume->DrvLetters);
    }

    for (i = 0; i < g_nDisks; i++) {
        disk = &gDisks[i];
        if (disk->DataParts == NULL) {
            continue;
        }
        for (j=0; j < disk->NumParts; j++) {
            part = &disk->DataParts[j];
            if (part) {
                 LetterMask &= ~(part->DrvLetters);
            }
        }
    }

    for (i = 0; i < g_nCdroms; i++) {
        cdrom = &gCdroms[i];
        LetterMask &= ~(cdrom->DrvLetters);
    }

    for (i=0; i < 10; i++) {
        if (drvDigits[i].bUsed && (drvDigits[i].Extent == NULL) &&
            (LetterMask & (((ULONGLONG) 1) << (i + 32)) ) ) {
            str.Format("%c: ", drvDigits[i].Letter);
            str += drvDigits[i].SymLink;
            cbList->AddString(str);
        }
    }

    for (i=0; i <26; i++) {
        if (drvLetters[i].bUsed && (drvLetters[i].Extent == NULL) &&
            (LetterMask & (((ULONGLONG) 1) << i)) ) {
            str.Format("%c: ", drvLetters[i].Letter);
            str += drvLetters[i].SymLink;
            cbList->AddString(str);
        }
    }

#if 0
    if (cbList->GetCount() == 0) {
        AfxMessageBox("No dead driver letters exist :)", MB_OK|MB_ICONINFORMATION);
        EndDialog(TRUE);
    }
#endif

    cbList->SetCurSel(0);
}

void CDelDeadLetter::OnReloadDl() 
{
	// TODO: Add your control notification handler code here
	UpdateDeadLetterList();
}

void CDelDeadLetter::OnAutoRemoval() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
    g_bAutoRemoveDeadLetters = m_bAutoRemoval;	
}

void CDelDeadLetter::OnAutoremovaltext() 
{
}
