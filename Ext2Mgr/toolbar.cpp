// ToolBar1.cpp : implementation file
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "afxpriv.h"
#include "resource.h"
#include "toolbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStandardBar

CStandardBar::CStandardBar() : m_pTBButtons(NULL)
{

}

CStandardBar::~CStandardBar()
{
	if (m_pTBButtons)
		delete []m_pTBButtons;

}


BEGIN_MESSAGE_MAP(CStandardBar, CToolBarCtrl)
	//{{AFX_MSG_MAP(CStandardBar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CStandardBar::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, int count, PBUTTON_INFO pButton)
{
	BOOL bRet = CToolBarCtrl::Create(dwStyle, rect, pParentWnd, nID);

	AddBitmap(count, IDR_STANDARDBAR);

	m_pTBButtons = new TBBUTTON[count];
    memset(m_pTBButtons, 0, sizeof(TBBUTTON) * count);

    m_nButtonCount = count;

	TBBUTTON sepButton;
	sepButton.idCommand = 0;
	sepButton.fsStyle = TBSTYLE_SEP;
	sepButton.fsState = TBSTATE_ENABLED;
	sepButton.iString = 0;
	sepButton.iBitmap = 0;
	sepButton.dwData = 0;

	for (int nIndex = 0; nIndex < count; nIndex++) {
		CString string;
		string.LoadString(pButton[nIndex].ID);

		// Add second '\0'
		int nStringLength = string.GetLength() + 1;
		TCHAR * pString = string.GetBufferSetLength(nStringLength);
		pString[nStringLength] = 0;

		m_pTBButtons[nIndex].iString = AddStrings(pString);

		string.ReleaseBuffer();


		m_pTBButtons[nIndex].fsState = TBSTATE_ENABLED;
		m_pTBButtons[nIndex].fsStyle = TBSTYLE_BUTTON;
		m_pTBButtons[nIndex].dwData = 0;
		m_pTBButtons[nIndex].iBitmap = nIndex;
		m_pTBButtons[nIndex].idCommand = pButton[nIndex].ID;

        if (pButton[nIndex].bSep) {
            AddButtons(1,&sepButton);
            AddButtons(1,&sepButton);
        }

        AddButtons(1, &m_pTBButtons[nIndex]);
	}

	return bRet;
}


/////////////////////////////////////////////////////////////////////////////
// CStandardBar message handlers

// MFC routes the notifications sent to the parent of the control to
// the control before the parent can process the notification.
// The control object can handle the notification or ignore it.
// If the notification is handled then return TRUE. Otherwise MFC
// will route it to the parent of the control.

BOOL CStandardBar::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	return CToolBarCtrl::OnChildNotify(message, wParam, lParam, pLResult);
}

BOOL CStandardBar::BeginAdjust(WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_BEGINADJUST\n"));
	// the customize dialog box is about to be displayed


	return TRUE;
}

BOOL CStandardBar::BeginDrag(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_BEGINDRAG\n"));

	// we are not implementing custon drag and drop
	* pLResult = FALSE;
	return FALSE;
}

BOOL CStandardBar::CustomizeHelp(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_CUSTHELP\n"));

	return TRUE;
}

BOOL CStandardBar::EndAdjust(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_ENDADJUST\n"));

	// the customize dialog box has been closed

	return TRUE;
}

BOOL CStandardBar::EndDrag(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_ENDDRAG\n"));

	// Code to handle custom drag and drop. This message indicates that
	// the item is being dropped
	* pLResult = FALSE;
	return TRUE;
}

BOOL CStandardBar::GetButtonInfo(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	// This notification message has to be handled correctly if
	// all operations in the custom dialogbox has to function correctly
	// We have to supply information for the button specified by pTBN->tbButton
	//
	// This notification is sent in the following cases
	//
	// After TBN_BEGINADJUST the control sends these notifications until
	// * pLResult is TRUE. We have to supply valid values when this value is
	// set to TRUE. Here the control is collecting information for all
	// the buttons that have to be displayed in the dialog box
	//
	// The control sends this notification to get information about
	// a button if the user is trying to add it to the toolbar or
	// rearranging the buttons on the toolbar from within the dialog

	TRACE(_T("TBN_GETBUTTONINFO\n"));

	TBNOTIFY *pTBN = (TBNOTIFY *) lParam;

	if (pTBN->iItem >= m_nButtonCount)
	{
		* pLResult = FALSE;
	}
	else
	{
		CString buffer;
		buffer.LoadString(pTBN->iItem + ID_NEW);

		// set the string for the button
		// truncate the string if its length is greater than the buffer
		// supplied by the toolbar
		_tcsncpy(pTBN->pszText, buffer, pTBN->cchText - 1);
		pTBN->pszText[pTBN->cchText - 1] = '\0';

		// set the button info
		pTBN->tbButton = m_pTBButtons[pTBN->iItem];

		// valid values are structure
		*pLResult = TRUE;
	}

	return TRUE;
}

BOOL CStandardBar::QueryDelete(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_QUERYDELETE\n"));

	// in this sample any button can be deleted
	// if a particular button cannot be deleted set *pResult to FALSE for that item
	*pLResult = FALSE;
	return TRUE;
}

BOOL CStandardBar::QueryInsert(WPARAM wParam, LPARAM  lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_QUERYINSERT\n"));

	// in this sample buttons can be inserted at any location on the
	// toolbar
	*pLResult = FALSE;
	return TRUE;
}

BOOL CStandardBar::Reset(WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	TRACE(_T("TBN_RESET\n"));

	*pLResult = TRUE;
	return TRUE;
}

BOOL CStandardBar::ToolBarChange(WPARAM wParam, LPARAM lParam,LRESULT* pLResult)
{
	TRACE(_T("TBN_TOOLBARCHANGE\n"));

	// the toolbar has changed
	return TRUE;
}


// Helper function for tooltips

CString CStandardBar::NeedText( UINT nID, NMHDR * pNotifyStruct, LRESULT * lResult )
{
	LPTOOLTIPTEXT lpTTT = (LPTOOLTIPTEXT)pNotifyStruct ;
	ASSERT(nID == lpTTT->hdr.idFrom);

	CString toolTipText;
	toolTipText.LoadString(nID);

	// szText length is 80
	int nLength = (toolTipText.GetLength() > 79) ? 79 : toolTipText.GetLength();

	toolTipText = toolTipText.Left(nLength);

	return toolTipText;
}


void CStandardBar::OnNeedTextW( UINT nID, NMHDR * pNotifyStruct, LRESULT * lResult )
{
	CString toolTipText = NeedText(nID, pNotifyStruct, lResult);

	LPTOOLTIPTEXTW lpTTT = (LPTOOLTIPTEXTW)pNotifyStruct;

#ifndef _UNICODE
	mbstowcs(lpTTT->szText,(LPCSTR)toolTipText, toolTipText.GetLength() + 1);
#else
	_tcsncpy(lpTTT->szText, toolTipText, toolTipText.GetLength() + 1);
#endif
}

void CStandardBar::OnNeedTextA( UINT nID, NMHDR * pNotifyStruct, LRESULT * lResult )
{
	CString toolTipText = NeedText(nID, pNotifyStruct, lResult);

	LPTOOLTIPTEXT lpTTT = (LPTOOLTIPTEXT)pNotifyStruct;

	_tcscpy(lpTTT->szText,(LPCTSTR)toolTipText);
}


///////////////////////////////////////////////////////////////////////
// This has been overridden so we can handle the tooltip TTN_NEEDTEXT//
// notification message                                              //
///////////////////////////////////////////////////////////////////////

BOOL CStandardBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);
	NMHDR* pNMHDR = (NMHDR*)lParam;
	HWND hWndCtrl = pNMHDR->hwndFrom;

	// get the child ID from the window itself
	// UINT nID = _AfxGetDlgCtrlID(hWndCtrl);

	//////////////////////////////////////////////////////////////////
	// If TTN_NEEDTEXT we cannot get the ID from the tooltip window //
	//////////////////////////////////////////////////////////////////

	int nCode = pNMHDR->code;

	//
	// if it is the following notification message
	// nID has to obtained from wParam
	//

	if (nCode == TTN_NEEDTEXTA || nCode == TTN_NEEDTEXTW)
	{
		UINT nID;   // = _AfxGetDlgCtrlID(hWndCtrl);
		nID = (UINT)wParam;


		ASSERT((UINT)pNMHDR->idFrom == (UINT)wParam);
		UNUSED(wParam);  // not used in release build
		ASSERT(hWndCtrl != NULL);
		ASSERT(::IsWindow(hWndCtrl));

		if (AfxGetThreadState()->m_hLockoutNotifyWindow == m_hWnd)
			return TRUE;        // locked out - ignore control notification

	// reflect notification to child window control
		if (ReflectLastMsg(hWndCtrl, pResult))
			return TRUE;        // eaten by child

		AFX_NOTIFY notify;
		notify.pResult = pResult;
		notify.pNMHDR = pNMHDR;
		return OnCmdMsg(nID, MAKELONG(nCode, WM_NOTIFY), &notify, NULL);
	}

	return CToolBarCtrl::OnNotify(wParam, lParam, pResult);
}
