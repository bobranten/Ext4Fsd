<#
.SYNOPSIS
    Unified diagnostic script for the Ext2Fsd/Ext4Fsd driver.

.DESCRIPTION
    Combines checks from check_code_integrity, check_driver_load_error,
    diagnose_error_577, and verify_driver_signature. Use this script for
    full driver diagnostics (load failures, error 577, signature, event log).

.PARAMETER DriverPath
    Path to the driver file. Defaults to system driver location.

.PARAMETER CertificatePath
    Optional path to certificate (.pfx) for additional checks.

.PARAMETER UseSigntool
    When driver exists, also run signtool verify if Windows SDK is present.
    Default: $true. Set -UseSigntool:$false to skip (e.g. no SDK).
#>

[CmdletBinding()]
param(
    [string]$DriverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys",
    [string]$CertificatePath,
    [bool]$UseSigntool = $true
)

$ErrorActionPreference = "Continue"

function Write-Section { param([string]$Title) Write-Host ""; Write-Host $Title -ForegroundColor Cyan; Write-Host ("-" * [Math]::Min(60, $Title.Length)) -ForegroundColor Gray }
function Write-Check { param([string]$Label, [string]$Value, [string]$Color = "White") Write-Host "  $Label" -NoNewline; Write-Host " $Value" -ForegroundColor $Color }

Write-Host ""
Write-Host "Ext2Fsd/Ext4Fsd Driver Diagnostic" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
Write-Host "  Driver: $DriverPath" -ForegroundColor White
Write-Host ""

# ---- 1. Code Integrity & Test Signing ----
Write-Section "1. Code Integrity & Test Signing"

try {
    $ciPolicy = Get-CIPolicy -ErrorAction SilentlyContinue
    if ($ciPolicy) {
        Write-Check "Code Integrity Policy:" "ACTIVE (may block driver)" "Yellow"
        $ciPolicy | Format-List | Out-String | Write-Host
    } else {
        Write-Check "Code Integrity Policy:" "NOT ACTIVE" "Green"
    }
} catch {
    Write-Check "Code Integrity:" "Could not check: $_" "Gray"
}

$bcdOutput = bcdedit /enum {current} 2>&1 | Select-String "testsigning"
Write-Check "BCD Test Signing:" $(if ($bcdOutput -match "Yes") { "ENABLED" } else { "DISABLED" }) $(if ($bcdOutput -match "Yes") { "Green" } else { "Yellow" })

# ---- 2. Windows Version ----
Write-Section "2. Windows Version"

$os = Get-CimInstance Win32_OperatingSystem
Write-Check "Version:" $os.Version "White"
Write-Check "Build:" $os.BuildNumber "White"
if ([int]$os.BuildNumber -ge 22000) { Write-Check "Note:" "Windows 11 - stricter Code Integrity" "Yellow" }

# ---- 3. Driver File ----
Write-Section "3. Driver File"

$driverExists = Test-Path $DriverPath
if ($driverExists) {
    $fileInfo = Get-Item $DriverPath
    Write-Check "Exists:" "YES" "Green"
    Write-Check "Size:" "$($fileInfo.Length) bytes" "White"
    Write-Check "Last modified:" $fileInfo.LastWriteTime "White"
    try {
        $null = [System.IO.File]::Open($DriverPath, 'Open', 'Read', 'None').Close()
        Write-Check "Accessible:" "YES" "Green"
    } catch {
        Write-Check "Accessible:" "LOCKED or inaccessible" "Red"
    }
} else {
    Write-Check "Exists:" "NO" "Red"
    Write-Check "Note:" "Likely cause of error 31 / load failure" "Yellow"
}

# ---- 4. Signature (Authenticode + optional signtool) ----
if ($driverExists) {
    Write-Section "4. Driver Signature"

    $signature = Get-AuthenticodeSignature -FilePath $DriverPath
    Write-Check "Authenticode Status:" $signature.Status $(if ($signature.Status -eq 'Valid') { 'Green' } else { 'Red' })
    if ($signature.SignerCertificate) {
        Write-Check "Signer:" $signature.SignerCertificate.Subject "White"
        $isTestSigned = $signature.SignerCertificate.Subject -match "Test|TEST" -or $signature.SignerCertificate.Subject -eq $signature.SignerCertificate.Issuer
        Write-Check "Test-signed:" $isTestSigned "Yellow"
    }

    if ($UseSigntool) {
        $sdkBase = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
        $signtool = $null
        if (Test-Path $sdkBase) {
            $dirs = Get-ChildItem $sdkBase -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
            foreach ($d in $dirs) {
                $path = Join-Path $d.FullName "x64\signtool.exe"
                if (Test-Path $path) { $signtool = $path; break }
            }
        }
        if ($signtool) {
            Write-Host "  signtool verify:" -ForegroundColor Yellow
            $stOut = & $signtool verify /pa /v $DriverPath 2>&1
            $stExit = $LASTEXITCODE
            Write-Host ($stOut | Out-String)
            Write-Check "signtool result:" $(if ($stExit -eq 0) { "PASSED" } else { "FAILED" }) $(if ($stExit -eq 0) { "Green" } else { "Red" })
        }
    }
}

# ---- 5. Certificate Stores, EKU, Chain (when driver exists and has signer) ----
$hasCodeSigning = $false
$publisherCertCount = 0
if ($driverExists -and $signature -and $signature.SignerCertificate) {
    Write-Section "5. Certificate Stores & EKU"

    $cert = $signature.SignerCertificate
    $thumbprint = $cert.Thumbprint

    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store([System.Security.Cryptography.X509Certificates.StoreName]::Root, [System.Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    $rootStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadOnly)
    $rootCert = $rootStore.Certificates.Find([System.Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint, $thumbprint, $false)
    $rootStore.Close()
    Write-Check "Trusted Root:" $(if ($rootCert.Count -gt 0) { "FOUND" } else { "NOT FOUND" }) $(if ($rootCert.Count -gt 0) { "Green" } else { "Red" })

    $pubStore = New-Object System.Security.Cryptography.X509Certificates.X509Store([System.Security.Cryptography.X509Certificates.StoreName]::TrustedPublisher, [System.Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    $pubStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadOnly)
    $publisherCert = $pubStore.Certificates.Find([System.Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint, $thumbprint, $false)
    $pubStore.Close()
    $publisherCertCount = $publisherCert.Count
    Write-Check "Trusted Publishers:" $(if ($publisherCert.Count -gt 0) { "FOUND" } else { "NOT FOUND (often causes 577)" }) $(if ($publisherCert.Count -gt 0) { "Green" } else { "Red" })

    $ekuExt = $cert.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.37' }
    if ($ekuExt) {
        $ekuStr = $ekuExt[0].Format($false)
        $hasCodeSigning = $ekuStr -match "Code Signing|1\.3\.6\.1\.5\.5\.7\.3\.3"
        Write-Check "Code Signing EKU:" $(if ($hasCodeSigning) { "PRESENT" } else { "MISSING" }) $(if ($hasCodeSigning) { "Green" } else { "Red" })
    } else {
        Write-Check "EKU:" "No EKU extensions" "Yellow"
    }

    Write-Section "6. Certificate Chain"
    try {
        $chain = New-Object System.Security.Cryptography.X509Certificates.X509Chain
        $chain.ChainPolicy.RevocationMode = [System.Security.Cryptography.X509Certificates.X509RevocationMode]::NoCheck
        $chain.ChainPolicy.VerificationFlags = [System.Security.Cryptography.X509Certificates.X509VerificationFlags]::AllowUnknownCertificateAuthority
        $built = $chain.Build($cert)
        Write-Check "Chain validation:" $(if ($built) { "SUCCESS (length $($chain.ChainElements.Count))" } else { "FAILED" }) $(if ($built) { "Green" } else { "Red" })
        if (-not $built) {
            foreach ($s in $chain.ChainStatus) { Write-Host "    $($s.Status): $($s.StatusInformation)" -ForegroundColor Red }
        }
    } catch {
        Write-Check "Chain:" "Error: $_" "Red"
    }
} else {
    Write-Section "5. Certificate Stores & EKU"
    Write-Host "  Skipped (driver missing or no signer)" -ForegroundColor Gray
    Write-Section "6. Certificate Chain"
    Write-Host "  Skipped (driver missing or no signer)" -ForegroundColor Gray
}

# ---- 7. Secure Boot ----
Write-Section "7. Secure Boot"
try {
    $sb = Confirm-SecureBootUEFI -ErrorAction SilentlyContinue
    Write-Check "Secure Boot:" $(if ($sb) { "ENABLED" } else { "DISABLED" }) $(if ($sb) { "Yellow" } else { "Green" })
} catch {
    Write-Check "Secure Boot:" "Could not determine (e.g. non-UEFI)" "Gray"
}

# ---- 8. Driver Service Status ----
Write-Section "8. Driver Service Status"

$scOut = & sc.exe query Ext2Fsd 2>&1
$scExit = $LASTEXITCODE
if ($scExit -eq 0) {
    $win32Line = $scOut | Select-String "WIN32_EXIT_CODE"
    Write-Host "  Service: Registered" -ForegroundColor White
    if ($win32Line) { Write-Host "  $($win32Line.Line)" -ForegroundColor $(if ($win32Line -match "577") { "Red" } else { "White" }) }
} else {
    Write-Check "Service:" "Not registered" "Yellow"
}

# ---- 9. Driver Load Status (WMI) ----
Write-Section "9. Driver Load Status"

$loaded = Get-WmiObject Win32_SystemDriver -ErrorAction SilentlyContinue | Where-Object { $_.Name -eq "Ext2Fsd" }
if ($loaded) {
    Write-Check "Loaded:" "YES" "Green"
    Write-Check "State:" $loaded.State "White"
    Write-Check "Status:" $loaded.Status "White"
} else {
    Write-Check "Loaded:" "NO" "Yellow"
    Write-Host "  (Expected if driver failed to start)" -ForegroundColor Gray
}

# ---- 10. Event Log ----
Write-Section "10. Event Log (Driver Load Errors)"

try {
    $events219 = Get-WinEvent -FilterHashtable @{LogName='System'; ID=219} -MaxEvents 10 -ErrorAction SilentlyContinue
    $relevant = $events219 | Where-Object { $_.Message -match "Ext2Fsd|ext2fsd" }
    if ($relevant) {
        foreach ($e in $relevant | Select-Object -First 5) {
            Write-Host "  [$($e.TimeCreated)]" -ForegroundColor White
            Write-Host "  $($e.Message)" -ForegroundColor Red
            Write-Host ""
        }
    } else {
        Write-Check "Event ID 219 (Ext2Fsd):" "No recent matches" "Green"
    }
} catch {
    Write-Check "Event log:" "Could not read: $_" "Gray"
}

try {
    $anyExt = Get-WinEvent -FilterHashtable @{LogName='System'} -MaxEvents 100 -ErrorAction SilentlyContinue | Where-Object { $_.Message -match "Ext2Fsd|ext2fsd" }
    if ($anyExt -and -not $relevant) {
        foreach ($e in ($anyExt | Select-Object -First 3)) {
            Write-Host "  [$($e.TimeCreated)] ID:$($e.Id) $($e.Message)" -ForegroundColor Gray
        }
    }
} catch { }

# ---- 11. Recommendations ----
Write-Section "Recommendations"

if (-not $driverExists) {
    Write-Host "  Install or copy the driver to: $DriverPath" -ForegroundColor Yellow
    Write-Host "  Then run .\Scripts\register_driver.ps1 and .\Scripts\install_driver.ps1 as needed." -ForegroundColor White
} elseif ($signature.Status -ne 'Valid') {
    Write-Host "  Signature is not valid. Re-sign and ensure cert is in Trusted Root:" -ForegroundColor Yellow
    Write-Host "    .\Scripts\sign_driver.ps1 -DriverPath '$DriverPath'" -ForegroundColor White
} else {
    if ($publisherCertCount -eq 0) {
        Write-Host "  1. Install certificate in Trusted Publishers: .\Scripts\install_certificate.ps1 -CertificatePath 'CERT_PATH'" -ForegroundColor Yellow
    }
    if (-not $hasCodeSigning -and $signature.SignerCertificate) {
        Write-Host "  2. Certificate may lack Code Signing EKU; recreate or use a code-signing cert." -ForegroundColor Yellow
    }
    Write-Host "  3. Steps to try:" -ForegroundColor Yellow
    Write-Host "     a. Certificate in Trusted Publishers" -ForegroundColor White
    Write-Host "     b. Re-sign: .\Scripts\sign_driver.ps1 -DriverPath '$DriverPath'" -ForegroundColor White
    Write-Host "     c. Restart computer" -ForegroundColor White
    Write-Host "     d. Start driver: sc.exe start Ext2Fsd" -ForegroundColor White
}

Write-Host ""
