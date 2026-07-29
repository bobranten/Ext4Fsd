<#
.SYNOPSIS
    Comprehensive diagnostic for error 577 (driver signature verification failure).
.PARAMETER DriverPath
    Path to the driver file.
.PARAMETER CertificatePath
    Optional path to certificate (reserved for future use).
.NOTES
    This runs the unified diagnostic (diagnose_ext2fsd.ps1), which includes signature,
    cert stores, EKU, chain, Secure Boot, service status, and recommendations.
#>
[CmdletBinding()]
param(
    [string]$DriverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys",
    [string]$CertificatePath
)
& "${PSScriptRoot}\diagnose_ext2fsd.ps1" -DriverPath $DriverPath -CertificatePath $CertificatePath
