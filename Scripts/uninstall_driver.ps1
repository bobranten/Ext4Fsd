<#
.SYNOPSIS
    Fully uninstall the locally built Ext4Fsd driver and Ext2Srv service

.DESCRIPTION
    Removes the Ext2Fsd kernel driver service, Ext2Srv user-mode service,
    and the driver file from System32\drivers. Use this before testing
    the official signed release (e.g. v0.71 from GitHub) to verify whether
    service communication issues (e.g. interactive flag) still apply.

.PARAMETER Ext2SrvPath
    Path to Ext2Srv.exe used when the service was installed. If provided,
    the script uses "Ext2Srv.exe /removeservice" for clean removal.
    If not provided, uses "sc delete Ext2Srv" (service is still fully removed).

.PARAMETER KeepDriverFile
    Do not delete Ext2Fsd.sys from System32\drivers. Used only for testing.

.EXAMPLE
    .\Scripts\uninstall_driver.ps1

.EXAMPLE
    .\Scripts\uninstall_driver.ps1 -Ext2SrvPath ".\Ext2Srv\Release\x64\Ext2Srv.exe"
#>

[CmdletBinding()]
param(
    [string]$Ext2SrvPath,
    [switch]$KeepDriverFile
)

$ErrorActionPreference = "Stop"
$scExe = "sc.exe"
$systemDriverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys"

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: Run as Administrator." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Uninstalling Ext4Fsd (full removal)" -ForegroundColor Cyan
Write-Host ""

# 1. Stop Ext2Srv
Write-Host "Step 1: Stopping Ext2Srv..." -ForegroundColor Yellow
$null = & $scExe query Ext2Srv 2>&1
if ($LASTEXITCODE -eq 0) {
    $null = & $scExe stop Ext2Srv 2>&1
    Start-Sleep -Seconds 2
    Write-Host "  Ext2Srv stopped" -ForegroundColor Green
} else {
    Write-Host "  Ext2Srv not installed" -ForegroundColor Gray
}

# 2. Remove Ext2Srv service
Write-Host ""
Write-Host "Step 2: Removing Ext2Srv service..." -ForegroundColor Yellow
if ($Ext2SrvPath -and (Test-Path $Ext2SrvPath)) {
    & $Ext2SrvPath /removeservice 2>&1
    Start-Sleep -Seconds 2
    Write-Host "  Ran $Ext2SrvPath /removeservice" -ForegroundColor Green
}
$null = & $scExe query Ext2Srv 2>&1
if ($LASTEXITCODE -eq 0) {
    $null = & $scExe delete Ext2Srv 2>&1
    Start-Sleep -Seconds 1
}
$null = & $scExe query Ext2Srv 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "  Ext2Srv service removed" -ForegroundColor Green
} else {
    Write-Host "  Ext2Srv service removed (or was not installed)" -ForegroundColor Green
}

# 3. Stop and remove Ext2Fsd kernel driver service
Write-Host ""
Write-Host "Step 3: Stopping and removing Ext2Fsd kernel driver service..." -ForegroundColor Yellow
$null = & $scExe query Ext2Fsd 2>&1
if ($LASTEXITCODE -eq 0) {
    $null = & $scExe stop Ext2Fsd 2>&1
    Start-Sleep -Seconds 2
    $null = & $scExe delete Ext2Fsd 2>&1
    Start-Sleep -Seconds 1
    Write-Host "  Ext2Fsd service removed" -ForegroundColor Green
} else {
    Write-Host "  Ext2Fsd service not installed" -ForegroundColor Gray
}

# 4. Remove driver file
Write-Host ""
Write-Host "Step 4: Removing driver file..." -ForegroundColor Yellow
if ($KeepDriverFile) {
    Write-Host "  Skipped (KeepDriverFile)" -ForegroundColor Gray
} elseif (Test-Path $systemDriverPath) {
    Remove-Item $systemDriverPath -Force -ErrorAction Stop
    Write-Host "  Removed $systemDriverPath" -ForegroundColor Green
} else {
    Write-Host "  File not present" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Uninstall complete." -ForegroundColor Green
Write-Host ""
Write-Host "You can now install the official signed release:" -ForegroundColor Cyan
Write-Host "  https://github.com/bobranten/Ext4Fsd/releases/tag/v0.71" -ForegroundColor White
Write-Host "  (Ext4Fsd-0.71-setup.exe or equivalent)" -ForegroundColor White
Write-Host ""
Write-Host "After installing the official build, check whether Ext2Srv still shows" -ForegroundColor Cyan
Write-Host "  TYPE 110 (interactive) with: sc qc Ext2Srv" -ForegroundColor White
Write-Host ""
