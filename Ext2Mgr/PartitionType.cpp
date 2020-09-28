// PartitionType.cpp : implementation file
//

#include "stdafx.h"
#include "ext2mgr.h"
#include "PartitionType.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPartitionType dialog


CPartitionType::CPartitionType(CWnd* pParent /*=NULL*/)
	: CDialog(CPartitionType::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPartitionType)
		// NOTE: the ClassWizard will add member initialization here
    m_Part = NULL;
    m_cPartType = 0;
    m_sDevice = _T("");
	m_sPartType = _T("");
	//}}AFX_DATA_INIT
}


void CPartitionType::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPartitionType)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPartitionType, CDialog)
	//{{AFX_MSG_MAP(CPartitionType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPartitionType message handlers

void CPartitionType::OnOK() 
{
    CString str;
    CComboBox *cbList = (CComboBox *)GetDlgItem(IDC_PARTTION_TYPE_LIST);
    m_cPartType = (UCHAR) cbList->GetCurSel();
    if (m_cPartType != m_Part->Entry->Mbr.PartitionType) {
        if (Ext2SetPartitionType(m_Part, m_cPartType)) {
            str.Format("Succeed to set partition type to %2.2X: %s",
                        m_cPartType, PartitionString(m_cPartType));
            AfxMessageBox(str, MB_OK | MB_ICONINFORMATION);
        } else {
            AfxMessageBox("Failed to set the partition type!",
                           MB_OK | MB_ICONWARNING);
            m_cPartType = 0;
            return;
        }
    } else {
        AfxMessageBox("Same partition type to the previous. Nothing is changed !",
                      MB_OK | MB_ICONWARNING);
    }

    CDialog::OnOK();
}

BOOL CPartitionType::OnInitDialog() 
{
    CString str, type;
	CDialog::OnInitDialog();

    SET_TEXT(IDC_PARTITION_NAME, m_sDevice);

	CComboBox   *cbList = (CComboBox *)GetDlgItem(IDC_PARTTION_TYPE_LIST);
    for (unsigned int i=0;  i < 0x100; i++) {
        type = PartitionString(i);
        str.Format("%2.2X ", i);
        if (type.CompareNoCase("UNKNOWN")) {
            str += type;
        }
        cbList->AddString(str);
    }
	
    m_cPartType = m_Part->Entry->Mbr.PartitionType;
    cbList->SetCurSel((int) m_cPartType);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
