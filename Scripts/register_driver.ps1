<#
.SYNOPSIS
    Register the Ext2Fsd kernel driver as a Windows service
    
.DESCRIPTION
    This script registers the Ext2Fsd.sys kernel driver as a Windows kernel driver service.
    The driver must already be copied to C:\Windows\System32\drivers\Ext2Fsd.sys
#>

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# Use sc.exe explicitly to avoid PowerShell alias (sc -> Set-Content)
$scExe = "sc.exe"

# Check for administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires administrator privileges!" -ForegroundColor Red
    Write-Host "  Please run PowerShell as Administrator" -ForegroundColor Yellow
    exit 1
}

$driverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys"

if (-not (Test-Path $driverPath)) {
    Write-Host "ERROR: Driver file not found: $driverPath" -ForegroundColor Red
    Write-Host "  Copy the driver first using: .\Scripts\install_driver.ps1" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "Registering Ext2Fsd Kernel Driver" -ForegroundColor Cyan
Write-Host ""

# Check if service already exists
$null = & $scExe query Ext2Fsd 2>&1
$queryExitCode = $LASTEXITCODE
if ($queryExitCode -eq 0) {
    Write-Host "  Kernel driver service already exists" -ForegroundColor Gray
    Write-Host "  Removing existing service..." -ForegroundColor Yellow
    $null = & $scExe delete Ext2Fsd 2>&1
    Start-Sleep -Seconds 1
}

Write-Host "  Creating kernel driver service..." -ForegroundColor Yellow

# Register the kernel driver service
# Type: filesys = FILE_SYSTEM_DRIVER (kernel filesystem driver)
# Start: system = SERVICE_SYSTEM_START (starts early in boot process)
# ErrorControl: normal = SERVICE_ERROR_NORMAL
# BinaryPathName: path to driver
$createOutput = & $scExe create Ext2Fsd type= filesys start= system binPath= "$driverPath" error= normal 2>&1
$createExitCode = $LASTEXITCODE

if ($createExitCode -eq 0) {
    Write-Host "  Kernel driver service registered successfully" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "  Starting driver..." -ForegroundColor Yellow
    $null = & $scExe start Ext2Fsd 2>&1
    
    Start-Sleep -Seconds 2
    
    $status = & $scExe query Ext2Fsd 2>&1
    $statusExitCode = $LASTEXITCODE
    if ($statusExitCode -eq 0) {
        $running = $null -ne ($status | Select-String "RUNNING")
        if ($running) {
            Write-Host "  Driver started successfully" -ForegroundColor Green
        } else {
            Write-Host "  WARNING: Driver did not start" -ForegroundColor Yellow
            Write-Host "  Check event logs for errors" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "  ERROR: Failed to register kernel driver service" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Verify driver is loaded: sc query Ext2Fsd" -ForegroundColor White
Write-Host "  2. Check if device exists: Test-Path '\\.\Ext2Fsd'" -ForegroundColor White
Write-Host "  3. Try Ext2Mgr again" -ForegroundColor White
Write-Host ""
