<#
.SYNOPSIS
    Build script for Ext4Fsd driver from source
    
.DESCRIPTION
    This script builds the Ext4Fsd driver, Ext2Srv service, and Ext2Mgr application
    from source. It requires Visual Studio 2019/2022 and Windows Driver Kit (WDK).
    
.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)
    
.PARAMETER Platform
    Target platform: x64, x86, ARM, or ARM64 (default: x64)
    
.PARAMETER Clean
    Clean the solution before building
    
.EXAMPLE
    .\Scripts\build.ps1
    Builds Release x64 configuration (run from repo root)

.EXAMPLE
    .\Scripts\build.ps1 -Configuration Debug -Platform x64
    Builds Debug x64 configuration
#>

[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    
    [ValidateSet("x64", "x86", "ARM", "ARM64")]
    [string]$Platform = "x64",
    
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$script:BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

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
    # Try older path
    $msbuildPath = Join-Path $vsInstallPath "MSBuild\15.0\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuildPath)) {
        Write-Host "ERROR: MSBuild not found!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Using MSBuild: $msbuildPath" -ForegroundColor Green

# Check for WDK
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
    Write-Host ""
    $continue = Read-Host "Continue anyway? (y/N)"
    if ($continue -ne "y" -and $continue -ne "Y") {
        exit 1
    }
}

# Solution file
$solutionPath = Join-Path $script:BuildRoot "Ext4Fsd.sln"
if (-not (Test-Path $solutionPath)) {
    Write-Host "ERROR: Solution file not found: $solutionPath" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Build Configuration:" -ForegroundColor Cyan
Write-Host "  Configuration: $Configuration" -ForegroundColor White
Write-Host "  Platform: $Platform" -ForegroundColor White
Write-Host "  Solution: $solutionPath" -ForegroundColor White
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning solution..." -ForegroundColor Yellow
    & $msbuildPath $solutionPath /t:Clean /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: Clean failed, but continuing..." -ForegroundColor Yellow
    }
    Write-Host ""
}

# Build the solution
Write-Host "Building solution..." -ForegroundColor Yellow
Write-Host ""

$buildArgs = @(
    $solutionPath
    "/p:Configuration=$Configuration"
    "/p:Platform=$Platform"
    "/t:Build"
    "/v:normal"
    "/m"  # Parallel build
    "/nologo"
)

& $msbuildPath @buildArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    $msg = " Build completed successfully!"
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Green
    Write-Host $msg -ForegroundColor Green
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Green
    Write-Host ""
    
    # Show output locations
    $outputDirs = @(
        "Ext4Fsd\$Configuration\$Platform",
        "Ext2Srv\$Configuration\$Platform",
        "Ext2Mgr\$Configuration\$Platform"
    )
    
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
    
    # Post-build: Sign driver if certificate is configured
    $driverPath = Join-Path $script:BuildRoot "Ext4Fsd\$Configuration\$Platform\Ext2Fsd.sys"
    if (Test-Path $driverPath) {
        Write-Host ""
        $signScript = Join-Path $script:BuildRoot "Scripts\sign_driver.ps1"
        if (Test-Path $signScript) {
            Write-Host "Running post-build driver signing..." -ForegroundColor Cyan
            & $signScript -DriverPath $driverPath -SkipIfMissing
            # Note: sign_driver.ps1 exits with 0 if skipped (SkipIfMissing), so this is fine
        }
    }
    
    Write-Host ""
    Write-Host "Next Steps:" -ForegroundColor Cyan
    Write-Host "  To sign the driver (optional):" -ForegroundColor Yellow
    Write-Host "    `$env:EXT4FSD_CERT_PATH = 'C:\path\to\certificate.pfx'" -ForegroundColor White
    Write-Host "    `$env:MSIX_CERT_PASSWORD = 'your_password'" -ForegroundColor White
    Write-Host "    .\Scripts\sign_driver.ps1" -ForegroundColor White
    Write-Host ""
    Write-Host "  To install the driver:" -ForegroundColor Yellow
    Write-Host "    .\Scripts\install_driver.ps1" -ForegroundColor White
    Write-Host "    (Requires administrator privileges)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  See INSTALLATION.md for detailed instructions" -ForegroundColor Cyan
    
} else {
    Write-Host ""
    $msg = " Build failed!"
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Red
    Write-Host $msg -ForegroundColor Red
    Write-Host ("=" * ($msg.Length + 1)) -ForegroundColor Red
    exit 1
}
