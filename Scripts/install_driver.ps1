<#
.SYNOPSIS
    Install script for Ext4Fsd driver and Ext2Srv service
    
.DESCRIPTION
    This script installs the compiled Ext4Fsd driver and Ext2Srv service.
    It copies the driver to the system drivers folder and registers/installs the service.
    
.PARAMETER DriverPath
    Path to the driver file (.sys). If not specified, uses default build output location.
    
.PARAMETER ServicePath
    Path to the Ext2Srv service executable. If not specified, uses default build output location.
    
.PARAMETER Configuration
    Build configuration (Release or Debug). Used to find default paths. Default: Release
    
.PARAMETER Platform
    Target platform (x64, x86, ARM, ARM64). Used to find default paths. Default: x64
    
.PARAMETER SkipServiceInstall
    Skip service installation/registration. Only copy the driver.
    
.PARAMETER SkipServiceStart
    Skip starting the service after installation.
    
.EXAMPLE
    .\Scripts\install_driver.ps1

.EXAMPLE
    .\Scripts\install_driver.ps1 -Configuration Release -Platform x64

.EXAMPLE
    .\Scripts\install_driver.ps1 -DriverPath "Ext4Fsd\Release\x64\Ext2Fsd.sys" -ServicePath "Ext2Srv\Release\x64\Ext2Srv.exe"
#>

[CmdletBinding()]
param(
    [string]$DriverPath,
    [string]$ServicePath,
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [ValidateSet("x64", "x86", "ARM", "ARM64")]
    [string]$Platform = "x64",
    [switch]$SkipServiceInstall,
    [switch]$SkipServiceStart
)

$ErrorActionPreference = "Stop"
$script:BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

# Use sc.exe explicitly to avoid PowerShell alias (sc -> Set-Content)
$scExe = "sc.exe"

# Check for administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires administrator privileges!" -ForegroundColor Red
    Write-Host "  Please run PowerShell as Administrator" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    Write-Host "  Or use: Start-Process powershell -Verb RunAs" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "Installing Ext4Fsd Driver" -ForegroundColor Cyan
Write-Host ""

# Default paths if not specified
if (-not $DriverPath) {
    $DriverPath = Join-Path $script:BuildRoot "Ext4Fsd\$Configuration\$Platform\Ext2Fsd.sys"
}

if (-not $ServicePath) {
    $ServicePath = Join-Path $script:BuildRoot "Ext2Srv\$Configuration\$Platform\Ext2Srv.exe"
}

# Verify files exist
if (-not (Test-Path $DriverPath)) {
    Write-Host "ERROR: Driver file not found: $DriverPath" -ForegroundColor Red
    Write-Host "  Build the driver first using: .\Scripts\build.ps1" -ForegroundColor Yellow
    exit 1
}

if (-not $SkipServiceInstall -and -not (Test-Path $ServicePath)) {
    Write-Host "ERROR: Service executable not found: $ServicePath" -ForegroundColor Red
    Write-Host "  Build the service first using: .\Scripts\build.ps1" -ForegroundColor Yellow
    exit 1
}

Write-Host "Files:" -ForegroundColor Cyan
Write-Host "  Driver: $DriverPath" -ForegroundColor White
if (-not $SkipServiceInstall) {
    Write-Host "  Service: $ServicePath" -ForegroundColor White
}
Write-Host ""

# Step 1: Stop the service if running
Write-Host "Step 1: Checking service status..." -ForegroundColor Yellow
$serviceStatus = & $scExe query Ext2Srv 2>$null
if ($LASTEXITCODE -eq 0) {
    $serviceRunning = $null -ne ($serviceStatus | Select-String "RUNNING")
    if ($serviceRunning) {
        Write-Host "  Service is running, stopping..." -ForegroundColor Yellow
        & $scExe stop Ext2Srv | Out-Null
        Start-Sleep -Seconds 2
        
        # Wait for service to stop
        $timeout = 10
        $elapsed = 0
        while ($elapsed -lt $timeout) {
            $status = & $scExe query Ext2Srv 2>$null
            if ($null -ne ($status | Select-String "STOPPED")) {
                Write-Host "  Service stopped successfully" -ForegroundColor Green
                break
            }
            Start-Sleep -Seconds 1
            $elapsed++
        }
        
        if ($elapsed -ge $timeout) {
            Write-Host "  WARNING: Service did not stop within timeout" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  Service is not running" -ForegroundColor Gray
    }
} else {
    Write-Host "  Service not installed yet" -ForegroundColor Gray
}

# Step 2: Copy driver to system folder
Write-Host ""
Write-Host "Step 2: Copying driver to system folder..." -ForegroundColor Yellow
$systemDriverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys"

try {
    Copy-Item $DriverPath $systemDriverPath -Force
    Write-Host "  Driver copied successfully" -ForegroundColor Green
    Write-Host "  Destination: $systemDriverPath" -ForegroundColor Gray
} catch {
    Write-Host "ERROR: Failed to copy driver: $_" -ForegroundColor Red
    exit 1
}

# Step 2a: Register kernel driver service (if not already registered)
Write-Host ""
Write-Host "Step 2a: Registering kernel driver service..." -ForegroundColor Yellow

$null = & $scExe query Ext2Fsd 2>&1
$queryExitCode = $LASTEXITCODE
if ($queryExitCode -ne 0) {
    Write-Host "  Kernel driver service not found, registering..." -ForegroundColor Yellow
    
    # Register the kernel driver service
    # Type: filesys = FILE_SYSTEM_DRIVER (kernel filesystem driver)
    # Start: system = SERVICE_SYSTEM_START (starts early in boot process)
    # ErrorControl: normal = SERVICE_ERROR_NORMAL
    $createOutput = & $scExe create Ext2Fsd type= filesys start= system binPath= "$systemDriverPath" error= normal 2>&1
    $createExitCode = $LASTEXITCODE
    
    if ($createExitCode -eq 0) {
        Write-Host "  Kernel driver service registered successfully" -ForegroundColor Green
        
        # Create registry keys that Ext2Mgr expects
        Write-Host "  Creating registry keys..." -ForegroundColor Yellow
        try {
            $regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Ext2Fsd"
            $paramsPath = "$regPath\Parameters"
            
            if (-not (Test-Path $regPath)) {
                New-Item -Path $regPath -Force | Out-Null
            }
            if (-not (Test-Path $paramsPath)) {
                New-Item -Path $paramsPath -Force | Out-Null
            }
            
            # Set default values from Ext2Fsd.inf
            Set-ItemProperty -Path $paramsPath -Name "AutoMount" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "CheckingBitmap" -Value 0 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "Ext3ForceWriting" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "WritingSupport" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "CodePage" -Value "utf8" -Type String -ErrorAction SilentlyContinue
            
            Write-Host "  Registry keys created successfully" -ForegroundColor Green
        } catch {
            Write-Host "  WARNING: Failed to create registry keys: $_" -ForegroundColor Yellow
            Write-Host "  Ext2Mgr may not be able to save settings until keys are created" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  ERROR: Failed to register kernel driver service" -ForegroundColor Red
        Write-Host "  Error output: $createOutput" -ForegroundColor Yellow
        Write-Host "  You may need to run: .\register_driver.ps1" -ForegroundColor Yellow
    }
} else {
    Write-Host "  Kernel driver service already registered" -ForegroundColor Gray
    
    # Ensure registry keys exist even if service was already registered
    try {
        $paramsPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Ext2Fsd\Parameters"
        if (-not (Test-Path $paramsPath)) {
            Write-Host "  Creating missing registry keys..." -ForegroundColor Yellow
            New-Item -Path $paramsPath -Force | Out-Null
            
            # Set default values
            Set-ItemProperty -Path $paramsPath -Name "AutoMount" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "Ext3ForceWriting" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "WritingSupport" -Value 1 -Type DWord -ErrorAction SilentlyContinue
            Set-ItemProperty -Path $paramsPath -Name "CodePage" -Value "utf8" -Type String -ErrorAction SilentlyContinue
            
            Write-Host "  Registry keys created" -ForegroundColor Green
        }
    } catch {
        Write-Host "  WARNING: Could not verify/create registry keys: $_" -ForegroundColor Yellow
    }
}

# Start the kernel driver if not running
$kernelStatus = & $scExe query Ext2Fsd 2>&1
$kernelQueryExitCode = $LASTEXITCODE
if ($kernelQueryExitCode -eq 0) {
    $kernelRunning = $null -ne ($kernelStatus | Select-String "RUNNING")
    if (-not $kernelRunning) {
        Write-Host "  Starting kernel driver..." -ForegroundColor Yellow
        $null = & $scExe start Ext2Fsd 2>&1
        Start-Sleep -Seconds 2
    }
}

# Step 3: Install/register the service
if (-not $SkipServiceInstall) {
    Write-Host ""
    Write-Host "Step 3: Installing service..." -ForegroundColor Yellow
    
    # Check if service already exists
    $serviceCheck = & $scExe query Ext2Srv 2>&1
    $serviceQueryExitCode = $LASTEXITCODE
    if ($serviceQueryExitCode -eq 0) {
        Write-Host "  Service already registered" -ForegroundColor Gray
        
        # Check if service binary path matches what we're installing
        $binaryPathMatch = $serviceCheck | Select-String "BINARY_PATH_NAME"
        $currentBinaryPath = $null
        
        if ($binaryPathMatch) {
            $binaryPathLine = $binaryPathMatch.Line
            if ($binaryPathLine -match "BINARY_PATH_NAME\s+:\s+(.+)") {
                $currentBinaryPath = $matches[1].Trim()
            }
        }
        
        # Also check service type to detect if it's still the old interactive version
        $serviceTypeMatch = $serviceCheck | Select-String "TYPE"
        $isInteractiveService = $false
        if ($serviceTypeMatch) {
            $typeLine = $serviceTypeMatch.Line
            if ($typeLine -match "TYPE\s+:\s+\d+\s+.*\(interactive\)") {
                $isInteractiveService = $true
            }
        }
        
        $newBinaryPath = (Resolve-Path $ServicePath).Path
        
        # Reinstall if: binary path differs OR service is still interactive (old version)
        if (($currentBinaryPath -and $currentBinaryPath -ne $newBinaryPath) -or $isInteractiveService) {
            if ($isInteractiveService) {
                Write-Host "  Service is configured as interactive (old version), reinstalling..." -ForegroundColor Yellow
            } else {
                Write-Host "  Service binary path differs, reinstalling..." -ForegroundColor Yellow
                Write-Host "    Current: $currentBinaryPath" -ForegroundColor Gray
                Write-Host "    New:     $newBinaryPath" -ForegroundColor Gray
            }
            
            # Remove old service
            Write-Host "  Removing old service..." -ForegroundColor Yellow
            & $ServicePath /removeservice 2>$null | Out-Null
            
            # Wait a moment for service removal
            Start-Sleep -Seconds 2
            
            # Reinstall with new binary
            Write-Host "  Registering Ext2Srv service with new binary..." -ForegroundColor Yellow
            & $ServicePath /installasservice
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  Service reinstalled successfully" -ForegroundColor Green
            } else {
                Write-Host "  WARNING: Service reinstallation may have failed (check output above)" -ForegroundColor Yellow
            }
        } else {
            if ($currentBinaryPath) {
                Write-Host "  Service binary path matches and type is correct, skipping reinstallation" -ForegroundColor Gray
            } else {
                Write-Host "  Could not determine current binary path, skipping reinstallation" -ForegroundColor Gray
                Write-Host "  If service type shows 'interactive', manually reinstall with:" -ForegroundColor Yellow
                $removeCmd = "& `"$ServicePath`" /removeservice"
                $installCmd = "& `"$ServicePath`" /installasservice"
                Write-Host "    $removeCmd" -ForegroundColor White
                Write-Host "    $installCmd" -ForegroundColor White
            }
        }
    } else {
        Write-Host "  Registering Ext2Srv service..." -ForegroundColor Yellow
        & $ServicePath /installasservice
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  Service registered successfully" -ForegroundColor Green
        } else {
            Write-Host "  WARNING: Service registration may have failed (check output above)" -ForegroundColor Yellow
        }
    }
}

# Step 4: Start the service
if (-not $SkipServiceStart) {
    Write-Host ""
    Write-Host "Step 4: Starting service..." -ForegroundColor Yellow
    
    $null = & $scExe start Ext2Srv 2>&1
    Start-Sleep -Seconds 2
    
    # Check if service started
    $serviceStatus = & $scExe query Ext2Srv 2>&1
    $serviceStartExitCode = $LASTEXITCODE
    if ($serviceStartExitCode -eq 0) {
        $serviceRunning = $null -ne ($serviceStatus | Select-String "RUNNING")
        if ($serviceRunning) {
            Write-Host "  Service started successfully" -ForegroundColor Green
        } else {
            Write-Host "  WARNING: Service did not start" -ForegroundColor Yellow
            Write-Host "  Check service status: sc query Ext2Srv" -ForegroundColor Yellow
            Write-Host "  Check event logs for errors" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  ERROR: Failed to query service status" -ForegroundColor Red
    }
}

# Summary
Write-Host ""
Write-Host "Installation Summary" -ForegroundColor Cyan
Write-Host "  Driver installed: $systemDriverPath" -ForegroundColor White

# Check kernel driver status
$kernelStatus = & $scExe query Ext2Fsd 2>&1
$kernelSummaryExitCode = $LASTEXITCODE
if ($kernelSummaryExitCode -eq 0) {
    $kernelRunning = $null -ne ($kernelStatus | Select-String "RUNNING")
    if ($kernelRunning) {
        Write-Host "  Kernel driver (Ext2Fsd): RUNNING" -ForegroundColor Green
    } else {
        Write-Host "  Kernel driver (Ext2Fsd): STOPPED" -ForegroundColor Yellow
    }
} else {
    Write-Host "  Kernel driver (Ext2Fsd): NOT REGISTERED" -ForegroundColor Red
}

if (-not $SkipServiceInstall) {
    $serviceStatus = & $scExe query Ext2Srv 2>&1
    $serviceSummaryExitCode = $LASTEXITCODE
    if ($serviceSummaryExitCode -eq 0) {
        $serviceRunning = $null -ne ($serviceStatus | Select-String "RUNNING")
        if ($serviceRunning) {
            Write-Host "  Service (Ext2Srv): RUNNING" -ForegroundColor Green
        } else {
            Write-Host "  Service (Ext2Srv): STOPPED" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  Service (Ext2Srv): NOT INSTALLED" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Verify driver is working: sc query Ext2Srv" -ForegroundColor White
Write-Host "  2. Mount ext4 partitions using Ext2Mgr or command line" -ForegroundColor White
Write-Host "  3. See INSTALLATION.md for mounting instructions" -ForegroundColor White
Write-Host ""
