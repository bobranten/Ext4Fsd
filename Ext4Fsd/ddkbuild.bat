@echo off
@set VERSION=V7.4
@set OSR_DEBUG=off

set W7BASE=C:\WINDDK\7600.16385.1

@if "%OS%"=="Windows_NT" goto :Prerequisites
@echo This script requires Windows NT 4.0 or later to run properly!
goto :EOF
:Prerequisites
:: Check whether FINDSTR is available. It's used to show warnings etc.
findstr /? > NUL 2>&1 || echo "FINDSTR is a prerequisite but wasn't found!" && goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
::    $Id: ddkbuild.cmd 43 2009-11-12 03:46:08Z oliver $
::
::    This software is supplied for instructional purposes only.
::
::    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
::    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
::    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
::    THE IMPLIED WARRANTIES OF MECHANTABILITY OR FITNESS FOR A PARTICULAR
::    PURPOSE.  THE ENTIRE RISK ARISING FROM THE USE OF THIS SOFTWARE REMAINS
::    WITH YOU.  OSR's entire liability and your exclusive remedy shall not
::    exceed the price paid for this material.  In no event shall OSR or its
::    suppliers be liable for any damages whatsoever (including, without
::    limitation, damages for loss of business profit, business interruption,
::    loss of business information, or any other pecuniary loss) arising out
::    of the use or inability to use this software, even if OSR has been
::    advised of the possibility of such damages.  Because some states/
::    jurisdictions do not allow the exclusion or limitation of liability for
::    consequential or incidental damages, the above limitation may not apply
::    to you.
::
::    OSR Open Systems Resources, Inc.
::    105 Route 101A Suite 19
::    Amherst, NH 03031  (603) 595-6500 FAX: (603) 595-6503
::    report bugs to <bugs@osr.com>
::    alternatively report them via <http://assarbad.net/contact/>
::
::
::    MODULE:
::
::      ddkbuild.cmd
::
::    ABSTRACT:
::
::      This script allows drivers to be built with Visual Studio 2002 through
::      Visual Studio 2008 and possibly future versions. It will also work fine
::      from the command line.
::      If you are interested in a project wizard that makes use of this script,
::      try DDKWizard from <http://ddkwizard.assarbad.net>.
::
::    AUTHOR(S):
::
::      - OSR Open Systems Resources, Inc.
::      - Oliver Schneider (ddkwizard.assarbad.net)
::
::    REQUIREMENTS:
::
::      Environment variables that must be set.
::        %NT4BASE%  - Set this up for "-NT4" builds (legacy, support not tested)
::        %W2KBASE%  - Set this up for "-W2K*" builds (legacy, support not tested)
::        %WXPBASE%  - Set this up for "-WXP*" builds
::        %WNETBASE% - Set this up for "-WNET*" builds
::        %WLHBASE%  - Set this up for "-WLH*" builds
::        %W7BASE%   - Set this up for "-W7*" builds
::        %WDF_ROOT% - Must be set if attempting to do a WDF Build.
::
::      Examples:
::        NT4BASE : could be "D:\NT4DDK"
::        W2KBASE : could be "D:\Nt50DDK"
::        WXPBASE : could be "D:\WINDDK\2600"
::        WNETBASE: could be "D:\WINDDK\3790.1830" or "C:\WINDDK\3790"
::        W7BASE  : could be "C:\WINDDK\7600.16385.0"
::
::    COMMAND FORMAT:
::
::      Run the script without any parameters to get the whole help content!
::      Note: "-WDF" has been tested with the 01.00.5054 version of the framework
::
::    RETURN CODES AND THEIR MEANING:
::
::      001 == Unknown type of build. Please recheck parameters.
::      002 == WDF_ROOT is not defined, are you using 00.01.5054 or later?
::      003 == To build using type <target> you need to set the <basedir>
::             environment variable to point to the <basediros> DDK base
::             directory!
::      004 == NT4BASE, W2KBASE, WXPBASE, WNETBASE and/or W7BASE environment
::             variable(s) not set. Environment variable(s) must be set by user
::             according to DDK version(s) installed.
::      005 == <build type> must be 'checked', 'free', 'chk' or 'fre'
::             (case-insensitive).
::      006 == No DIR or SOURCES file found in the given target directory.
::      007 == Target directory must have a SOURCES+MAKEFILE or DIRS file.
::      008 == The <directory> parameter must be a valid directory.
::      009 == The SETENV script failed.
::
::      Note: If %OSR_ERRCODE% and %ERRORLEVEL% are equal, the return code stems
::            from one of the tools being called during the build process.
::
::    BROWSE FILES:
::
::      This procedure supports the building of BROWSE files to be used by
::      Visual Studio 6 and by Visual Studio.NET  However, the BSCfiles created
::      by bscmake for the two are not compatible. When this command procedure
::      runs, it selects the first bscmake.exe found in the path. So, make sure
::      that the correct bscmake.exe is in the path ...
::
::      Note that if using Visual Studio.NET the .BSC must be added to the project
::      in order for the project to be browsed.
::      Another alternative is the VS addon named "Visual Assist X" which will
::      parse the header files - no more need for browse files.
::
::    COMPILERS:
::
::      If you are building NT4 you should really be using the VC6 compiler.
::      Later versions of the DDK now contain the compiler and the linker. This
::      procedure should use the correct compiler.
::
::    GENERAL COMMENTS:
::
::      This procedure has been cleaned up to be modular and easy to understand.
::
::      As of the Server 2003 SP1 DDK DDKBUILD now clears the NO_BROWSE_FILE and
::      NO_BINPLACE environment variables so that users can use these features.
::
::      Starting with the Vista WDK, the output in the respective tool window
::      in VS is in Unicode by default. This garbles the output from DDKBUILD
::      and we therefore clear the environment variable VS_UNICODE_OUTPUT.
::
::      To modify the default behavior of this script with the newest WDKs,
::      set the variable SETTING_OACR in the ddkbldenv.cmd hook script to turn
::      OACR back on (NB: no OACR tools exist for Itanium in the WDK).
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / MAIN function of the script
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:MAIN
:: Building "stack frame"
setlocal ENABLEEXTENSIONS & pushd .
:: Check whether the REG utility is available
reg /? > NUL 2>&1 && set OSR_REGAVAILABLE=1

:: This is set by client-side keyword substitution
set SVN_REVISION=$Revision: 43 $
:: Extract the revision number from the revision keyword
set SVN_REVISION=%SVN_REVISION:~0,-2%
set SVN_REVISION=%SVN_REVISION:~11%
:: This is set by client-side keyword substitution
set SVN_REVDATE=$Date: 2009-11-12 03:46:08 +0000 (Thu, 12 Nov 2009) $
:: Extract the date from the Date keyword
set SVN_REVDATE=%SVN_REVDATE:~7,10%
set VERSION=%VERSION%/r%SVN_REVISION%

:: Init some special variables
set OSR_VERSTR=OSR DDKBUILD.CMD %VERSION% (%SVN_REVDATE%) - OSR, Open Systems Resources, Inc.
set OSR_PREBUILD_SCRIPT=ddkprebld.cmd
set OSR_POSTBUILD_SCRIPT=ddkpostbld.cmd
set OSR_SETENV_SCRIPT=ddkbldenv.cmd
set OSR_ECHO=@echo DDKBLD:
set OSR_RANDEXT=%RANDOM%%RANDOM%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set error messages
:: Possible codes: 1
set ERR_UnknownBuildType=Unknown type of build. Please recheck parameters.
:: Possible codes: 2
set ERR_NoWdfRoot=WDF_ROOT is not defined, are you using 00.01.5054 or later?
:: Possible codes: 3
set ERR_BaseDirNotSet=To build using type %%OSR_TARGET%% you need to set the %%%%%%BASEDIRVAR%%%%%% environment variable to point to the %%BASEDIROS%% DDK base directory!
:: Possible codes: 4
set ERR_NoBASEDIR=NT4BASE, W2KBASE, WXPBASE, WNETBASE and/or W7BASE environment variable(s) not set. Environment variable(s) must be set by user according to DDK version(s) installed.
:: Possible codes: 5
set ERR_BadMode=^<build type^> must be 'checked', 'free', 'chk' or 'fre' (case-insensitive).
:: Possible codes: 6
set ERR_NoTarget=Target directory must have a SOURCES+MAKEFILE or DIRS file.
:: Possible codes: 7, 8
set ERR_NoDir=The ^<directory^> parameter must be a valid directory.
:: Possible codes: 9
set ERR_SetEnvFailed=The SETENV script failed.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Clear the error code variable
set OSR_ERRCODE=0
set PREFAST_BUILD=0

:: Turn on tracing, use %OSR_TRACE% instead of ECHO
if /i "%OSR_DEBUG%" == "on" (set OSR_TRACE=%OSR_ECHO% [TRACE]) else (set OSR_TRACE=rem)
:: Turn on echoing of current line if %OSR_DEBUG% is set to "on"
@echo %OSR_DEBUG%

:: Output version string
@echo %OSR_VERSTR%
%OSR_TRACE% ^(Current module: ^"%~f0^"^)
@echo.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set the target platform variable
set OSR_TARGET=%~1
:: Remove any dashes in the variable
if not "%OSR_TARGET%" == "" set OSR_TARGET=%OSR_TARGET:-=%
:: Show help if the target parameter is empty after removal of the dashes
if "%OSR_TARGET%" == "" goto :USAGE

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: In the build directory check for this script and call it if it exists.
:: This allows to override any global system variable setting, if desired.
if not "%3" == "" call :GetCustomEnvironment "%~f3"
if not "%OSR_ERRCODE%" == "0" goto :USAGE
:: Additional error handling for better usability
:: These subroutines will also attempt to locate the requested DDK!!!
set OSR_ERRCODE=3
%OSR_TRACE% Checking whether the environment variable for the build type was set
:: Calling as a subroutine has 2 advantages:
:: 1. the script does not quit if the label was not found
:: 2. we return to the line after the call and can check variables there
call :%OSR_TARGET%Check > NUL 2>&1
:: If the BASEDIROS/BASEDIRVAR variable is not defined, it means the subroutine did not exist!
if not DEFINED BASEDIROS call :ShowErrorMsg 1 "%ERR_UnknownBuildType% (BASEDIROS)" & goto :USAGE
if not DEFINED BASEDIRVAR call :ShowErrorMsg 1 "%ERR_UnknownBuildType% (BASEDIRVAR)" & goto :USAGE
if not "%OSR_ERRCODE%" == "0" call :ShowErrorMsg %OSR_ERRCODE% "%ERR_BaseDirNotSet%" & goto :USAGE

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BASEDIR=%%%BASEDIRVAR%%%
call :ResolveVar BASEDIR
call :MakeShort BASEDIR "%BASEDIR%"
:: Check for existing %BASEDIR%
if "%BASEDIR%" == "" call :ShowErrorMsg 4 "%ERR_NoBASEDIR%" & goto :USAGE
set PATH=%BASEDIR%\bin;%PATH%
%OSR_TRACE% Now jump to the initialization of the commandline
:: Calling as a subroutine has 2 advantages:
:: 1. the script does not quit if the label was not found
:: 2. we return to the line after the call and can check variables there
call :%OSR_TARGET%Build

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
%OSR_TRACE% We returned from the variable initialization
if not DEFINED OSR_CMDLINE call :ShowErrorMsg 1 "%ERR_UnknownBuildType% (OSR_CMDLINE)" & goto :USAGE

%OSR_TRACE% Hurrah, all the variables have been initialized, continuing
:: Proceed with common build steps
goto :CommonBuild

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check whether the parameter makes sense and try to
:: correct it if possible
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: These labels are for compatibility with the respective
:: modes supported by another flavor of DDKBUILD.
:WIN7Check
:WIN764Check
:WIN7A64Check
:WIN7WLHCheck
:WIN7WLH64Check
:WIN7WLHA64Check
:WIN7NETCheck
:WIN7NET64Check
:WIN7NETA64Check
:WIN7XPCheck
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:W7Check
:W7I64Check
:W7X64Check
:W7LHCheck
:W7LHI64Check
:W7LHX64Check
:W7NETCheck
:W7NETI64Check
:W7NETX64Check
:W7XPCheck
set BASEDIROS=Windows 7/Windows 2008 Server R2
set BASEDIRVAR=W7BASE
:: The default for OACR is off ("no_oacr" appended)
if not DEFINED SETTING_OACR set SETTING_OACR=no_oacr
:: The default for "separate_object_root" is to not pass it to setenv.bat
if not DEFINED SETTING_SEP_OBJ_ROOT set SETTING_SEPARATE_OBJ_ROOT=
:: Other flavor of DDKBUILD
if not DEFINED W7BASE if DEFINED WIN7BASE set BASEDIRVAR=WIN7BASE
:: Compatibility between BUILD and VS ... prevent pipes from being used
%OSR_ECHO% Clearing %%VS_UNICODE_OUTPUT%% ...
set VS_UNICODE_OUTPUT=
:: Return to caller if the BASEDIR is already defined (either customized or global)
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :DetectBaseDirTemp "7600.16385.0"
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :CommonCheckSetVarWithReturn
goto :CommonCheckErrorNotSupportedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: These labels are for compatibility with the respective
:: modes supported by another flavor of DDKBUILD.
:WLH64Check
:WLHA64Check
:WLHXP64Check
:WLHNET64Check
:WLHNETA64Check
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:WLHCheck
:WLHX64Check
:WLHI64Check
:WLHNETX64Check
:WLHNETI64Check
:WLHXPCheck
:WLH2KCheck
:WLHNETCheck
set BASEDIROS=Windows Vista/Windows 2008 Server
set BASEDIRVAR=WLHBASE
:: Compatibility between BUILD and VS ... prevent pipes from being used
%OSR_ECHO% Clearing %%VS_UNICODE_OUTPUT%% ...
set VS_UNICODE_OUTPUT=
:: Return to caller if the BASEDIR is already defined (either customized or global)
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :DetectBaseDirTemp "6001.18002 6001.18001 6001.18000 6000"
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :CommonCheckSetVarWithReturn
goto :CommonCheckErrorNotSupportedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: These labels are for compatibility with the respective
:: modes supported by another flavor of DDKBUILD.
:WNETW2KCheck
:WNETA64Check
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:WNET2KCheck
:WNETXPCheck
:WNETXP64Check
:WNET64Check
:WNETI64Check
:WNETAMD64Check
:WNETX64Check
:WNETCheck
set BASEDIROS=Windows 2003 Server
set BASEDIRVAR=WNETBASE
:: Return to caller if the BASEDIR is already defined (either customized or global)
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :DetectBaseDirTemp "3790.1830 3790.1218 3790"
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :CommonCheckSetVarWithReturn
goto :CommonCheckErrorNotDetectedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: These labels are for compatibility with the respective
:: modes supported by another flavor of DDKBUILD.
:XPCheck
:XP64Check
:XPW2KCheck
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:WXP64Check
:WXPI64Check
:WXPCheck
:WXP2KCheck
set BASEDIROS=Windows XP
set BASEDIRVAR=WXPBASE
:: Other flavor of DDKBUILD
if not DEFINED WXPBASE if DEFINED XPBASE set BASEDIRVAR=XPBASE
:: Return to caller if the BASEDIR is already defined (either customized or global)
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :DetectBaseDirTemp "2600.1106 2600"
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :CommonCheckSetVarWithReturn
goto :CommonCheckErrorNotDetectedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:W2K64Check
:W2KI64Check
:W2KCheck
set BASEDIROS=Windows 2000
set BASEDIRVAR=W2KBASE
:: Return to caller
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :CommonCheckMsg2
goto :CommonCheckErrorNotSupportedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:NT4Check
set BASEDIROS=Windows NT4
set BASEDIRVAR=NT4BASE
:: Return to caller
if DEFINED %BASEDIRVAR% goto :CommonCheckNoErrorWithReturn
call :CommonCheckMsg2
goto :CommonCheckErrorNotSupportedWithReturn

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CommonCheckMsg1
echo.
%OSR_ECHO% WARNING: %%%BASEDIRVAR%%% NOT SET!
%OSR_ECHO%   Attempting to auto-detect the installation folder and set %%%BASEDIRVAR%%%.
%OSR_ECHO%   (If this fails *you* will have to set it!)
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CommonCheckMsg2
echo.
%OSR_ECHO% WARNING:
%OSR_ECHO%   Auto-detection of the folder settings is not supported for the requested DDK.
%OSR_ECHO%   Please set %%%BASEDIRVAR%%% yourself!
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CommonCheckSetVarWithReturn
%OSR_ECHO% Found!
echo.
set %BASEDIRVAR%=%BASEDIRTEMP%
set BASEDIRTEMP=
:: Tell the caller it was successful
:CommonCheckNoErrorWithReturn
set OSR_ERRCODE=0
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CommonCheckErrorNotDetectedWithReturn
echo.
%OSR_ECHO% None of the usual default paths works. Set %%%BASEDIRVAR%%% manually!
:CommonCheckErrorNotSupportedWithReturn
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Initialize variables specific to the respective platform
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: 
:: Valid parameters for setenv in different DDKs/WDKs:
::
:: 2600        - "setenv <directory> [fre|chk] [64] [hal]"
:: 2600.1106   - "setenv <directory> [fre|chk] [64] [hal] [WXP|W2K]"
:: 3790        - "setenv <directory> [fre|chk] [64|AMD64] [hal] [WXP|WNET|W2K]"
:: 3790.1830   - "setenv <directory> [fre|chk] [64|AMD64] [hal] [WXP|WNET|W2K] [no_prefast] [bscmake]"
:: 6000        - "setenv <directory> [fre|chk] [64|AMD64] [hal] [WLH|WXP|WNET|W2K] [bscmake]"
:: 6001.18000  - "setenv <directory> [fre|chk] [64|x64] [hal] [WLH|WXP|WNET|W2K] [bscmake]"
:: 6001.18001  - "setenv <directory> [fre|chk] [64|x64] [hal] [WLH|WXP|WNET|W2K] [bscmake]"
:: 6001.18002  - "setenv <directory> [fre|chk] [64|x64] [hal] [WLH|WXP|WNET|W2K] [bscmake]"
:: 7600.16385  - "setenv <directory> [fre|chk] [64|x64] [WIN7|WLH|WXP|WNET] [bscmake] [no_oacr] [separate_object_root]"

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: NT 4.0 build using NT4 DDK
:NT4Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% "%%MSDEVDIR%%"
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W2K build for 32bit using WXP DDK
:XPW2KBuild
:WXP2KBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\w2k\set2k.bat" %%BASEDIR%% %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W2K build for 64bit (Itanium) using W2K DDK
:W2K64Build
:W2KI64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv64.bat" %%BASEDIR%% %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W2K build for 32bit using W2K DDK
:W2KBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 64bit (Itanium) using WXP DDK
:XP64Build
:WXP64Build
:WXPI64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 32bit using WXP DDK
:XPBuild
:WXPBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W2K build for 32bit using WNET DDK
:WNETW2KBuild
:WNET2KBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% W2K %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 32bit using WNET DDK
:WNETXPBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% WXP
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 64bit using WNET DDK
:WNETXP64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64 WXP
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (Itanium) using WNET DDK
:WNET64Build
:WNETI64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64 WNET
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (x64) using WNET DDK
:WNETA64Build
:WNETAMD64Build
:WNETX64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% AMD64 WNET
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 32bit using WNET DDK
:WNETBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 32bit using WLH WDK
:WLHBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% WLH
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 64bit (x64) using WLH WDK
:WLHA64Build
:WLHX64Build
call :DetectVistaWDK
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% %OSR_AMD64FLAG% WLH
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 64bit (Itanium) using WLH WDK
:WLH64Build
:WLHI64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64 WLH
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (x64) using WLH WDK
:WLHNETA64Build
:WLHNETX64Build
call :DetectVistaWDK
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% %OSR_AMD64FLAG% WNET
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (Itanium) using WLH WDK
:WLHNET64Build
:WLHNETI64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64 WNET
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 32bit using WLH WDK
:WLHXPBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% WXP
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 64bit (Itanium) using WLH WDK
:WLHXP64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% 64 WXP
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W2K build for 32bit using WLH WDK
:WLH2KBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% W2K
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 32bit using WLH WDK
:WLHNETBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% WNET
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W7 build for 32bit using W7/2008 WDK
:W7Build
:WIN7Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x86 WIN7 %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W7 build for 64bit (x64) using W7/2008 WDK
:W7X64Build
:WIN7A64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x64 WIN7 %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: W7 build for 64bit (Itanium) using W7/2008 WDK
:W7I64Build
:WIN764Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% ia64 WIN7 no_oacr %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 32bit using W7/2008 WDK
:W7LHBuild
:WIN7WLHBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x86 WLH %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 64bit (x64) using W7/2008 WDK
:W7LHX64Build
:WIN7WLHA64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x64 WLH %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WLH build for 64bit (Itanium) using W7/2008 WDK
:W7LHI64Build
:WIN7WLH64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% ia64 WLH no_oacr %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 32bit using W7/2008 WDK
:W7NETBuild
:WIN7NETBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x86 WNET %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (x64) using W7/2008 WDK
:W7NETX64Build
:WIN7NETA64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x64 WNET %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WNET build for 64bit (Itanium) using W7/2008 WDK
:W7NETI64Build
:WIN7NET64Build
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% ia64 WNET no_oacr %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WXP build for 32bit using W7/2008 WDK
:W7XPBuild
:WIN7XPBuild
set OSR_CMDLINE="%%BASEDIR%%\bin\setenv.bat" %%BASEDIR%% %%BuildMode%% x86 WXP %SETTING_OACR% %SETTING_SEPARATE_OBJ_ROOT%
goto :EOF

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: All builds go here for the rest of the procedure. Now,
:: we are getting ready to call build. The big problem
:: here is to figure our the name of the buildxxx files
:: being generated for the different platforms.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:CommonBuild
:: Remove first command line arg
shift
call :SetMode %1
if not "%OSR_ERRCODE%" == "0" call :ShowErrorMsg %OSR_ERRCODE% "%ERR_BadMode%" & goto :USAGE
set OSR_BUILDNAME=%OSR_TARGET% (%BuildMode%) using the %BASEDIROS% DDK and %%%BASEDIRVAR%%%

call :CheckTargets %2
if "%OSR_ERRCODE%" == "6" call :ShowErrorMsg %OSR_ERRCODE% "%ERR_NoTarget%" & goto :USAGE
if not "%OSR_ERRCODE%" == "0" call :ShowErrorMsg %OSR_ERRCODE% "%ERR_NoDir%" & goto :USAGE

:: Resolve any variables in the command line string
call :ResolveVar OSR_CMDLINE

pushd .
set ERRORLEVEL=0
:: This external script prepares the build environment (e.g. setenv.bat)
call %OSR_CMDLINE%
:: Will only work with newer SETENV.BAT versions, but will be helpful in this case.
if not "%ERRORLEVEL%" == "0" call :ShowErrorMsg 9 "%ERR_SetEnvFailed%" & goto :USAGE
popd

:: ----------------------------------------------------------------------------
:: Setting global variables for the scope of this CMD session
set NO_BROWSER_FILE=
set NO_BINPLACE=
set buildDirectory=%~fs2
call :MakeShort buildDirectory "%buildDirectory%"
set buildDirectory_raw=%2
set buildDirectory_fname=%~n2
%OSR_TRACE% buildDirectory       == %buildDirectory%
%OSR_TRACE% buildDirectory_raw   == %buildDirectory_raw%
%OSR_TRACE% buildDirectory_fname == %buildDirectory_fname%

set mpFlag=-M
if "%BUILD_ALT_DIR%" == "" goto :NT4

:: W2K sets this!
set OSR_EXT=%BUILD_ALT_DIR%
set mpFlag=-MI

:NT4
if "%NUMBER_OF_PROCESSORS%" == "" set mpFlag=
if "%NUMBER_OF_PROCESSORS%" == "1" set mpFlag=

:: Set additional variables at this point or do whatever you please
@if exist "%buildDirectory%\%OSR_PREBUILD_SCRIPT%" @(
  %OSR_ECHO% ^>^> Performing pre-build steps [%OSR_PREBUILD_SCRIPT%] ...
  pushd "%buildDirectory%"
  call "%OSR_PREBUILD_SCRIPT%" > "%TEMP%\%OSR_PREBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"
  for /f "tokens=*" %%x in ('type "%TEMP%\%OSR_PREBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"') do @(
    %OSR_ECHO% %%x
  )
  if exist "%TEMP%\%OSR_PREBUILD_SCRIPT%_%OSR_RANDEXT%.tmp" del /f /q "%TEMP%\%OSR_PREBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"
  popd
  %OSR_ECHO% ^<^< Finished pre-build steps [%OSR_PREBUILD_SCRIPT%] ...
)
:: Save the current directory (before changing into the build directory!)
:: AFTERPREBUILD
pushd .

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Determine the settings of flags, WDF and PREFAST in
:: other words what was set for %3 and beyond....
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
%OSR_ECHO% %OSR_BUILDNAME%
set OSR_ARGS= + argument(s):
if not "%3" == "" set OSR_ARGS=%OSR_ARGS% %3
if not "%4" == "" set OSR_ARGS=%OSR_ARGS% %4
if not "%5" == "" set OSR_ARGS=%OSR_ARGS% %5
if /i "%OSR_ARGS%" == " + argument(s):" set OSR_ARGS=
%OSR_ECHO% Directory: %buildDirectory%%OSR_ARGS%
%OSR_ECHO% %BASEDIRVAR%: %BASEDIR%

cd /D %~s2
set bFlags=-Ze
set bscFlags=

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:ContinueParsing
if "%3" == "" goto :DONE
if "%3" == "/a" goto :RebuildallFound
if /i "%3" == "-WDF" goto :WDFFound
if /i "%3" == "-PREFAST" goto :PrefastFound
if /i "%3" == "-CUV" goto :CallUsageVerifier
set bscFlags=/n
set bFlags=%bFlags% %3
:: Remove next arg
shift
goto :ContinueParsing

:WDFFound
shift
:: Note, that the setwdf.bat is called from setenv.bat in the WDK,
:: therefore we skip it.
if /i "%BASEDIRVAR%" == "WLHBASE" goto :WDFOkay
if /i "%BASEDIRVAR%" == "W7BASE" goto :WDFOkay
if /i "%BASEDIRVAR%" == "WIN7BASE" goto :WDFOkay
if "%WDF_ROOT%" == "" call :ShowErrorMsg 2 "%ERR_NoWdfRoot%" & goto :USAGE
pushd .
if exist "%WDF_ROOT%\set_wdf_env.cmd" call "%WDF_ROOT%\set_wdf_env.cmd"
popd
:WDFOkay
goto :ContinueParsing

:PrefastFound
shift
set PREFAST_BUILD=1
goto :ContinueParsing

:CallUsageVerifier
shift
set VERIFIER_DDK_EXTENSIONS=1
set PREFAST_BUILD=1
goto :ContinueParsing

:RebuildallFound
shift
set bscFlags=/n
set bFlags=%bFlags:-Ze=-cfeZ%
set bFlags=%bFlags: -cZ=%
goto :ContinueParsing
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:DONE
for %%x in (build%OSR_EXT%.err build%OSR_EXT%.wrn build%OSR_EXT%.log prefast%OSR_EXT%.log) do @(
  if exist "%%x"   del /f /q "%%x"
)

if not "%PREFAST_BUILD%" == "0" goto :RunPrefastBuild
%OSR_ECHO% Run build %mpFlag% %bFlags% for %BuildMode% version in %buildDirectory_raw%
pushd .
build %mpFlag% %bFlags%
popd
goto :BuildComplete

:RunPrefastBuild
%OSR_ECHO% Run prefast build %mpFlag% %bFlags% for %BuildMode% version in %buildDirectory_raw%
setlocal ENABLEEXTENSIONS & pushd .
set PREFASTLOG=PREfast_defects_%OSR_EXT%.xml
prefast /log=%PREFASTLOG% /reset build %mpFlag% %bFlags% > NUL 2>&1
if "%errorlevel%" GTR "0" set OSR_ERRCODE=%errorlevel%
prefast /log=%PREFASTLOG% list > prefast%OSR_EXT%.log
%OSR_ECHO% The PREfast logfile is ^"%prefastlog%^"!
popd & endlocal

:BuildComplete
if not "%errorlevel%" == "0" set OSR_ERRCODE=%errorlevel%

@echo %OSR_DEBUG%
:: Assume that the onscreen errors are complete!
setlocal
set WARNING_FILE_COUNT=0
if exist "build%OSR_EXT%.log" for /f "tokens=*" %%x in ('findstr "warning[^.][CDMRU][0-9][0-9]* warning[^.][BRP][KCWG][0-9][0-9]* warning[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]* error[^.][CDMRU][0-9][0-9]* error[^.][BRP][KCWG][0-9][0-9]* error[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]*" "build%OSR_EXT%.log"') do @(
  set /a WARNING_FILE_COUNT=%WARNING_FILE_COUNT%+1
)
if not "%WARNING_FILE_COUNT%" == "0" (
  %OSR_ECHO% ================ Build warnings =======================
  if exist "build%OSR_EXT%.log" for /f "tokens=*" %%x in ('findstr "warning[^.][CDMRU][0-9][0-9]* warning[^.][BRP][KCWG][0-9][0-9]* warning[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]* error[^.][CDMRU][0-9][0-9]* error[^.][BRP][KCWG][0-9][0-9]* error[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]*" "build%OSR_EXT%.log"') do @(
    @echo %%x
  )
)
set WARNING_FILE_COUNT_PRE=0
if exist "prefast%OSR_EXT%.log" for /f "tokens=*" %%x in ('findstr "warning[^.][CDMRU][0-9][0-9]* warning[^.][BRP][KCWG][0-9][0-9]* warning[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]*" "prefast%OSR_EXT%.log"') do @(
  set /a WARNING_FILE_COUNT_PRE=%WARNING_FILE_COUNT_PRE%+1
)
:: Reset if this is no PREfast build
if "%PREFAST_BUILD%" == "0" set WARNING_FILE_COUNT_PRE=0
if not "%WARNING_FILE_COUNT_PRE%" == "0" (
  %OSR_ECHO% =============== PREfast warnings ======================
  if exist "prefast%OSR_EXT%.log" for /f "tokens=*" %%x in ('findstr "warning[^.][CDMRU][0-9][0-9]* warning[^.][BRP][KCWG][0-9][0-9]* warning[^.][ACLPS][DNRTVX][JKLTX][0-9][0-9]*" "prefast%OSR_EXT%.log"') do @(
    @echo %%x
  )
)
set /a WARNING_FILE_COUNT=%WARNING_FILE_COUNT%+%WARNING_FILE_COUNT_PRE%
if not "%WARNING_FILE_COUNT%" == "0" (
  %OSR_ECHO% =======================================================
)
endlocal
@echo.
%OSR_ECHO% Build complete
%OSR_ECHO% Building browse information files
if exist "buildbrowse.cmd" call "buildbrowse.cmd" & goto :postBuildSteps
set sbrlist=sbrList.txt
if not exist sbrList%CPU%.txt goto :sbrDefault
set sbrlist=sbrList%CPU%.txt

:sbrDefault
if not exist %sbrlist% goto :postBuildSteps
:: Prepend blank space
if not "%bscFlags%" == "" set bscFlags= %bscFlags%
:: bscmake%bscFlags% prevents a double blank space ...
bscmake%bscFlags% @%sbrlist%

:: Perform whatever post-build steps
:postBuildSteps
:: Restore the current directory (after changing into the build directory!)
:: Search upwards for "AFTERPREBUILD" to find the corresponding PUSHD
popd
@if exist "%buildDirectory%\%OSR_POSTBUILD_SCRIPT%" @(
  %OSR_ECHO% ^>^> Performing post-build steps [%OSR_POSTBUILD_SCRIPT%] ...
  pushd "%buildDirectory%"
  call "%OSR_POSTBUILD_SCRIPT%" > "%TEMP%\%OSR_POSTBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"
  for /f "tokens=*" %%x in ('type "%TEMP%\%OSR_POSTBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"') do @(
    %OSR_ECHO% %%x
  )
  if exist "%TEMP%\%OSR_POSTBUILD_SCRIPT%_%OSR_RANDEXT%.tmp" del /f /q "%TEMP%\%OSR_POSTBUILD_SCRIPT%_%OSR_RANDEXT%.tmp"
  popd
  %OSR_ECHO% ^<^< Finished post-build steps [%OSR_POSTBUILD_SCRIPT%] ...
)
goto :END
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ MAIN function of the script
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  / GetCustomEnvironment
::    First parameter is the "directory" that supposedly contains the SOURCES
::    or DIRS file (and the build scripts)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:GetCustomEnvironment
pushd .
call :CheckTargets "%~f1"
@if not "%OSR_ERRCODE%" == "0" @(
  echo.
  %OSR_ECHO% The target directory seemed to not contain a DIRS or SOURCES file
  %OSR_ECHO% when trying to set a custom environment! Quitting. ^(ERROR #%OSR_ERRCODE%^)
  set buildDirectory=%~f1
  if "%OSR_ERRCODE%" == "6" call :ShowErrorMsg %OSR_ERRCODE% "%ERR_NoTarget%" & goto :GetCustomEnvironment_ret
  call :ShowErrorMsg %OSR_ERRCODE% "%ERR_NoDir%" & goto :GetCustomEnvironment_ret
  goto :GetCustomEnvironment_ret
)
:: If the user provided a script to customize the environment, execute it.
@if exist "%~f1\%OSR_SETENV_SCRIPT%" @(
  %OSR_ECHO% ^>^> Setting custom environment variables [%OSR_SETENV_SCRIPT%] ...
  pushd "%~f1"
  call "%OSR_SETENV_SCRIPT%" > "%TEMP%\%OSR_SETENV_SCRIPT%_%OSR_RANDEXT%.tmp"
  for /f "tokens=*" %%x in ('type "%TEMP%\%OSR_SETENV_SCRIPT%_%OSR_RANDEXT%.tmp"') do @(
    %OSR_ECHO% %%x
  )
  if exist "%TEMP%\%OSR_SETENV_SCRIPT%_%OSR_RANDEXT%.tmp" del /f /q "%TEMP%\%OSR_SETENV_SCRIPT%_%OSR_RANDEXT%.tmp"
  popd
  %OSR_ECHO% ^<^< Finished setting custom environment variables [%OSR_SETENV_SCRIPT%] ...
)
:GetCustomEnvironment_ret
popd
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  \ GetCustomEnvironment
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  / SetMode
::    Subroutine to validate the mode of the build passed in. It must be free,
::    FREE, fre, FRE or checked, CHECKED, chk, CHK. Anything else is an error.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:SetMode
set BuildMode=
if /i "%OSR_TARGET%" == "WLH2K" goto :SetModeWLH2K
for %%f in (free fre) do if /i "%%f" == "%1" set BuildMode=free
for %%f in (checked chk) do if /i "%%f" == "%1" set BuildMode=checked
goto :SetModeCommonEnd
:SetModeWLH2K
for %%f in (free fre) do if /i "%%f" == "%1" set BuildMode=f
for %%f in (checked chk) do if /i "%%f" == "%1" set BuildMode=c
:SetModeCommonEnd
%OSR_TRACE% Mode set to ^"%BuildMode%^"
if "%BuildMode%" == "" set OSR_ERRCODE=5
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  \ SetMode
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / CheckTargets subroutine
::   Subroutine to validate that the target directory exists and that there is
::   either a DIRS or SOURCES and MakeFile in it.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CheckTargets
:: Building "stack frame"
setlocal & pushd . & set OSR_ERRCODE=0
set lTarget=%~1
if not "%lTarget%" == "" goto :CheckTargets1
set OSR_ERRCODE=7
goto :CheckTargets_ret
:CheckTargets1
if exist "%lTarget%" goto :CheckTargets2
set OSR_ERRCODE=8
goto :CheckTargets_ret
:CheckTargets2
if not exist "%lTarget%\DIRS" goto :CheckTargets3
set OSR_ERRCODE=0
goto :CheckTargets_ret
:CheckTargets3
if exist "%lTarget%\SOURCES" goto :CheckTargets4
set OSR_ERRCODE=6
goto :CheckTargets_ret
:CheckTargets4
if exist "%lTarget%\MAKEFILE" goto :CheckTargets5
set OSR_ERRCODE=6
goto :CheckTargets_ret
:CheckTargets5
set OSR_ERRCODE=0
:CheckTargets_ret
:: Cleaning "stack frame" and returning error code into global scope
popd & endlocal & set OSR_ERRCODE=%OSR_ERRCODE%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ CheckTargets subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / ResolveVar subroutine
::   There is only one parameter, the name of the variable to be resolved!
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:ResolveVar
:: Get the name of the variable we are working with
setlocal ENABLEEXTENSIONS & set VAR_NAME=%1
set VAR_TEMPRET2=%%%VAR_NAME%%%
:ResolveVarLoop
set VAR_TEMPRET1=%VAR_TEMPRET2%
set VAR_TEMPRET2=%VAR_TEMPRET1%
for /f "tokens=*" %%i in ('echo %VAR_TEMPRET1%') do (
  set VAR_TEMPRET2=%%i
)
if not "%VAR_TEMPRET1%" == "%VAR_TEMPRET2%" goto :ResolveVarLoop
:: Re-export the variable out of the local scope
endlocal & set %VAR_NAME%=%VAR_TEMPRET1%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ ResolveVar subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / MakeShort subroutine
::   Two parameters. First parameter is the variable name, second is the path
::   to convert into a short filename.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:MakeShort
setlocal ENABLEEXTENSIONS
:: Get the name of the variable we are working with and the path to convert
set VAR_NAME=%~1
set PATH_SHORT=%~dpns2
set PATH_EXTSHORT=%~xs2
if not "" == "%PATH_EXTSHORT%" set PATH_EXTSHORT=%PATH_EXTSHORT:~0,4%
set PATH_SHORT=%PATH_SHORT%%PATH_EXTSHORT%
endlocal & set %VAR_NAME%=%PATH_SHORT%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ MakeShort subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / ErrorWithUsage subroutine
::   This one will take the passed in parameters and build a nice error
::   message which is returned to the user along with the usage hints.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:ShowErrorMsg
@set OSR_ERRCODE=%~1
@set OSR_ERRMSG=%~2
@set OSR_ERRMSG=%OSR_ERRMSG:'="%
@set OSR_ERRMSG=ERROR #%OSR_ERRCODE%: %OSR_ERRMSG%
@echo.
%OSR_ECHO% %OSR_ERRMSG%
if DEFINED buildDirectory %OSR_ECHO% -^> Target directory: %buildDirectory%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ ErrorWithUsage subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / SetVar subroutine
::   Param1 == name of the variable, Param2 == value to be set for the variable
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:SetVar
:: Get the name of the variable we are working with
setlocal ENABLEEXTENSIONS & set VAR_NAME=%1
endlocal & set %VAR_NAME%=%~2
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ SetVar subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / DetectVistaWDK subroutine
::   No parameters expected
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:DetectVistaWDK
setlocal ENABLEEXTENSIONS
:: Newer flag (starting with W2K8) is default
set OSR_AMD64FLAG=x64
:: The Vista WDK accepted *only* "AMD64", the newer W2K8 WDK accepts only "x64"
:: We detect the older one by checking the setenv.bat for a certain string
findstr /C:"Windows Server Longhorn" "%BASEDIR%\bin\setenv.bat" > NUL 2>&1 && set OSR_AMD64FLAG=AMD64
endlocal & set OSR_AMD64FLAG=%OSR_AMD64FLAG%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ DetectVistaWDK subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / DetectBaseDirTemp subroutine
::   The first parameter is the list of directory names to check, separated by
::   blank spaces.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:DetectBaseDirTemp
:: Get the name of the variable we are working with
if "%~1" == "" goto :EOF
setlocal ENABLEEXTENSIONS
call :CommonCheckMsg1
:: Try to find an installed DDK/WDK from the registry keys
if DEFINED OSR_REGAVAILABLE if not "%OSR_REGAVAILABLE%" == "0" (
  for %%i in (%~1) do @(
    call :RegTryBaseDirTemp "%%i"
  )
)
:: Try all the "default" locations
if not DEFINED BASEDIRTEMP (
  for %%i in (%~1) do @(
    for %%a in (WINDDK DDK) do @(
      call :BruteTryBaseDirTemp "%SystemDrive%\%%a\%%i"
      call :BruteTryBaseDirTemp "%ProgramFiles%\%%a\%%i"
    )
  )
)
endlocal & set BASEDIRTEMP=%BASEDIRTEMP%
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ DetectBaseDirTemp subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / RegTryBaseDirTemp subroutine
::   Attempt to find the install key in the registry.
::   This functions tests old-style DDKs and new-style WDKs.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:RegTryBaseDirTemp
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :EOF
setlocal ENABLEEXTENSIONS
call :RegTryBaseDirTempSingle "%~1" "LFNDirectory" BASEDIRTEMP
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :RegTryBaseDirTemp_EOF
call :RegTryBaseDirTempSingle "%~1\Setup" "BUILD" BASEDIRTEMP
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :RegTryBaseDirTemp_EOF
if not DEFINED BASEDIRTEMP (endlocal & goto :EOF)
:RegTryBaseDirTemp_EOF
%OSR_ECHO% Found directory (%BASEDIRTEMP%) from install key
endlocal & set BASEDIRTEMP=%BASEDIRTEMP% & goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ RegTryBaseDirTemp subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / RegTryBaseDirTempSingle subroutine
::   Attempt to find the install key in the registry.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:RegTryBaseDirTempSingle
setlocal ENABLEEXTENSIONS
set REGSUBKEY=%~1
set REGVALUE=%~2
set VARIABLETOSET=%~3
set REGMAINKEY=HKLM\SOFTWARE\Microsoft\WINDDK
:: Test whether we can read the value below this key
reg query "%REGMAINKEY%\%REGSUBKEY%" /v "%REGVALUE%" > NUL 2>&1 || goto :RegTryBaseDirTempSingle_WOW64
for /f "tokens=2*" %%i in ('reg query "%REGMAINKEY%\%REGSUBKEY%" /v "%REGVALUE%"^|findstr /C:"%REGVALUE%"') do @(
  call :SetVar _SETVARIABLE "%%j"
)
endlocal & set %VARIABLETOSET%=%_SETVARIABLE%
:RegTryBaseDirTempSingle_WOW64
set REGMAINKEY=HKLM\SOFTWARE\Wow6432Node\Microsoft\WINDDK
:: Test whether we can read the value below this key
reg query "%REGMAINKEY%\%REGSUBKEY%" /v "%REGVALUE%" > NUL 2>&1 || goto :RegTryBaseDirTempSingle_EOF
for /f "tokens=2*" %%i in ('reg query "%REGMAINKEY%\%REGSUBKEY%" /v "%REGVALUE%"^|findstr /C:"%REGVALUE%"') do @(
  call :SetVar _SETVARIABLE "%%j"
)
endlocal & set %VARIABLETOSET%=%_SETVARIABLE%
:RegTryBaseDirTempSingle_EOF
endlocal
goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ RegTryBaseDirTempSingle subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: / BruteTryBaseDirTemp subroutine
::   Brute-force test the given directory.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:BruteTryBaseDirTemp
if DEFINED BASEDIRTEMP if exist "%BASEDIRTEMP%" goto :EOF
setlocal ENABLEEXTENSIONS
:: We will not overwrite BASETEMPDIR if it has been set and is valid
:: Just try
set BASEDIRTEMP=%~1
%OSR_ECHO% Trying %BASEDIRTEMP% ...
if not exist "%BASEDIRTEMP%" (endlocal & goto :EOF)
endlocal & set BASEDIRTEMP=%BASEDIRTEMP% & goto :EOF
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: \ BruteTryBaseDirTemp subroutine
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Usage output
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:USAGE
@echo.
@echo USAGE:
@echo ======
@echo   %~n0 ^<target^> ^<build type^> ^<directory^> [flags] [-WDF] [-PREFAST] [-CUV]
@echo.
@echo Values for ^<target^>:
@echo    ---------------------------------------------------------------------------
@echo     Target OS version and architecture  ^| Miscellaneous
@echo    -------------------------------------^|-------------------------------------
@echo     ^<target^>    ^| Windows     ^| CPU     ^| Base directory ^| ^<target^> alias(es)
@echo    -------------^|-------------^|---------^|----------------^|--------------------
@echo     -NT4        ^| NT 4.0      ^| x86     ^| %%NT4BASE%%      ^|
@echo     -W2K        ^| 2000        ^| x86     ^| %%W2KBASE%%      ^|
@echo     -W2K64      ^| 2000        ^| Itanium ^| %%W2KBASE%%      ^| -W2KI64
@echo     -WXP        ^| XP          ^| x86     ^| %%WXPBASE%%      ^| -XP
@echo     -WXP64      ^| XP          ^| Itanium ^| %%WXPBASE%%      ^| -WXPI64, -XP64
@echo     -WXP2K      ^| 2000        ^| x86     ^| %%WXPBASE%%      ^| -XPW2K
@echo     -WNET       ^| 2003        ^| x86     ^| %%WNETBASE%%     ^|
@echo     -WNET64     ^| 2003        ^| Itanium ^| %%WNETBASE%%     ^| -WNETI64
@echo     -WNETXP     ^| XP          ^| x86     ^| %%WNETBASE%%     ^|
@echo     -WNETXP64   ^| XP          ^| Itanium ^| %%WNETBASE%%     ^|
@echo     -WNETAMD64  ^| 2003/XP x64 ^| x64     ^| %%WNETBASE%%     ^| -WNETX64, -WNETA64
@echo     -WNET2K     ^| 2000 SP3    ^| x86     ^| %%WNETBASE%%     ^| -WNETW2K
@echo     -WLH        ^| Vista/2008  ^| x86     ^| %%WLHBASE%%      ^|
@echo     -WLH2K      ^| 2000 SP4    ^| x86     ^| %%WLHBASE%%      ^|
@echo     -WLHXP      ^| XP          ^| x86     ^| %%WLHBASE%%      ^|
@echo     -WLHXP64    ^| XP          ^| Itanium ^| %%WLHBASE%%      ^|
@echo     -WLHNET     ^| 2003        ^| x86     ^| %%WLHBASE%%      ^|
@echo     -WLHNETI64  ^| 2003        ^| Itanium ^| %%WLHBASE%%      ^| -WLHNET64
@echo     -WLHNETX64  ^| 2003/XP x64 ^| x64     ^| %%WLHBASE%%      ^| -WLHNETA64
@echo     -WLHI64     ^| Vista/2008  ^| Itanium ^| %%WLHBASE%%      ^| -WLH64
@echo     -WLHX64     ^| Vista/2008  ^| x64     ^| %%WLHBASE%%      ^| -WLHA64
@echo     -W7         ^| 7/2008 R2   ^| x86     ^| %%W7BASE%%       ^| -WIN7
@echo     -W7I64      ^| 7/2008 R2   ^| Itanium ^| %%W7BASE%%       ^| -WIN764
@echo     -W7X64      ^| 7/2008 R2   ^| x64     ^| %%W7BASE%%       ^| -WIN7A64
@echo     -W7LH       ^| Vista/2008  ^| x86     ^| %%W7BASE%%       ^| -WIN7WLH
@echo     -W7LHI64    ^| Vista/2008  ^| Itanium ^| %%W7BASE%%       ^| -WIN7WLH64
@echo     -W7LHX64    ^| Vista/2008  ^| x64     ^| %%W7BASE%%       ^| -WIN7WLHA64
@echo     -W7NET      ^| 2003        ^| x86     ^| %%W7BASE%%       ^| -WIN7NET
@echo     -W7NETI64   ^| 2003        ^| Itanium ^| %%W7BASE%%       ^| -WIN7NET64
@echo     -W7NETX64   ^| 2003/XP x64 ^| x64     ^| %%W7BASE%%       ^| -WIN7NETA64
@echo     -W7XP       ^| XP          ^| x86     ^| %%W7BASE%%       ^| -WIN7XP
@echo    ---------------------------------------------------------------------------
@echo     Support for NT4 and W2K DDKs is deprecated and not checked anymore
@echo     in new versions. It may or may not work properly.
@echo    ---------------------------------------------------------------------------
@echo.
@echo Values for ^<build type^>:
@echo       checked, chk     indicates a checked build
@echo       free, fre        indicates a free build
@echo.
@echo Remaining parameters (all optional):
@echo       ^<directory^>      path to build directory, try . (current directory)
@echo       [flags]          any flags you think should be passed to build (try /a
@echo                        for clean)
@echo       -WDF             performs a WDF build
@echo       -PREFAST         performs a PREFAST build
@echo       -CUV             uses the Call Usage Verifier, implies a PREFAST build
@echo.
@echo Special files:
@echo       The build target directory (where the DIRS or SOURCES file resides) can
@echo       contain the following files:
@echo       - %OSR_PREBUILD_SCRIPT%
@echo         Allows to include a step before the BUILD tool from the DDK is called
@echo         but after the environment for the respective DDK has been set!
@echo       - %OSR_POSTBUILD_SCRIPT%
@echo         Allows to include a step after the BUILD tool from the DDK is called,
@echo         so the environment is still available to the script.
@echo       - %OSR_SETENV_SCRIPT%
@echo         Allows to set (or override) _any_ environment variables that may exist
@echo         in the global environment. Thus you can set the base directory for the
@echo         DDK from inside this script, making your project more self-contained.
@echo.
@echo       DDKBUILD will only handle those files which exist, so you may choose to
@echo       use none, one or multiple of these script files.
@echo       (All scripts execute inside their current directory. Consider this!)
@echo.
@echo Examples:
@echo       ^"%~n0 -NT4 checked .^" (for NT4 BUILD)
@echo       ^"%~n0 -WXP64 chk .^"
@echo       ^"%~n0 -WXP chk c:\projects\myproject^"
@echo       ^"%~n0 -WNET64 chk .^"      (IA64 build)
@echo       ^"%~n0 -WNETAMD64 chk .^"   (AMD64/EM64T build)
@echo       ^"%~n0 -WNETXP chk . -cZ -WDF^"
@echo       ^"%~n0 -WNETXP chk . -cZ -PREFAST^"
@echo.
@echo       In order for this procedure to work correctly for each platform, it
@echo       requires an environment variable to be set up for certain platforms.
@echo       There is an auto-detection mechanism in this script, which will work best
@echo       if the DDK/WDK was installed using the normal installer (i.e. not just
@echo       copied). The auto-detection is based on the DDK/WDK for which you request
@echo       a build. Whenever you set the variable explicitly, this will take
@echo       precedence over the auto-detected path!
@echo       The environment variables are as follows:
@echo.
@echo       %%NT4BASE%%  - Set this up for ^"-NT4^" builds
@echo       %%W2KBASE%%  - Set this up for ^"-W2K^" and ^"-W2K64^" builds
@echo       %%WXPBASE%%  - Set this up for ^"-WXP^", ^"-WXP64^", ^"-WXP2K^" builds
@echo       %%WNETBASE%% - Set this up for ^"-WNET*^" builds
@echo       %%WLHBASE%%  - Set this up for ^"-WLH*^" builds
@echo       %%W7BASE%%   - Set this up for ^"-W7*^" builds
@echo.
@echo       %%WDF_ROOT%% must be set if attempting to do a WDF Build previous to the
@echo       Vista WDK (in later DDKs there is no need to set WDF_ROOT).
@echo.
@echo Path to this script:
@echo      %~f0
@echo.
@echo   %OSR_VERSTR%
@echo   -^> report any problems to ^<info@osr.com^> or ^<http://assarbad.net/contact/^>
@echo.

:END
popd & endlocal & exit /b %OSR_ERRCODE%
