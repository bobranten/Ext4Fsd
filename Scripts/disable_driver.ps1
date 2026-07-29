<#
.SYNOPSIS
    Disable the Ext2Fsd kernel driver service to restore Windows stability

.DESCRIPTION
    When the Ext2Fsd driver fails to load (e.g. signature verification errors),
    Windows may keep retrying and cause boot delays or instability.
    This script disables the Ext2Fsd service so Windows stops trying to start it.
#>

[CmdletBinding()]
param(
    [switch]$AlsoDisableExt2Srv,
    [switch]$RemoveService
)

$ErrorActionPreference = "Stop"
$scExe = "sc.exe"

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: Run as Administrator." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Disabling Ext2Fsd Driver Service" -ForegroundColor Cyan
Write-Host ""

# Stop Ext2Fsd if running
$null = & $scExe query Ext2Fsd 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "Stopping Ext2Fsd..." -ForegroundColor Yellow
    $null = & $scExe stop Ext2Fsd 2>&1
    Start-Sleep -Seconds 2
}

# Disable Ext2Fsd
Write-Host "Disabling Ext2Fsd service (start= disabled)..." -ForegroundColor Yellow
$null = & $scExe config Ext2Fsd start= disabled 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  Ext2Fsd: DISABLED" -ForegroundColor Green
} else {
    Write-Host "  Ext2Fsd: Failed to disable" -ForegroundColor Red
}

if ($AlsoDisableExt2Srv) {
    Write-Host "Disabling Ext2Srv service..." -ForegroundColor Yellow
    $null = & $scExe stop Ext2Srv 2>&1
    $null = & $scExe config Ext2Srv start= disabled 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Ext2Srv: DISABLED" -ForegroundColor Green
    }
}

if ($RemoveService) {
    Write-Host "Removing Ext2Fsd service..." -ForegroundColor Yellow
    $null = & $scExe delete Ext2Fsd 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Ext2Fsd service removed" -ForegroundColor Green
    }
    $driverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys"
    if (Test-Path $driverPath) {
        Remove-Item $driverPath -Force -ErrorAction SilentlyContinue
        Write-Host "  Driver file removed" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "Done. Windows should no longer attempt to start Ext2Fsd at boot." -ForegroundColor Green
Write-Host "To re-enable later: sc config Ext2Fsd start= system" -ForegroundColor Gray
Write-Host ""
