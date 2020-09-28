// MyHyperLink.cpp : implementation file
//

// Written By : Renjith.R
// Email : renji12renji@m2comsys.com
// Details :Derived from MFC CStatic 
// Date :Nov 25 2002

#include "stdafx.h"
#include "HyperLink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMyHyperLink

CMyHyperLink::CMyHyperLink()
{
	m_sLinkColor = RGB(0, 0 ,255);
	m_sHoverColor = RGB(255, 0, 0);
	m_sVisitedColor = RGB(5, 34, 143);

	m_bFireChild = false;
	m_bMouseOver = false;
	m_bEnableToolTip = false;
	m_bVisited =  false;
	
	//Create Tooltip
}

CMyHyperLink::~CMyHyperLink()
{
}


BEGIN_MESSAGE_MAP(CMyHyperLink, CStatic)
	//{{AFX_MSG_MAP(CMyHyperLink)
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)


	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyHyperLink message handlers

//Sets the Link Color
void CMyHyperLink::SetLinkColor(COLORREF sLinkColor)
{
	m_sLinkColor = sLinkColor;

}

//open the URL by Windows ShellExecute()
bool CMyHyperLink::GoToLinkUrl(CString csLink)
{

	HINSTANCE hInstance = (HINSTANCE)ShellExecute(NULL, _T("open"), csLink.operator LPCTSTR(), NULL, NULL, 2);

	if ((UINT_PTR)hInstance < HINSTANCE_ERROR){
		return false;
	}else
		return true;
}

//User can Active/Inactive the Tooltip already they set 
void CMyHyperLink::ActiveToolTip(int nFlag)
{
	if (nFlag)
		m_bEnableToolTip = true;
	else
		m_bEnableToolTip = false;
}

//change The Tooltip text
void CMyHyperLink::SetTootTipText(LPCSTR szToolTip)
{
	if (m_bEnableToolTip )
	{
		m_ToolTip.UpdateTipText(szToolTip,this,1001);
	}

}

//The Mouse Move Message
void CMyHyperLink::OnMouseMove(UINT nFlags, CPoint point) 
{
	CStatic::OnMouseMove(nFlags, point);
	if (m_bMouseOver)
	{
		CRect oRect;
		GetClientRect(&oRect);

		//check if the mouse is in the rect
		if (oRect.PtInRect(point) == false)
		{
			m_bMouseOver = false;
			//Release the Mouse capture previously take
			ReleaseCapture();
			RedrawWindow();
			return;
		}
	}else
	{
		m_bMouseOver = true;
		RedrawWindow();
		//capture the mouse
		SetCapture();
	}
}

//before Subclassing 
void CMyHyperLink::PreSubclassWindow() 
{

	//Enable the Static to send the Window Messages To its parent
	DWORD dwStyle = GetStyle();
	SetWindowLongPtr(GetSafeHwnd() ,GWL_STYLE ,dwStyle | SS_NOTIFY);

	char szCurretText[MAX_PATH];
	GetWindowText(szCurretText, MAX_PATH);
	if ((szCurretText) == NULL){
		SetWindowText(m_csLinkText.operator LPCTSTR());
	}
	
	LOGFONT sLogFont;
	GetFont()->GetLogFont(&sLogFont);
	//Set the Link UnderLined
	sLogFont.lfUnderline = true;
	//Set the Font to  the Control
	m_oTextFont.CreateFontIndirect(&sLogFont);
	this->SetFont(&m_oTextFont, true);
	
	//Adjust the window
	//IsValidURL();

	//Set the Cursor Hand
	//WinHlp32.exe in windows folder ResourceID 106
	//is a standard window HAND cursor 
	//courtesy www.codeguru.com
	//you can use a custom Hand cursor resourse also
	// i added that  as a resourse in this project with 
	// ID - IDC_CURSOR_HAND

	char szWindowsDir[MAX_PATH*2];
	GetWindowsDirectory(szWindowsDir ,MAX_PATH*2);
	strcat(szWindowsDir,"\\Winhlp32.exe");
	HMODULE hModule = LoadLibrary(szWindowsDir);
	
	if (hModule){
		m_hHyperCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
	}

	this->SetCursor(m_hHyperCursor);

	//free the module
	if (hModule)
		FreeLibrary(hModule);

	CStatic::PreSubclassWindow();
	this->SetCursor(m_hHyperCursor);
	
	m_ToolTip.Create(this,TTS_ALWAYSTIP);
	CRect oRect;
	GetClientRect(&oRect);
	m_ToolTip.AddTool(this,"",oRect,1001);
	m_ToolTip.ShowWindow(SW_HIDE);
}

void CMyHyperLink::SetLinkText(CString csLinkText)
{
	m_csLinkText = csLinkText;
	this->SetWindowText(csLinkText.operator LPCTSTR());

}

BOOL CMyHyperLink::PreTranslateMessage(MSG* pMsg) 
{
	m_ToolTip.RelayEvent(pMsg);
	return CStatic::PreTranslateMessage(pMsg);
}


BOOL CMyHyperLink::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{

	::SetCursor(m_hHyperCursor);
	return true;
	//return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

//////////////////EVENT WILL GET HERE //////////////////////


void CMyHyperLink::OnClicked() 
{
	if (m_bFireChild){
		//Fire the Event to Parent Window
		CWnd *pParent;
		pParent = GetParent();
		int nCtrlID = GetDlgCtrlID();
		::SendMessage(pParent->m_hWnd, _HYPERLINK_EVENT, (WPARAM)nCtrlID, 0);
		//::PostMessage(pParent->m_hWnd, __EVENT_ID_, (WPARAM)nCtrlID, 0);

	}else
	{
		GoToLinkUrl(m_csUrl);
	}

	m_bVisited = true;
	//reddraw the control 
	this->Invalidate(true);
}

HBRUSH CMyHyperLink::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	if (m_bMouseOver){
		if (m_bVisited)
			pDC->SetTextColor(m_sVisitedColor);
		else
			pDC->SetTextColor(m_sHoverColor);
	}else {
		if (m_bVisited)
			pDC->SetTextColor(m_sVisitedColor);
		else
			pDC->SetTextColor(m_sLinkColor);
	}
	pDC->SetBkMode(TRANSPARENT);
	return((HBRUSH)GetStockObject(NULL_BRUSH));
}

void CMyHyperLink::SetToolTipTextColor(COLORREF sToolTipText) {
	m_ToolTip.SetTipTextColor(sToolTipText);
}

void CMyHyperLink::SetToolTipBgColor(COLORREF sToolTipBgColor)
{
	m_ToolTip.SetTipBkColor(sToolTipBgColor);

}

CString CMyHyperLink::GetLinkText()  {
	if (m_csLinkText.IsEmpty())
		return CString("");
	return m_csLinkText;
}

void CMyHyperLink::SetLinkUrl(CString csUrl) {
	m_csUrl= csUrl;
}

CString CMyHyperLink::GetLinkUrl() {
	return m_csUrl;
}

void CMyHyperLink::SetVisitedColor(COLORREF sVisitedColor) {
	m_sVisitedColor = sVisitedColor ;
}

void CMyHyperLink::SetHoverColor(COLORREF cHoverColor) {
	m_sHoverColor = cHoverColor;
}

void CMyHyperLink::SetFireChild(int nFlag) {
	if (nFlag)
		m_bFireChild = true;
	else
		m_bFireChild = false;
}

BOOL CMyHyperLink::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pMsgHdr;
	pMsgHdr = (NMHDR*) lParam;

	switch (pMsgHdr->code){
	case NM_RCLICK:
		break;
	default:
	;
	}
	
	return CStatic::OnNotify(wParam, lParam, pResult);
}
