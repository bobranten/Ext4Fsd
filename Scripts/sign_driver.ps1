<#
.SYNOPSIS
    Post-build script to sign the Ext4Fsd driver with a code signing certificate
    
.DESCRIPTION
    This script signs the compiled Ext4Fsd driver (.sys file) with a code signing certificate.
    It can be run manually or integrated into the build process.
    
.PARAMETER DriverPath
    Path to the driver file to sign. If not specified, uses default build output location.
    
.PARAMETER CertificatePath
    Path to the certificate (.pfx) file. If not specified, checks environment variable EXT4FSD_CERT_PATH.
    
.PARAMETER CertificatePassword
    Certificate password. If not specified, checks environment variable MSIX_CERT_PASSWORD or prompts.
    
.PARAMETER SkipIfMissing
    If certificate is not found, skip signing instead of failing (useful for automated builds).
    
.EXAMPLE
    .\Scripts\sign_driver.ps1

.EXAMPLE
    .\Scripts\sign_driver.ps1 -CertificatePath "C:\path\to\cert.pfx" -SkipIfMissing
#>

[CmdletBinding()]
param(
    [string]$DriverPath,
    [string]$CertificatePath,
    [string]$CertificatePassword,
    [switch]$SkipIfMissing
)

$ErrorActionPreference = "Stop"
$script:BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

# Default driver path if not specified
if (-not $DriverPath) {
    $DriverPath = Join-Path $script:BuildRoot "Ext4Fsd\Release\x64\Ext2Fsd.sys"
}

# Check if driver exists
if (-not (Test-Path $DriverPath)) {
    Write-Host "ERROR: Driver file not found: $DriverPath" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Signing Driver" -ForegroundColor Cyan
Write-Host "  Driver: $DriverPath" -ForegroundColor White

# Get certificate path
if (-not $CertificatePath) {
    $CertificatePath = $env:EXT4FSD_CERT_PATH
}

if (-not $CertificatePath) {
    if ($SkipIfMissing) {
        Write-Host "  Certificate path not specified (EXT4FSD_CERT_PATH not set)" -ForegroundColor Yellow
        Write-Host "  Skipping driver signing..." -ForegroundColor Yellow
        exit 0
    } else {
        Write-Host "ERROR: Certificate path not specified!" -ForegroundColor Red
        Write-Host "  Set EXT4FSD_CERT_PATH environment variable or use -CertificatePath parameter" -ForegroundColor Yellow
        Write-Host "  Example: `$env:EXT4FSD_CERT_PATH = 'C:\path\to\certificate.pfx'" -ForegroundColor Yellow
        exit 1
    }
}

if (-not (Test-Path $CertificatePath)) {
    if ($SkipIfMissing) {
        Write-Host "  Certificate file not found: $CertificatePath" -ForegroundColor Yellow
        Write-Host "  Skipping driver signing..." -ForegroundColor Yellow
        exit 0
    } else {
        Write-Host "ERROR: Certificate file not found: $CertificatePath" -ForegroundColor Red
        exit 1
    }
}

Write-Host "  Certificate: $CertificatePath" -ForegroundColor White

# Get certificate password
if (-not $CertificatePassword) {
    $CertificatePassword = $env:MSIX_CERT_PASSWORD
}

if (-not $CertificatePassword) {
    Write-Host "  Certificate password not found in MSIX_CERT_PASSWORD environment variable" -ForegroundColor Yellow
    $securePassword = Read-Host "Enter certificate password" -AsSecureString
    $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePassword)
    $CertificatePassword = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR)
    [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($BSTR)
}

# Find signtool.exe
Write-Host "  Searching for signtool.exe..." -ForegroundColor Yellow

# Try to find Windows SDK version from driver path or use default
$sdkVersion = "10.0.26100.0"  # Default, adjust if needed

# Check common SDK locations
$signtoolPaths = @(
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\$sdkVersion\x64\signtool.exe",
    "${env:ProgramFiles}\Windows Kits\10\bin\$sdkVersion\x64\signtool.exe"
)

# Also try to find latest SDK version
$sdkBasePath = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
if (Test-Path $sdkBasePath) {
    $sdkVersions = Get-ChildItem $sdkBasePath -Directory | Sort-Object Name -Descending
    foreach ($version in $sdkVersions) {
        $signtoolPath = Join-Path $version.FullName "x64\signtool.exe"
        if (Test-Path $signtoolPath) {
            $signtoolPaths = @($signtoolPath) + $signtoolPaths
            break
        }
    }
}

$signtool = $null
foreach ($path in $signtoolPaths) {
    if (Test-Path $path) {
        $signtool = $path
        break
    }
}

if (-not $signtool) {
    Write-Host "ERROR: signtool.exe not found!" -ForegroundColor Red
    Write-Host "  Please install Windows SDK or specify the path manually" -ForegroundColor Yellow
    exit 1
}

Write-Host "  Found signtool: $signtool" -ForegroundColor Green

# Sign the driver
Write-Host ""
Write-Host "Signing driver..." -ForegroundColor Yellow

$signArgs = @(
    "sign"
    "/f", $CertificatePath
    "/p", $CertificatePassword
    "/fd", "SHA256"
    "/t", "http://timestamp.digicert.com"
    $DriverPath
)

try {
    & $signtool @signArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "  Driver signed successfully!" -ForegroundColor Green
        
        # Verify signature
        Write-Host "  Verifying signature..." -ForegroundColor Yellow
        & $signtool verify /pa /v $DriverPath | Out-Null
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  Signature verified successfully!" -ForegroundColor Green
        } else {
            Write-Host "  WARNING: Signature verification failed" -ForegroundColor Yellow
        }
    } else {
        Write-Host ""
        Write-Host "ERROR: Driver signing failed!" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host ""
    Write-Host "ERROR: Failed to sign driver: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
