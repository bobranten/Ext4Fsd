<#
.SYNOPSIS
    Verify the digital signature of the Ext4Fsd driver.
.PARAMETER DriverPath
    Path to the driver file.
.NOTES
    Runs the unified diagnostic script; signature and cert checks are in sections 4–6.
#>
[CmdletBinding()]
param([string]$DriverPath = "C:\Windows\System32\drivers\Ext2Fsd.sys")
& "${PSScriptRoot}\diagnose_ext2fsd.ps1" -DriverPath $DriverPath
