<#
.SYNOPSIS
    Build and package user-mode release binaries (Ext2Srv + ext2mgr_iced).

.DESCRIPTION
    Does NOT build or ship the Ext2Fsd kernel driver - leave that to upstream.
    Builds only Ext2Srv (MSVC) and the iced GUI (Cargo), then copies them into
    dist\usermode-<version>\ with per-arch zips, SHA256 sums, and RELEASE_NOTES.txt.

    With no -Platforms, builds only the host architecture (x64 or ARM64).
    With explicit -Platforms, every listed arch must have its toolchain or the
    script exits without building and prints install help.

    Requirements:
      - Visual Studio 2019/2022 (or Build Tools) with MSVC for the requested arch(es)
      - For ARM64 Ext2Srv: MSVC v143 C++ ARM64/ARM64EC build tools
      - Rust (rustup) with the matching *-pc-windows-msvc target(s)
      - WDK is NOT required for this script

.PARAMETER Version
    Version string for folder/zip names. Default: git describe --tags --always, else date.

.PARAMETER OutDir
    Output root. Default: <repo>\dist

.PARAMETER Platforms
    Architectures to build: x64 and/or ARM64.
    Default (omit this parameter): host architecture only.

.PARAMETER SkipExt2Srv
    Skip Ext2Srv MSBuild

.PARAMETER SkipIced
    Skip ext2mgr_iced cargo build

.PARAMETER SkipZip
    Do not create .zip archives

.PARAMETER Clean
    Clean MSBuild / cargo before building

.EXAMPLE
    .\Scripts\release_usermode.ps1
    Build Ext2Srv + iced for this machine's architecture only

.EXAMPLE
    .\Scripts\release_usermode.ps1 -Platforms x64,ARM64
    Build both arches (fails early if any toolchain is missing)

.EXAMPLE
    .\Scripts\release_usermode.ps1 -Version 0.1.0-preview1 -SkipZip
#>

[CmdletBinding()]
param(
    [string]$Version = "",
    [string]$OutDir = "",
    [ValidateSet("x64", "ARM64")]
    [string[]]$Platforms = @(),
    [switch]$SkipExt2Srv,
    [switch]$SkipIced,
    [switch]$SkipZip,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$script:BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$script:PlatformsExplicit = $PSBoundParameters.ContainsKey("Platforms")

function Write-Header([string]$text) {
    Write-Host ""
    Write-Host ("=" * ($text.Length + 1)) -ForegroundColor Cyan
    Write-Host $text -ForegroundColor Cyan
    Write-Host ("=" * ($text.Length + 1)) -ForegroundColor Cyan
}

function Get-HostReleasePlatform {
    # PROCESSOR_ARCHITECTURE is the native OS arch (not WoW64 process arch).
    $arch = [string]$env:PROCESSOR_ARCHITECTURE
    switch -Regex ($arch) {
        '^(ARM64|aarch64)$' { return "ARM64" }
        '^(AMD64|x86_64)$' { return "x64" }
        '^(x86)$' {
            if ($env:PROCESSOR_ARCHITEW6432 -match '^(AMD64|x86_64)$') {
                return "x64"
            }
            throw "32-bit x86 host is not supported by this release script."
        }
        default {
            throw "Unrecognized PROCESSOR_ARCHITECTURE='$arch' (expected AMD64 or ARM64)."
        }
    }
}

function Find-MSBuild {
    $vsVersions = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\BuildTools"
    )
    foreach ($path in $vsVersions) {
        $msbuild = Join-Path $path "MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path -LiteralPath $msbuild) {
            return $msbuild
        }
    }
    return $null
}

function Get-VSInstallRootFromMSBuild([string]$msbuildPath) {
    # ...\MSBuild\Current\Bin\MSBuild.exe -> VS root
    return (Split-Path (Split-Path (Split-Path (Split-Path $msbuildPath -Parent) -Parent) -Parent) -Parent)
}

function Test-MsvcArm64Tools([string]$vsRoot) {
    # Need MSBuild ARM64 platform props AND Hostx64\arm64 cl.exe under some MSVC version.
    $platformDir = Join-Path $vsRoot "MSBuild\Microsoft\VC\v170\Platforms\ARM64"
    if (-not (Test-Path -LiteralPath $platformDir)) {
        return $false
    }
    $msvcRoot = Join-Path $vsRoot "VC\Tools\MSVC"
    if (-not (Test-Path -LiteralPath $msvcRoot)) {
        return $false
    }
    foreach ($verDir in Get-ChildItem -LiteralPath $msvcRoot -Directory) {
        $cl = Join-Path $verDir.FullName "bin\Hostx64\arm64\cl.exe"
        if (Test-Path -LiteralPath $cl) {
            return $true
        }
    }
    return $false
}

function Write-Arm64ToolchainHelp([string]$vsRoot) {
    Write-Host ""
    Write-Host "ARM64 MSVC build tools are not installed in:" -ForegroundColor Red
    Write-Host "  $vsRoot" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Install component: MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools" -ForegroundColor Cyan
    Write-Host "  Visual Studio Installer -> Modify Build Tools -> Individual components -> search ARM64" -ForegroundColor White
    Write-Host ""
    Write-Host "Or from an elevated prompt (adjust InstallPath if needed):" -ForegroundColor Cyan
    $setup = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\setup.exe"
    Write-Host "  Start-Process -FilePath `"$setup`" -ArgumentList 'modify','--installPath','$vsRoot','--add','Microsoft.VisualStudio.Component.VC.Tools.ARM64','--passive','--norestart' -Verb RunAs -Wait" -ForegroundColor White
    Write-Host ""
    Write-Host "Host-arch only (no -Platforms):" -ForegroundColor Cyan
    Write-Host "  .\Scripts\release_usermode.ps1" -ForegroundColor White
}

function Resolve-ReleasePlatforms {
    if (-not $script:PlatformsExplicit -or $Platforms.Count -eq 0) {
        $hostPlatform = Get-HostReleasePlatform
        Write-Host "Platforms: $hostPlatform (host default; pass -Platforms to select explicitly)" -ForegroundColor Gray
        return ,@($hostPlatform)
    }
    # Dedupe while preserving order
    $resolved = [System.Collections.Generic.List[string]]::new()
    foreach ($platform in $Platforms) {
        if (-not $resolved.Contains($platform)) {
            $resolved.Add($platform)
        }
    }
    Write-Host "Platforms: $($resolved -join ', ') (explicit)" -ForegroundColor Gray
    return ,$resolved.ToArray()
}

function Get-Ext2SrvOutDir([string]$configuration, [string]$platform) {
    # Matches Ext2Srv.vcxproj OutDir (arm64 is lowercase in the project file)
    $folder = switch ($platform) {
        "x64" { "x64" }
        "ARM64" { "arm64" }
        default { $platform.ToLowerInvariant() }
    }
    return Join-Path $script:BuildRoot "Ext2Srv\$configuration\$folder"
}

function Get-RustTarget([string]$platform) {
    switch ($platform) {
        "x64" { return "x86_64-pc-windows-msvc" }
        "ARM64" { return "aarch64-pc-windows-msvc" }
        default { throw "Unsupported platform for Rust: $platform" }
    }
}

function Resolve-Version {
    if ($Version -and $Version.Trim().Length -gt 0) {
        return $Version.Trim()
    }
    Push-Location $script:BuildRoot
    try {
        $described = & git describe --tags --always 2>$null
        if ($LASTEXITCODE -eq 0 -and $described) {
            return ($described.Trim() -replace '[\\/:*?"<>|]', '-')
        }
    } finally {
        Pop-Location
    }
    return (Get-Date -Format "yyyyMMdd")
}

function Write-Sha256Sums([string]$directory) {
    $sumPath = Join-Path $directory "SHA256SUMS.txt"
    $lines = @()
    Get-ChildItem -LiteralPath $directory -File |
        Where-Object { $_.Name -ne "SHA256SUMS.txt" } |
        ForEach-Object {
            $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            $lines += "$hash  $($_.Name)"
        }
    Set-Content -LiteralPath $sumPath -Value $lines -Encoding utf8
    Write-Host "  wrote $sumPath" -ForegroundColor Gray
}

function New-ReleaseNotes([string]$path, [string]$versionLabel) {
    $notes = @"
Ext4Fsd fork - user-mode tools $versionLabel
============================================

This release packages **user-mode** binaries only:

- Ext2Srv.exe - mount helper service (named pipe for Ext2Mgr / iced)
- ext2mgr_iced.exe - Rust + Iced Ext2/Ext4 volume manager GUI

Architectures
-------------
- x64   -> Windows on x86_64
- arm64 -> Windows on ARM64 (Snapdragon / Windows on Arm)

Use the folder that matches your Windows install. Mixing arches (e.g. ARM64 GUI
with x64 Ext2Srv on ARM64 Windows) is not supported by this layout.

What this does NOT include
--------------------------
- The Ext2Fsd **kernel driver** (.sys) and installer
- Classic MFC Ext2Mgr.exe

Install the signed driver from **upstream** (or your existing Ext2Fsd install),
then replace/add these user-mode tools on top if you want this fork's Ext2Srv /
iced improvements.

Upstream driver / classic stack (example):
  https://www.accum.se/~bosse/ext2fsd/

Requirements
------------
- Windows 10/11 with Ext2Fsd driver already installed and working
- Matching-architecture Ext2Srv (install as service, usually elevated)
- For permanent mount modes / Ext2Fsd registry writes: run iced elevated
- Temporary letter ops normally need Ext2Srv running

Install sketch (Administrator)
------------------------------
1. Stop the existing Ext2Srv service if present:
     sc.exe stop Ext2Srv
2. Copy the arch-matching Ext2Srv.exe over your install (often
   "C:\Program Files\Ext2Fsd\Ext2Srv.exe"), or install via Ext2Srv's own
   /installasservice if you use a fresh path.
3. Start the service:
     sc.exe start Ext2Srv
4. Run ext2mgr_iced.exe (elevated when changing permanent mounts / services).

Limits / caveats
----------------
- These builds are **not** a full product installer.
- Binaries may be **unsigned**; Windows SmartScreen can warn on first run.
- This fork does not ship a re-signed kernel driver - driver trust stays with
  upstream (or your own signing process).
- If the driver on the machine is an older build, some iced/Ext2Srv behaviors
  may still depend on what that driver supports.

Verify
------
Each arch folder has SHA256SUMS.txt. Example:

  Get-FileHash .\Ext2Srv.exe -Algorithm SHA256

"@
    # UTF-8 no BOM; normalize LF/CRLF so a CRLF-saved .ps1 does not become CRCRLF
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    $normalized = (($notes -replace "`r`n", "`n") -replace "`r", "`n") -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($path, $normalized, $utf8NoBom)
    Write-Host "  wrote $path" -ForegroundColor Gray
}

Write-Header " Ext4Fsd user-mode release (Ext2Srv + iced)"

$versionLabel = Resolve-Version
if (-not $OutDir) {
    $OutDir = Join-Path $script:BuildRoot "dist"
}
$Platforms = Resolve-ReleasePlatforms
$stageRoot = Join-Path $OutDir "usermode-$versionLabel"
Write-Host "Repo:     $script:BuildRoot"
Write-Host "Version:  $versionLabel"
Write-Host "Out:      $stageRoot"
Write-Host ""

$msbuild = $null
$vsRoot = $null
if (-not $SkipExt2Srv) {
    $msbuild = Find-MSBuild
    if (-not $msbuild) {
        Write-Host "ERROR: MSBuild not found (need VS 2019/2022 or Build Tools)." -ForegroundColor Red
        exit 1
    }
    Write-Host "MSBuild: $msbuild" -ForegroundColor Green
    $vsRoot = Get-VSInstallRootFromMSBuild $msbuild

    # Whatever is in $Platforms (host default or explicit) must be buildable.
    if (($Platforms -contains "ARM64") -and -not (Test-MsvcArm64Tools $vsRoot)) {
        Write-Arm64ToolchainHelp $vsRoot
        Write-Host "No build was started (fix the toolchain, then re-run)." -ForegroundColor Yellow
        exit 2
    }
}

if (-not $SkipIced) {
    $cargo = Get-Command cargo -ErrorAction SilentlyContinue
    if (-not $cargo) {
        Write-Host "ERROR: cargo not found on PATH." -ForegroundColor Red
        exit 1
    }
    Write-Host "Cargo:   $($cargo.Source)" -ForegroundColor Green
}

New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
$notesPath = Join-Path $stageRoot "RELEASE_NOTES.txt"
New-ReleaseNotes -path $notesPath -versionLabel $versionLabel

$ext2SrvProject = Join-Path $script:BuildRoot "Ext2Srv\Ext2Srv.vcxproj"
$icedDir = Join-Path $script:BuildRoot "ext2mgr_iced"
$solutionDir = $script:BuildRoot
if (-not $solutionDir.EndsWith("\")) {
    $solutionDir += "\"
}

foreach ($platform in $Platforms) {
    Write-Header " Platform $platform"

    $archFolder = switch ($platform) {
        "x64" { "x64" }
        "ARM64" { "arm64" }
    }
    $archOut = Join-Path $stageRoot $archFolder
    New-Item -ItemType Directory -Force -Path $archOut | Out-Null

    if (-not $SkipExt2Srv) {
        Write-Host "Building Ext2Srv ($platform Release)..." -ForegroundColor Yellow
        if ($Clean) {
            & $msbuild $ext2SrvProject /t:Clean /p:Configuration=Release /p:Platform=$platform /p:SolutionDir=$solutionDir /v:minimal /nologo
        }
        & $msbuild $ext2SrvProject `
            /t:Build `
            /p:Configuration=Release `
            /p:Platform=$platform `
            /p:SolutionDir=$solutionDir `
            /v:minimal `
            /m `
            /nologo
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Ext2Srv build failed for $platform" -ForegroundColor Red
            if ($platform -eq "ARM64") {
                $vsRootHint = Get-VSInstallRootFromMSBuild $msbuild
                Write-Arm64ToolchainHelp $vsRootHint
            }
            exit $LASTEXITCODE
        }
        $builtSrv = Join-Path (Get-Ext2SrvOutDir "Release" $platform) "Ext2Srv.exe"
        if (-not (Test-Path -LiteralPath $builtSrv)) {
            Write-Host "ERROR: missing $builtSrv" -ForegroundColor Red
            exit 1
        }
        Copy-Item -LiteralPath $builtSrv -Destination (Join-Path $archOut "Ext2Srv.exe") -Force
        Write-Host "  staged Ext2Srv.exe" -ForegroundColor Green
    }

    if (-not $SkipIced) {
        $rustTarget = Get-RustTarget $platform
        Write-Host "Ensuring Rust target $rustTarget ..." -ForegroundColor Yellow
        & rustup target add $rustTarget
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: rustup target add failed for $rustTarget" -ForegroundColor Red
            exit $LASTEXITCODE
        }
        Write-Host "Building ext2mgr_iced ($rustTarget release)..." -ForegroundColor Yellow
        Push-Location $icedDir
        try {
            $cargoArgs = @("build", "--release", "--target", $rustTarget)
            if ($Clean) {
                & cargo clean --release --target $rustTarget
            }
            & cargo @cargoArgs
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: cargo build failed for $rustTarget" -ForegroundColor Red
                exit $LASTEXITCODE
            }
        } finally {
            Pop-Location
        }
        $builtIced = Join-Path $icedDir "target\$rustTarget\release\ext2mgr_iced.exe"
        if (-not (Test-Path -LiteralPath $builtIced)) {
            Write-Host "ERROR: missing $builtIced" -ForegroundColor Red
            exit 1
        }
        Copy-Item -LiteralPath $builtIced -Destination (Join-Path $archOut "ext2mgr_iced.exe") -Force
        Write-Host "  staged ext2mgr_iced.exe" -ForegroundColor Green
    }

    Write-Sha256Sums $archOut

    if (-not $SkipZip) {
        $zipPath = Join-Path $stageRoot "Ext4Fsd-usermode-$versionLabel-$archFolder.zip"
        if (Test-Path -LiteralPath $zipPath) {
            Remove-Item -LiteralPath $zipPath -Force
        }
        # Zip arch folder contents + top-level RELEASE_NOTES
        $zipStage = Join-Path $env:TEMP "ext4fsd-usermode-zip-$archFolder"
        if (Test-Path -LiteralPath $zipStage) {
            Remove-Item -LiteralPath $zipStage -Recurse -Force
        }
        New-Item -ItemType Directory -Force -Path $zipStage | Out-Null
        Copy-Item -LiteralPath (Join-Path $archOut "*") -Destination $zipStage -Force
        Copy-Item -LiteralPath $notesPath -Destination $zipStage -Force
        Compress-Archive -Path (Join-Path $zipStage "*") -DestinationPath $zipPath -Force
        Remove-Item -LiteralPath $zipStage -Recurse -Force
        Write-Host "  zip $zipPath" -ForegroundColor Green
    }
}

Write-Header " Done"
Write-Host "Artifacts under: $stageRoot" -ForegroundColor Green
Get-ChildItem -LiteralPath $stageRoot -Recurse -File | ForEach-Object {
    $rel = $_.FullName.Substring($stageRoot.Length).TrimStart("\")
    Write-Host ("  {0,12}  {1}" -f $_.Length, $rel) -ForegroundColor Gray
}
Write-Host ""
Write-Host "Attach the per-arch .zip files (and RELEASE_NOTES.txt) to your GitHub/GitLab release." -ForegroundColor Cyan
Write-Host "Driver remains upstream - this package is user-mode only." -ForegroundColor Yellow
