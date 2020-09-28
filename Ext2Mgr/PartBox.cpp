// PartBox.cpp : implementation file
//

#include "stdafx.h"
#include "PartBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPartBox

CPartBox::CPartBox()
{
    m_nID = IDC_PROPERTY_SDEV;
    m_nLeft = 10;
    m_nSize = 80;
}

CPartBox::~CPartBox()
{
}

BEGIN_MESSAGE_MAP(CPartBox, CButton)
	//{{AFX_MSG_MAP(CPartBox)
	ON_WM_SETFOCUS()
	ON_CBN_SELCHANGE(IDC_PROPERTY_SDEV, OnSelectChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPartBox message handlers

void CPartBox::OnSelectChanged()
{
    GetParent()->SendMessage(WM_GROUP_BOX_UPDATED, 'GB', 'PVLU');
}

void CPartBox::PreSubclassWindow() 
{
    //
	// Make sure that this control has the BS_ICON style set.
	// If not, it behaves very strangely:
	//  It erases itself if the user TABs to controls in the dialog,
	//  unless the user first clicks it.  Very strange!!
    //

	ModifyStyle(0, BS_ICON|WS_TABSTOP|WS_GROUP);

	CString	strTitle;
	GetWindowText(strTitle);

	int nWidth = AssemblingTitle();

	CRect	r;
	GetWindowRect(&r);
	ScreenToClient(r);

	r.OffsetRect(m_nLeft, 0);
	r.bottom = r.top + m_nSize;
	r.right = r.left + m_nSize + nWidth;

    m_ComboBox.Create(WS_CHILD | CBS_DROPDOWN | WS_VSCROLL |
                      CBS_NOINTEGRALHEIGHT | CBS_DROPDOWNLIST,
                      r, this, m_nID);
	m_ComboBox.SetFont(GetFont(), true);
	m_ComboBox.ShowWindow(SW_SHOW);

    SetListboxHeight(m_ComboBox.m_hWnd);
}

int CPartBox::AssemblingTitle()
{
    //
	// The group box title needs to be erased, but I need to keep
    // the border away from the check box text. I create a string
    // of spaces (' ') that is the same length as the title was
    // plus the size of the checkbox. plus a little more.
    //

	CString	strOldTitle, strNewTitle;
	GetWindowText(strOldTitle);

	CClientDC dc(this);
	CFont*	  pOldFont = dc.SelectObject(GetFont());

	CSize	czText	= dc.GetTextExtent(strOldTitle);
	int		nRet	= czText.cx;
	int		nTarget = czText.cx + m_nSize;
	
	while(czText.cx < nTarget)
	{
		strNewTitle.Insert(0, ' ');
		czText = dc.GetTextExtent(strNewTitle);
	}

	dc.SelectObject(pOldFont);

	SetWindowText(strNewTitle);
	return nRet;
}

void CPartBox::OnSetFocus(CWnd* pOldWnd) 
{
	CButton::OnSetFocus(pOldWnd);
	m_ComboBox.SetFocus();
}


void CPartBox::SetListboxHeight(HWND hWnd)
{   
    RECT   rc;   

    ::SendMessage(hWnd, LB_GETITEMRECT, 0, (LPARAM)&rc);   
    ::GetClientRect(hWnd, &rc);   
    int   cyClient= rc.bottom - rc.top;   

    ::GetWindowRect(hWnd, &rc);   
    int   cxListbox = rc.right - rc.left;   
    int   cyListbox = rc.bottom - rc.top;   

    cyListbox = 120;
    ::SetWindowPos(hWnd, NULL, 0, 0,     
                   cxListbox, cyListbox,
                   SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOCOPYBITS|
                   SWP_NOOWNERZORDER|SWP_NOZORDER
        );
}
