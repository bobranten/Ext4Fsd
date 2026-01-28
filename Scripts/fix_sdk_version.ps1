<#
.SYNOPSIS
    Updates the Windows SDK version in Ext4Fsd project files to match installed SDK
    
.DESCRIPTION
    This script updates the WindowsTargetPlatformVersion in the project files
    to use the installed Windows SDK version (10.0.26100.0) instead of the
    hardcoded version (10.0.22621.0).
#>

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

Write-Host "Updating Windows SDK version in project files..." -ForegroundColor Cyan
Write-Host ""

# Find installed SDK version
$sdkPath = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
if (Test-Path $sdkPath) {
    $installedSdk = Get-ChildItem $sdkPath -Directory | Sort-Object Name -Descending | Select-Object -First 1
    $sdkVersion = $installedSdk.Name
    Write-Host "Found installed SDK version: $sdkVersion" -ForegroundColor Green
} else {
    Write-Host "ERROR: Windows SDK not found!" -ForegroundColor Red
    exit 1
}

# Project files to update
$projectFiles = @(
    "Ext4Fsd\Ext4Fsd.vcxproj",
    "Ext2Mgr\Ext2Mgr.vcxproj",
    "Ext2Srv\Ext2Srv.vcxproj"
)

$updated = 0
foreach ($file in $projectFiles) {
    $fullPath = Join-Path $repoRoot $file
    if (Test-Path $fullPath) {
        Write-Host "Updating: $file" -ForegroundColor Yellow
        $content = Get-Content $fullPath -Raw
        
        # Replace WindowsTargetPlatformVersion
        if ($content -match 'WindowsTargetPlatformVersion>10\.0\.\d+\.\d+<') {
            $newContent = $content -replace 'WindowsTargetPlatformVersion>10\.0\.\d+\.\d+<', "WindowsTargetPlatformVersion>$sdkVersion<"
            Set-Content $fullPath -Value $newContent -NoNewline
            Write-Host "  Updated to $sdkVersion" -ForegroundColor Green
            $updated++
        } else {
            Write-Host "  No WindowsTargetPlatformVersion found" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  ✗ File not found: $fullPath" -ForegroundColor Red
    }
}

Write-Host ""
if ($updated -gt 0) {
    Write-Host "Updated $updated project file(s) to use SDK version $sdkVersion" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can now try building again:" -ForegroundColor Cyan
    Write-Host "  .\Scripts\build.ps1" -ForegroundColor White
} else {
    Write-Host "No files were updated." -ForegroundColor Yellow
}
