<#
.SYNOPSIS
    Build script for Ext4Fsd driver and user-mode tools from source

.DESCRIPTION
    Builds the Ext4Fsd driver, Ext2Srv service, and/or Ext2Mgr application.
    Full solution builds require Visual Studio 2019/2022 and the Windows Driver Kit (WDK).
    Ext2Mgr / Ext2Srv targets need MSVC + MFC only (no WDK).

.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)

.PARAMETER Platform
    Target platform: x64, x86, ARM, or ARM64 (default: x64)

.PARAMETER Target
    What to build:
      All     - full solution (driver + Ext2Srv + Ext2Mgr); WDK expected
      Ext2Mgr - classic MFC Ext2Mgr.exe only (no WDK)
      Ext2Srv - Ext2Srv.exe only (no WDK)

.PARAMETER Clean
    Clean before building

.EXAMPLE
    .\Scripts\build.ps1
    Builds Release x64 full solution (run from repo root)

.EXAMPLE
    .\Scripts\build.ps1 -Target Ext2Mgr
    Builds only Ext2Mgr.exe (Release x64)

.EXAMPLE
    .\Scripts\build.ps1 -Target Ext2Mgr -Configuration Debug -Platform x64
    Builds Debug x64 Ext2Mgr.exe

.EXAMPLE
    .\Scripts\build.ps1 -Configuration Debug -Platform x64
    Builds Debug x64 full solution
#>

[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "x86", "ARM", "ARM64")]
    [string]$Platform = "x64",

    [ValidateSet("All", "Ext2Mgr", "Ext2Srv")]
    [string]$Target = "All",

    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$script:BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Get-MSBuildPlatform([string]$platform) {
    # Solution uses x86; vcxproj configs use Win32 for that arch.
    if ($platform -eq "x86") { return "Win32" }
    return $platform
}

function Get-OutArchFolder([string]$platform) {
    switch ($platform) {
        "x64" { return "x64" }
        "ARM64" { return "arm64" }
        "ARM" { return "arm" }
        "x86" { return "x86" }
        default { return $platform.ToLowerInvariant() }
    }
}

function Get-SolutionDirProperty {
    $solutionDir = $script:BuildRoot
    if (-not $solutionDir.EndsWith("\")) {
        $solutionDir += "\"
    }
    return $solutionDir
}

$header = " Ext4Fsd Build Script"
Write-Host ("=" * ($header.Length + 1)) -ForegroundColor Cyan
Write-Host $header -ForegroundColor Cyan
Write-Host ("=" * ($header.Length + 1)) -ForegroundColor Cyan
Write-Host ""

# Find Visual Studio installation (including Build Tools)
Write-Host "Searching for Visual Studio or Build Tools..." -ForegroundColor Yellow
$vsInstallPath = $null
$vsVersions = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\Professional",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\Enterprise",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\BuildTools",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools"
)

foreach ($path in $vsVersions) {
    if (Test-Path $path) {
        $vsInstallPath = $path
        Write-Host "Found Visual Studio at: $vsInstallPath" -ForegroundColor Green
        break
    }
}

if (-not $vsInstallPath) {
    Write-Host "ERROR: Visual Studio 2019/2022 or Build Tools not found!" -ForegroundColor Red
    Write-Host "Please install Visual Studio 2019/2022 or Build Tools with C++ desktop development workload." -ForegroundColor Yellow
    exit 1
}

# Find MSBuild
$msbuildPath = Join-Path $vsInstallPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuildPath)) {
    $msbuildPath = Join-Path $vsInstallPath "MSBuild\15.0\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuildPath)) {
        Write-Host "ERROR: MSBuild not found!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Using MSBuild: $msbuildPath" -ForegroundColor Green

$needsWdk = ($Target -eq "All")
if ($needsWdk) {
    Write-Host ""
    Write-Host "Checking for Windows Driver Kit (WDK)..." -ForegroundColor Yellow
    $wdkPaths = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\Include",
        "${env:ProgramFiles}\Windows Kits\10\Include"
    )

    $wdkFound = $false
    foreach ($path in $wdkPaths) {
        if (Test-Path $path) {
            $wdkFound = $true
            Write-Host "Found WDK at: $path" -ForegroundColor Green
            break
        }
    }

    if (-not $wdkFound) {
        Write-Host "WARNING: Windows Driver Kit (WDK) not found!" -ForegroundColor Yellow
        Write-Host "The driver project requires WDK to build." -ForegroundColor Yellow
        Write-Host "Download from: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk" -ForegroundColor Yellow
        Write-Host "Tip: use -Target Ext2Mgr or -Target Ext2Srv to build user-mode only (no WDK)." -ForegroundColor Cyan
        Write-Host ""
        if ([Environment]::UserInteractive -and -not [Console]::IsInputRedirected) {
            $continue = Read-Host "Continue anyway? (y/N)"
            if ($continue -ne "y" -and $continue -ne "Y") {
                exit 1
            }
        } else {
            Write-Host "Non-interactive session: continuing without WDK (driver build may fail)." -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "Target ${Target}: WDK not required." -ForegroundColor Green
}

$solutionPath = Join-Path $script:BuildRoot "Ext4Fsd.sln"
$solutionDir = Get-SolutionDirProperty
$msbuildPlatform = if ($Target -eq "All") { $Platform } else { Get-MSBuildPlatform $Platform }
$outArch = Get-OutArchFolder $Platform

switch ($Target) {
    "All" {
        $projectPath = $solutionPath
        if (-not (Test-Path $projectPath)) {
            Write-Host "ERROR: Solution file not found: $projectPath" -ForegroundColor Red
            exit 1
        }
        $msbuildTarget = "Build"
        $outputDirs = @(
            "Ext4Fsd\$Configuration\$Platform",
            "Ext2Srv\$Configuration\$outArch",
            "Ext2Mgr\$Configuration\$outArch"
        )
    }
    "Ext2Mgr" {
        $projectPath = Join-Path $script:BuildRoot "Ext2Mgr\Ext2Mgr.vcxproj"
        if (-not (Test-Path $projectPath)) {
            Write-Host "ERROR: Ext2Mgr project not found: $projectPath" -ForegroundColor Red
            exit 1
        }
        $msbuildTarget = "Build"
        $outputDirs = @("Ext2Mgr\$Configuration\$outArch")
    }
    "Ext2Srv" {
        $projectPath = Join-Path $script:BuildRoot "Ext2Srv\Ext2Srv.vcxproj"
        if (-not (Test-Path $projectPath)) {
            Write-Host "ERROR: Ext2Srv project not found: $projectPath" -ForegroundColor Red
            exit 1
        }
        $msbuildTarget = "Build"
        $outputDirs = @("Ext2Srv\$Configuration\$outArch")
    }
}

Write-Host ""
Write-Host "Build Configuration:" -ForegroundColor Cyan
Write-Host "  Target:        $Target" -ForegroundColor White
Write-Host "  Configuration: $Configuration" -ForegroundColor White
Write-Host "  Platform:      $Platform (MSBuild: $msbuildPlatform)" -ForegroundColor White
Write-Host "  Project:       $projectPath" -ForegroundColor White
Write-Host ""

$commonProps = @(
    "/p:Configuration=$Configuration"
    "/p:Platform=$msbuildPlatform"
    "/v:normal"
    "/m"
    "/nologo"
)
if ($Target -ne "All") {
    $commonProps += "/p:SolutionDir=$solutionDir"
}

if ($Clean) {
    Write-Host "Cleaning..." -ForegroundColor Yellow
    & $msbuildPath $projectPath (@("/t:Clean") + $commonProps + @("/v:minimal"))
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: Clean failed, but continuing..." -ForegroundColor Yellow
    }
    Write-Host ""
}

Write-Host "Building $Target..." -ForegroundColor Yellow
Write-Host ""

$buildArgs = @($projectPath, "/t:$msbuildTarget") + $commonProps
& $msbuildPath @buildArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    $msg = " Build completed successfully!"
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Green
    Write-Host $msg -ForegroundColor Green
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Green
    Write-Host ""

    Write-Host "Output files:" -ForegroundColor Cyan
    foreach ($dir in $outputDirs) {
        $fullPath = Join-Path $script:BuildRoot $dir
        if (Test-Path $fullPath) {
            Write-Host "  $fullPath" -ForegroundColor White
            Get-ChildItem $fullPath -File | ForEach-Object {
                Write-Host "    - $($_.Name)" -ForegroundColor Gray
            }
        }
    }

    if ($Target -eq "All") {
        $driverPath = Join-Path $script:BuildRoot "Ext4Fsd\$Configuration\$Platform\Ext2Fsd.sys"
        if (Test-Path $driverPath) {
            Write-Host ""
            $signScript = Join-Path $script:BuildRoot "Scripts\sign_driver.ps1"
            if (Test-Path $signScript) {
                Write-Host "Running post-build driver signing..." -ForegroundColor Cyan
                & $signScript -DriverPath $driverPath -SkipIfMissing
            }
        }

        Write-Host ""
        Write-Host "Next Steps:" -ForegroundColor Cyan
        Write-Host "  To sign the driver (optional):" -ForegroundColor Yellow
        Write-Host "    set EXT4FSD_CERT_PATH and MSIX_CERT_PASSWORD, then .\Scripts\sign_driver.ps1" -ForegroundColor White
        Write-Host ""
        Write-Host "  To install the driver:" -ForegroundColor Yellow
        Write-Host "    .\Scripts\install_driver.ps1" -ForegroundColor White
        Write-Host "    (Requires administrator privileges)" -ForegroundColor Gray
        Write-Host ""
        Write-Host "  See INSTALLATION.md for detailed instructions" -ForegroundColor Cyan
    } elseif ($Target -eq "Ext2Mgr") {
        $exePath = Join-Path $script:BuildRoot "Ext2Mgr\$Configuration\$outArch\Ext2Mgr.exe"
        Write-Host ""
        Write-Host "Run (as Administrator):" -ForegroundColor Cyan
        Write-Host "  $exePath" -ForegroundColor White
    } elseif ($Target -eq "Ext2Srv") {
        $exePath = Join-Path $script:BuildRoot "Ext2Srv\$Configuration\$outArch\Ext2Srv.exe"
        Write-Host ""
        Write-Host "Built:" -ForegroundColor Cyan
        Write-Host "  $exePath" -ForegroundColor White
    }

} else {
    Write-Host ""
    $msg = " Build failed!"
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Red
    Write-Host $msg -ForegroundColor Red
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Red
    exit 1
}
