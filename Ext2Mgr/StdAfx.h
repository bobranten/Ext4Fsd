// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A7E6CA3E_7403_4367_BC73_C64644BB42F9__INCLUDED_)
#define AFX_STDAFX_H__A7E6CA3E_7403_4367_BC73_C64644BB42F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>

#include "AfxPriv.h"
#include "enumDisk.h"
#include <process.h>
#include "resource.h"

#define EXT2_DIALOG_MAGIC  'EXTM'

#define SET_TEXT(id, s) ((CWnd *)GetDlgItem(id))->SetWindowText(s)
#define SET_WIN(id, t) ((CWnd *)GetDlgItem(id))->EnableWindow(t)
#define SET_CHECK(ID, V) ((CButton *)GetDlgItem(ID))->SetCheck(V)

//--------------our own message-----------------
#define WM_TRAY_ICON_NOTIFY		WM_USER + 0x1001
#define WM_TERMINATE_PROGRAM    WM_USER + 0x1002
#define WM_GROUP_BOX_UPDATED    WM_USER + 0x1003
#define WM_MOUNTPOINT_NOTIFY    WM_USER + 0x1004
//----------------------------------------------

extern CHAR *IrpMjStrings[];
extern CHAR *PerfStatStrings[];



/*
 * Ext2Pipe.cpp
 */

BOOL Ext2DefineDosDevice(DWORD flags,  CHAR *dos,  CHAR *symlink);
DWORD Ext2QueryDrive(CHAR drive, CHAR *symlink);

BOOL Ext2StartPipeSrv();
VOID Ext2StopPipeSrv();

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A7E6CA3E_7403_4367_BC73_C64644BB42F9__INCLUDED_)
