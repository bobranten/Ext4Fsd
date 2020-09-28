// Ext2Mgr.h : main header file for the EXT2MGR application
//

#if !defined(AFX_EXT2MGR_H__6D13DACF_307A_4633_B268_5F4F4C2AD90E__INCLUDED_)
#define AFX_EXT2MGR_H__6D13DACF_307A_4633_B268_5F4F4C2AD90E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CExt2MgrApp:
// See Ext2Mgr.cpp for the implementation of this class
//

class CExt2MgrApp : public CWinApp
{
public:
	CExt2MgrApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExt2MgrApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CExt2MgrApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXT2MGR_H__6D13DACF_307A_4633_B268_5F4F4C2AD90E__INCLUDED_)
