<#
.SYNOPSIS
    Install code signing certificate to Trusted Publishers store
    
.DESCRIPTION
    This script installs a code signing certificate to the Trusted Publishers store,
    which is required for Windows to load kernel-mode drivers signed with self-signed certificates.
    
.PARAMETER CertificatePath
    Path to the certificate (.pfx) file.
    
.PARAMETER CertificatePassword
    Certificate password. If not specified, prompts for it.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$CertificatePath,
    [SecureString]$CertificatePassword
)

$ErrorActionPreference = "Stop"

# Check for administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires administrator privileges!" -ForegroundColor Red
    Write-Host "  Please run PowerShell as Administrator" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $CertificatePath)) {
    Write-Host "ERROR: Certificate file not found: $CertificatePath" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Installing Certificate to Trusted Publishers Store" -ForegroundColor Cyan
Write-Host "  Certificate: $CertificatePath" -ForegroundColor White
Write-Host ""

# Get certificate password
if (-not $CertificatePassword) {
    $certPasswordString = $env:MSIX_CERT_PASSWORD
    if ($certPasswordString) {
        $CertificatePassword = ConvertTo-SecureString -String $certPasswordString -AsPlainText -Force
    }
}

if (-not $CertificatePassword) {
    Write-Host "Enter certificate password:" -ForegroundColor Yellow
    $CertificatePassword = Read-Host -AsSecureString
}

try {
    # Read the certificate
    Write-Host "Reading certificate..." -ForegroundColor Yellow
    $certData = Get-PfxData -FilePath $CertificatePath -Password $CertificatePassword
    
    if ($certData.EndEntityCertificates.Count -eq 0) {
        Write-Host "ERROR: No certificate found in PFX file" -ForegroundColor Red
        exit 1
    }
    
    $cert = $certData.EndEntityCertificates[0]
    Write-Host "  Certificate Subject: $($cert.Subject)" -ForegroundColor White
    Write-Host "  Certificate Thumbprint: $($cert.Thumbprint)" -ForegroundColor White
    
    # Check for Code Signing EKU
    $ekuExtensions = $cert.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.37' }
    if ($ekuExtensions) {
        $eku = $ekuExtensions[0]
        $ekuOids = $eku.Format($false) -split "`r`n"
        $hasCodeSigning = $false
        foreach ($oid in $ekuOids) {
            if ($oid -match "Code Signing|1\.3\.6\.1\.5\.5\.7\.3\.3") {
                $hasCodeSigning = $true
                break
            }
        }
        if ($hasCodeSigning) {
            Write-Host "  Code Signing EKU: PRESENT" -ForegroundColor Green
        } else {
            Write-Host "  Code Signing EKU: MISSING" -ForegroundColor Yellow
            Write-Host "  WARNING: Certificate may not work for kernel driver signing!" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  Code Signing EKU: NOT CHECKED" -ForegroundColor Gray
    }
    Write-Host ""
    
    # Check if already installed
    $publishersStore = New-Object System.Security.Cryptography.X509Certificates.X509Store([System.Security.Cryptography.X509Certificates.StoreName]::TrustedPublisher, [System.Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    $publishersStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadOnly)
    $existingCert = $publishersStore.Certificates.Find([System.Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint, $cert.Thumbprint, $false)
    $publishersStore.Close()
    
    if ($existingCert.Count -gt 0) {
        Write-Host "Certificate is already installed in Trusted Publishers store" -ForegroundColor Green
        Write-Host ""
        Write-Host "If the driver still fails with error 577, try:" -ForegroundColor Yellow
        Write-Host "  1. Restart the computer" -ForegroundColor White
        Write-Host "  2. Verify Secure Boot status: Confirm-SecureBootUEFI" -ForegroundColor White
        exit 0
    }
    
    # Install to Trusted Publishers store
    Write-Host "Installing certificate to Trusted Publishers store..." -ForegroundColor Yellow
    $publishersStore = New-Object System.Security.Cryptography.X509Certificates.X509Store([System.Security.Cryptography.X509Certificates.StoreName]::TrustedPublisher, [System.Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    $publishersStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    $publishersStore.Add($cert)
    $publishersStore.Close()
    
    Write-Host "Certificate installed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Restart your computer (required for kernel driver changes)" -ForegroundColor White
    Write-Host "  2. After restart, try starting the driver: sc.exe start Ext2Fsd" -ForegroundColor White
    Write-Host "  3. If it still fails, verify Secure Boot status: Confirm-SecureBootUEFI" -ForegroundColor White
    Write-Host ""
    
} catch {
    Write-Host ""
    Write-Host "ERROR: Failed to install certificate: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Alternative method using certmgr.msc:" -ForegroundColor Yellow
    Write-Host "  1. Open certmgr.msc" -ForegroundColor White
    Write-Host "  2. Navigate to: Trusted Publishers > Certificates" -ForegroundColor White
    Write-Host "  3. Right-click Certificates > All Tasks > Import" -ForegroundColor White
    Write-Host "  4. Import your certificate: $CertificatePath" -ForegroundColor White
    exit 1
}
