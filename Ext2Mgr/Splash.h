//  ===========================================================================
//  File    Splash.h
//  Desc    The interface of the CSplash class
//  ===========================================================================
#ifndef _ABHI_SPLASH_H_
#define _ABHI_SPLASH_H_

#include "windows.h"

//  ===========================================================================
//  Class   CSplash
//  Desc    Use it for displaying splash screen for applications
//          Works only on Win2000 , WinXP and later versions of Windows
//  ===========================================================================
class CSplash
{
public:
    //  =======================================================================
    //  Func   CSplash
    //  Desc   Default constructor
    //  =======================================================================
    CSplash();
    
    //  =======================================================================
    //  Func   CSplash
    //  Desc   Constructor
    //  Arg    Path of the Bitmap that will be show on the splash screen
    //  Arg    The color on the bitmap that will be made transparent
    //  =======================================================================
    CSplash(UINT id, COLORREF colTrans);

    //  =======================================================================
    //  Func   ~CSplash
    //  Desc   Desctructor
    //  =======================================================================
    virtual ~CSplash();

    //  =======================================================================
    //  Func   ShowSplash
    //  Desc   Launches the non-modal splash screen
    //  Ret    void 
    //  =======================================================================
    void ShowSplash();

    //  =======================================================================
    //  Func   DoLoop
    //  Desc   Launched the splash screen as a modal window. Not completely 
    //         implemented.
    //  Ret    int 
    //  =======================================================================
    int DoLoop();

    //  =======================================================================
    //  Func   CloseSplash
    //  Desc   Closes the splash screen started with ShowSplash
    //  Ret    int 
    //  =======================================================================
    int CloseSplash();

    //  =======================================================================
    //  Func   SetBitmap
    //  Desc   Call this with the path of the bitmap. Not required to be used
    //         when the construcutor with the image path has been used.
    //  Ret    1 if succesfull
    //  Arg    Either the file path or the handle to an already loaded bitmap
    //  =======================================================================
    DWORD SetBitmap(UINT id);
    DWORD SetBitmap(HBITMAP hBitmap);

    //  =======================================================================
    //  Func   SetTransparentColor
    //  Desc   This is used to make one of the color transparent
    //  Ret    1 if succesfull
    //  Arg    The colors RGB value. Not required if the color is specified 
    //         using the constructor
    //  =======================================================================
    bool SetTransparentColor(COLORREF col);

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;

private:
    void Init();
    void  OnPaint(HWND hwnd);
    bool MakeTransparent();
    HWND RegAndCreateWindow();
    COLORREF m_colTrans;
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    void FreeResources();
    HBITMAP m_hBitmap;
    LPCTSTR m_lpszClassName;

};


#endif //_ABHI_SPLASH_H_
