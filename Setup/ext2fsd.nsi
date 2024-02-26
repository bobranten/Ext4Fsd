; ext2fsd.nsi
;
; This is a NSIS script to create an install program for the Ext2Fsd project
; developed by Bo Brantén <bosse@acc.umu.se> in 2020 to help beta testing.
;
; To build an installation program follow these steps:
; 1. Install NSIS (Nullsoft Scriptable Install System)
; 2. Compile Ext2Mgr, Ext2Srv and Ext4Fsd.
; 3. Run the command "makensis ext2fsd.nsi"
; This will create an install program called "Ext2Fsd-setup.exe".
; (for compatibility reasons the install program, install direcory and
;  file names are still called Ext2Fsd even if the driver supports the
;  ext2, ext3 and ext4 filesystems)
;
Unicode true
Name "Ext2,Ext3,Ext4 filesystem driver"
!define PROJECTNAME "Ext2Fsd"
!define DRIVERNAME "Ext2Fsd"
Icon "..\Ext2Mgr\res\Ext2Mgr.ico"
Caption "${PROJECTNAME} 0.71"
DirText "This is a release of the ${PROJECTNAME} project from Bo Brantén to test the new ext4 features metadata checksums and 64-bit block numbers. You may choose the install directory:"
InstallDir "$PROGRAMFILES\${PROJECTNAME}"
OutFile "${PROJECTNAME}-setup.exe"

; the paths to the binaries when compiled with Visual Studio to support Windows 10.
; (the driver files are automatically signed or testsigned by Visual Studio)
!define MGRPATH_X86 "..\Ext2Mgr\Release\x86"
!define MGRPATH_X64 "..\Ext2Mgr\Release\x64"
!define SRVPATH_X86 "..\Ext2Srv\Release\x86"
!define SRVPATH_X64 "..\Ext2Srv\Release\x64"
!define SYSPATH_X86 "..\Ext4Fsd\Release\x86"
!define SYSPATH_X64 "..\Ext4Fsd\Release\x64"
!define MSVPATH_X86 "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x86\Microsoft.VC143.CRT"
!define MSVPATH_X64 "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x64\Microsoft.VC143.CRT"
!define MFCPATH_X86 "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x86\Microsoft.VC143.MFC"
!define MFCPATH_X64 "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x64\Microsoft.VC143.MFC"
!define VCDLL_X86 "vcruntime140"
!define VCDLL_X64 "vcruntime140_1"
!define MFCDLL "mfc140"

; the paths to the binaries when compiled with an older WDK to support Windows XP - Windows 8.1.
; (remember to sign or testsign the driver files before packing the installation program)
;!define MGRPATH_X86 "..\..\Ext2Fsd-0.69\Setup"
;!define MGRPATH_X64 "..\..\Ext2Fsd-0.69\Setup"
;!define SRVPATH_X86 "..\..\Ext2Fsd-0.69\Setup"
;!define SRVPATH_X64 "..\..\Ext2Fsd-0.69\Setup"
;!define SYSPATH_X86 "..\Ext4Fsd\winxp\fre\i386"
;!define SYSPATH_X64 "..\Ext4Fsd\winnet\fre\amd64"
;!define MSVPATH_X86 "c:\windows\syswow64"
;!define MSVPATH_X64 "c:\windows\syswow64" ; "c:\windows\sysnative"
;!define MFCPATH_X86 "c:\windows\syswow64"
;!define MFCPATH_X64 "c:\windows\syswow64" ; "c:\windows\sysnative"
;!define VCDLL_X86 "msvcrt"
;!define VCDLL_X64 "msvcrt"
;!define MFCDLL "mfc42"
; note that when building the installation program on a 64-bit system
; the 32-bit system dll's will be in the "\windows\syswow64" directory while
; the 64-bit system dll's will be in the "\windows\system32" directory and
; since the installation script compiler itself is a 32-process the
; "\windows\system32" directory is reached through the alias "\windows\sysnative"
; (in this case the app's and dll's is always 32-bit so we get
;  the dll's from the "\windows\syswow64" directory)

RequestExecutionLevel admin

Function .onInit
    SetShellVarContext all
    IfFileExists $WINDIR\SysWOW64\*.* 0 else
        StrCpy $INSTDIR "$PROGRAMFILES64\${PROJECTNAME}"
        Goto endif
    else:
        StrCpy $INSTDIR "$PROGRAMFILES\${PROJECTNAME}"
    endif:
FunctionEnd

Section "Driver"
SetShellVarContext all
ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROJECTNAME}" \
                   "UninstallString"
StrCmp $0 "" install
    ; uninstall an old version if any.
install:

SetOutPath $INSTDIR

; select the files.
IfFileExists $WINDIR\SysWOW64\*.* 0 else
    ; 64-bit.
    File "${MSVPATH_X64}\${VCDLL_X64}.dll"
    File "${MFCPATH_X64}\${MFCDLL}.dll"
    File "${MGRPATH_X64}\Ext2Mgr.exe"
    File "${SRVPATH_X64}\Ext2Srv.exe"
;    File "${SYSPATH_X64}\${DRIVERNAME}.pdb"
    File "${SYSPATH_X64}\${DRIVERNAME}.sys"
    Goto endif
else:
    ; 32-bit.
    File "${MSVPATH_X86}\${VCDLL_X86}.dll"
    File "${MFCPATH_X86}\${MFCDLL}.dll"
    File "${MGRPATH_X86}\Ext2Mgr.exe"
    File "${SRVPATH_X86}\Ext2Srv.exe"
;    File "${SYSPATH_X86}\${DRIVERNAME}.pdb"
    File "${SYSPATH_X86}\${DRIVERNAME}.sys"
endif:

File "..\ext4fsd\${DRIVERNAME}.inf"

SetOutPath $INSTDIR\Documents
File "..\ext4fsd\COPYRIGHT.txt"
File "..\ext4fsd\FAQ.txt"
File "..\ext4fsd\notes.txt"
File "..\ext4fsd\readme.txt"

; install the driver.
IfFileExists $WINDIR\SysWOW64\*.* 0 else32
    ExecWait '"$WINDIR\sysnative\rundll32.exe" setupapi.dll,InstallHinfSection DefaultInstall 132 $INSTDIR\${DRIVERNAME}.inf'
    Goto endif32
else32:
    ExecWait '"rundll32.exe" setupapi.dll,InstallHinfSection DefaultInstall 132 $INSTDIR\${DRIVERNAME}.inf'
endif32:

; create the uninstaller.
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROJECTNAME}" \
            "DisplayName" "Ext2,Ext3,Ext4 filesystem driver"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROJECTNAME}" \
            "UninstallString" '"$INSTDIR\Uninstall.exe"'
WriteUninstaller "Uninstall.exe"

; create the start menu items.
createDirectory "$SMPROGRAMS\${PROJECTNAME}"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Ext2 Volume Manager.lnk" "$INSTDIR\Ext2Mgr.exe" "" "$INSTDIR\Ext2Mgr.exe" "" SW_SHOWNORMAL "" "Ext2 Volume Manager"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Uninstall Ext2Fsd.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" "" SW_SHOWNORMAL "" "Uninstall Ext2Fsd"
createDirectory "$SMPROGRAMS\${PROJECTNAME}\Documents"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Documents\COPYRIGHT.lnk" "$INSTDIR\Documents\COPYRIGHT.txt" "" "" "" SW_SHOWNORMAL "" "COPYRIGHT"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Documents\FAQ.lnk" "$INSTDIR\Documents\FAQ.txt" "" "" "" SW_SHOWNORMAL "" "FAQ"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Documents\Release notes.lnk" "$INSTDIR\Documents\notes.txt" "" "" "" SW_SHOWNORMAL "" "Release notes"
createShortCut "$SMPROGRAMS\${PROJECTNAME}\Documents\README.lnk" "$INSTDIR\Documents\readme.txt" "" "" "" SW_SHOWNORMAL "" "README"

; install Ext2Srv and start the driver.
ExecWait '"$INSTDIR\Ext2Srv.exe" /installasservice'
ExecWait '"net.exe" start ${DRIVERNAME}'
SectionEnd

Function un.onInit
    SetShellVarContext all

    MessageBox MB_YESNO "This will uninstall ${PROJECTNAME}. Continue?" IDYES continue
        Abort
    continue:

    IfFileExists $WINDIR\SysWOW64\*.* 0 else
        StrCpy $INSTDIR "$PROGRAMFILES64\${PROJECTNAME}"
        Goto endif
    else:
        StrCpy $INSTDIR "$PROGRAMFILES\${PROJECTNAME}"
    endif:
FunctionEnd

Section "Uninstall"
SetShellVarContext all

; stop and uninstall Ext2Srv.
ExecWait '"net.exe" stop ext2srv'
ExecWait '"$INSTDIR\Ext2Srv.exe" /removeservice'

; uninstall the driver.
IfFileExists $WINDIR\SysWOW64\*.* 0 else
    ExecWait '"$WINDIR\sysnative\rundll32.exe" setupapi.dll,InstallHinfSection DefaultUninstall 132 $INSTDIR\${DRIVERNAME}.inf'
    Delete $INSTDIR\${VCDLL_X64}.dll"
    Goto endif
else:
    ExecWait '"rundll32.exe" setupapi.dll,InstallHinfSection DefaultUninstall 132 $INSTDIR\${DRIVERNAME}.inf'
    Delete $INSTDIR\${VCDLL_X86}.dll"
endif:

; delete the start menu items.
Delete "$SMPROGRAMS\${PROJECTNAME}\Documents\COPYRIGHT.lnk"
Delete "$SMPROGRAMS\${PROJECTNAME}\Documents\FAQ.lnk"
Delete "$SMPROGRAMS\${PROJECTNAME}\Documents\Release notes.lnk"
Delete "$SMPROGRAMS\${PROJECTNAME}\Documents\README.lnk"
Delete "$SMPROGRAMS\${PROJECTNAME}\Ext2 Volume Manager.lnk"
Delete "$SMPROGRAMS\${PROJECTNAME}\Uninstall Ext2Fsd.lnk"
RMDir "$SMPROGRAMS\${PROJECTNAME}\Documents"
RMDir "$SMPROGRAMS\${PROJECTNAME}"

; delete the installed files.
Delete $INSTDIR\Documents\COPYRIGHT.txt"
Delete $INSTDIR\Documents\FAQ.txt"
Delete $INSTDIR\Documents\notes.txt"
Delete $INSTDIR\Documents\readme.txt"

Delete $INSTDIR\${DRIVERNAME}.inf
;Delete $INSTDIR\${DRIVERNAME}.pdb
Delete $INSTDIR\${DRIVERNAME}.sys
Delete $INSTDIR\${MFCDLL}.dll"
Delete $INSTDIR\Ext2Mgr.exe"
Delete $INSTDIR\Ext2Srv.exe"
Delete $INSTDIR\Uninstall.exe

RMDir $INSTDIR\Documents
RMDir $INSTDIR

; delete the reg key for the uninstaller.
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROJECTNAME}"
SectionEnd
