// TreeList.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TreeList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeList

CTreeList::CTreeList()
{
    m_SelectionRect = CRect(0,0,0,0);
    m_SelectionFlag = FALSE;
    m_Point = CPoint(0, 0);
	m_PrevItem = -1;
    m_Rows = 0;
    m_Columns = 1;
    m_hBitmap = NULL;
    m_hMemDC = NULL;
    m_hOldBmp = NULL;
    m_bFocus = FALSE;
}

CTreeList::~CTreeList()
{
	if (m_hBitmap) {

        if (m_hMemDC) {
            ::SelectObject(m_hMemDC, m_hOldBmp);
            ::DeleteDC(m_hMemDC);
        }

        ::DeleteObject (m_hBitmap);
        m_hBitmap = NULL;
    }
}


BEGIN_MESSAGE_MAP(CTreeList, CListCtrl)
	//{{AFX_MSG_MAP(CTreeList)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(NM_KILLFOCUS, OnKillfocus)
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETFONT, OnSetFont)
    ON_WM_MEASUREITEM_REFLECT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTreeList message handlers

void CTreeList::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_Point = point;
	m_SelectionFlag = FALSE;

	Invalidate();
	
	CListCtrl::OnLButtonDown(nFlags, point);
}

BOOL CTreeList::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED| WS_BORDER;
	return CListCtrl::PreCreateWindow(cs);
}

LRESULT CTreeList::OnSetFont(WPARAM wParam, LPARAM)
{
    LRESULT res = Default();

    CRect rc;
    GetWindowRect( &rc );

    WINDOWPOS wp;
    wp.hwnd  = m_hWnd;
    wp.cx    = rc.Width();
    wp.cy    = rc.Height();
    wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
    SendMessage( WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp );

    return res;
}

void CTreeList::MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
    lpMeasureItemStruct->itemHeight = 16;
}

void CTreeList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    PULONG  pMagic = (PULONG) lpDrawItemStruct->itemData;

    TCHAR  lpBuffer[256];
    LV_ITEM lvi;
    LV_COLUMN lvc, lvcprev ;

    memset(lpBuffer, 0, 256);
    lvi.mask = LVIF_TEXT | LVIF_PARAM ;
    lvi.iItem = lpDrawItemStruct->itemID ;  
    lvi.iSubItem = 0;
    lvi.pszText = lpBuffer ;
    lvi.cchTextMax = sizeof(lpBuffer);
    GetItem(&lvi);
    ::ZeroMemory(&lvc, sizeof(lvc));
    ::ZeroMemory(&lvcprev, sizeof(lvcprev));
    lvc.mask = LVCF_WIDTH |LVCF_FMT;
    lvcprev.mask = LVCF_WIDTH | LVCF_FMT;

    CDC* pDC;
    pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    int nCol;
    CRect rcText = lpDrawItemStruct->rcItem;

    CFont Fnt, *pOldFont = NULL;
    LOGFONT lf;

    int cyPixels = pDC->GetDeviceCaps(LOGPIXELSY);
    memset(&lf, 0, sizeof(LOGFONT));

    if (pMagic == NULL) {
        return;
    }

    if (*pMagic == EXT2_CDROM_DEVICE_MAGIC || *pMagic == EXT2_DISK_MAGIC) {

        if (IsVistaOrAbove()) {
            lstrcpy(lf.lfFaceName, "MS Sans Serif"); /*Courier New*/
            lf.lfHeight = -MulDiv(8, cyPixels, 72);
            lf.lfWeight = TRUE;
        } else {
            lstrcpy(lf.lfFaceName, "Arial Black"); /*Courier New*/
            lf.lfHeight = -MulDiv(8, cyPixels, 72);
            lf.lfWeight = TRUE;
        }
    } else {
        lstrcpy(lf.lfFaceName, "MS Sans Serif");
        lf.lfHeight = -MulDiv(8, cyPixels, 72);
    }

    Fnt.CreateFontIndirect(&lf);
    pOldFont = (CFont *) pDC->SelectObject(&Fnt);

    /* loading bitmap */
    if (m_hBitmap == NULL) {
        m_hBitmap = (HBITMAP)::LoadImage(GetModuleHandle(NULL), 
                             MAKEINTRESOURCE(IDB_LINE_SEP),
                             IMAGE_BITMAP, 0, 0, 0);

        if (m_hBitmap) {
            m_hMemDC  = ::CreateCompatibleDC(this->GetDC()->m_hDC);
            m_hOldBmp = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);
        }
    }

    if (TRUE) {

        if (!m_SelectionFlag) {
            for (nCol=0; GetColumn(nCol, &lvc); nCol++) {
                if (nCol > 0) {
                    GetSubItemRect(lpDrawItemStruct->itemID, 
                        nCol,LVIR_BOUNDS, m_SelectionRect);
                } else {
                    GetItemRect(lpDrawItemStruct->itemID, 
                             m_SelectionRect,LVIR_BOUNDS);
                    m_SelectionRect.right = GetColumnWidth(0);
                    m_SelectionRect.left = 0;
                }

                if (m_SelectionRect.PtInRect(m_Point)) {
                    m_SelectionFlag = TRUE;
                    break;
                } else {
                    m_SelectionFlag = FALSE;
                }
            } 
        }
 
        if ((lpDrawItemStruct->itemState & ODS_SELECTED) && m_SelectionFlag ) {

            CRect rc = lpDrawItemStruct->rcItem;
            rc.left  += 4; rc.right -= 4;
            rc.top   += 1; rc.bottom -= 0;
            if (*pMagic == EXT2_CDROM_DEVICE_MAGIC || *pMagic == EXT2_DISK_MAGIC) {
                rc.bottom -= 3; rc.top -= 1;
                rc.right = (rc.Width() * 7 / 8) + rc.left;
            }
            pDC->FillSolidRect(&rc, GetSysColor(m_bFocus ? COLOR_HIGHLIGHT : COLOR_INACTIVEBORDER));
        } else {
            CRect rc = lpDrawItemStruct->rcItem;
            pDC->FillSolidRect(&rc, GetSysColor(COLOR_WINDOW)) ;
            pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT)) ; 
        }
    }

    for (nCol=0; GetColumn(nCol, &lvc); nCol++) {

        UINT  uFormat    = DT_LEFT ;

        if (*pMagic == EXT2_CDROM_DEVICE_MAGIC || *pMagic == EXT2_DISK_MAGIC) {

            rcText = lpDrawItemStruct->rcItem;
            rcText.left += 4;
            rcText.bottom += 1;
            rcText.top = rcText.bottom - 6 + lf.lfHeight;

            ::DrawText(lpDrawItemStruct->hDC, lpBuffer, (int)strlen(lpBuffer), 
                              &rcText, DT_LEFT) ;

            CRect rect = lpDrawItemStruct->rcItem;
            int rc = 0;
            BITMAP  cs;


            rect.top = rcText.bottom - 4;
            rc = ::GetObject(m_hBitmap, sizeof(cs), &cs);
            if (rc == 0) {
                pDC->SelectObject(pOldFont);
                return;
            }

            ::StretchBlt(pDC->m_hDC, rect.left + 4, (rect.bottom + rect.top) / 2,
                         rect.Width() * 7 / 8, cs.bmHeight,
                         m_hMemDC, 0, 0, cs.bmWidth, cs.bmHeight, SRCCOPY);

        } else {

            if (nCol > 0) {
               GetColumn(nCol, &lvcprev) ;
               rcText.left = rcText.right;
               rcText.right += lvcprev.cx;
               rcText.left += 4;

                if (nCol == 3 || nCol == 4) {
                    uFormat = DT_RIGHT;
                    rcText.right -= 4;
                }

            } else {
                rcText = lpDrawItemStruct->rcItem;
                rcText.top += (16 + lf.lfHeight) / 2;
                rcText.right = rcText.left + GetColumnWidth(0);
                rcText.left += 20;
            }

            // Get and draw the text 
            memset(lpBuffer, 0, 256);
            ::ZeroMemory(&lvi, sizeof(lvi));
            lvi.iItem = lpDrawItemStruct->itemID;
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iSubItem = nCol;
            lvi.pszText = lpBuffer;
            lvi.cchTextMax = sizeof(lpBuffer);
            GetItem(&lvi);
   
            ::DrawText(lpDrawItemStruct->hDC, lpBuffer, (int)strlen(lpBuffer), 
                              &rcText, uFormat) ;

            if (nCol == 0) {
                rcText.left -= 20;
            } else {
                rcText.left -= 4;
                if (nCol == 3 || nCol == 4) {
                    rcText.right += 4;
                }
            }
        }
    }

    pDC->SelectObject(pOldFont);

    return;
}

void CTreeList::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_Point = point;
	m_SelectionFlag = FALSE;

	Invalidate();
	
	CListCtrl::OnRButtonDown(nFlags, point);
}

void CTreeList::OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    m_bFocus = FALSE;
    Invalidate();
	*pResult = 0;
}

void CTreeList::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
    m_bFocus = TRUE;
    Invalidate();

	*pResult = 0;
}

int CTreeList::QuerySubItemText(int item, CHAR *Data, int length)
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

VOID CTreeList::Redraw()
{
    m_bFocus = TRUE;
    m_SelectionFlag = TRUE;
    Invalidate();
}