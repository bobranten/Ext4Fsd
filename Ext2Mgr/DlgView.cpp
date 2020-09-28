// DlgView.cpp : implementation file
//

#include "stdafx.h"
#include "DlgView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgView

IMPLEMENT_DYNCREATE(CDlgView, CScrollView)

CDlgView::CDlgView()
{
}

CDlgView::~CDlgView()
{
}


BEGIN_MESSAGE_MAP(CDlgView, CScrollView)
	//{{AFX_MSG_MAP(CDlgView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgView drawing

void CDlgView::OnDraw(CDC* pDC)
{
	CRect rectClient;
    CSize size = GetTotalSize();

    rectClient.left = 0;
	rectClient.top = 0;
    rectClient.bottom = size.cy;
    rectClient.right = size.cx;

	rectClient.DeflateRect(15, 15);
	pDC->DrawText(m_csText, rectClient, DT_LEFT);
	
	rectClient.InflateRect(5, 5);
	pDC->Draw3dRect(rectClient, RGB(0, 0, 255), RGB(0, 0, 255));

}

/////////////////////////////////////////////////////////////////////////////
// CDlgView diagnostics

#ifdef _DEBUG
void CDlgView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CDlgView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDlgView message handlers

void CDlgView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	
}
