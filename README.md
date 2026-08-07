Ext2Mgr Iced port (this branch)
-------------------------------

This branch (`ext2mgr-iced`) adds an optional **Rust + [Iced](https://iced.rs/) GUI** for managing Ext2/Ext4 volumes on Windows, under **`ext2mgr_iced/`**. It is modeled on classic MFC **Ext2 Volume Manager** (`Ext2Mgr/`) and talks to the same Ext2Fsd driver / Ext2Srv pipe.

- Build / run: see [`ext2mgr_iced/README.md`](ext2mgr_iced/README.md) (`cargo build --release`, `cargo run`, `cargo run --release`)
- What differs from classic Ext2Mgr: [`ext2mgr_iced/PORT_IMPROVEMENTS.md`](ext2mgr_iced/PORT_IMPROVEMENTS.md)
- Source layout: [`ext2mgr_iced/docs/SOURCE_LAYOUT.md`](ext2mgr_iced/docs/SOURCE_LAYOUT.md)

Classic Ext2Mgr and Ext2Srv remain available. Driver build/install scripts below are unchanged.


Scripts and building from source (this fork)
--------------------------------------------

This fork adds PowerShell scripts for building, installing, signing, and diagnosing the driver. All scripts live in **Scripts/**. Run them from the repo root, e.g. `.\Scripts\build.ps1`.

**Build and install**
- **Scripts\build.ps1** — Build driver, Ext2Srv, and/or Ext2Mgr (Visual Studio 2019/2022). Default `-Target All` needs WDK. Use `-Target Ext2Mgr` or `-Target Ext2Srv` for user-mode only (no WDK). Optionally signs the driver if `EXT4FSD_CERT_PATH` is set. Cursor/VS Code: `.vscode/tasks.json` + `launch.json`.
- **Scripts\release_usermode.ps1** — Package Ext2Srv + `ext2mgr_iced` user-mode zips (no driver / no WDK). Default: **host architecture only**. Pass `-Platforms x64,ARM64` for both (requires ARM64 MSVC tools). Writes `dist\usermode-<version>\` with `RELEASE_NOTES.txt` and SHA256 sums.
- **Scripts\install_driver.ps1** — Copy driver to System32, register kernel driver, install Ext2Srv. Run as Administrator.
- **Scripts\uninstall_driver.ps1** — Remove driver and services.
- **Scripts\register_driver.ps1** — Register the kernel driver service (driver must already be in System32).
- **Scripts\disable_driver.ps1** — Disable the Ext2Fsd service (e.g. to stop boot retries after signature failures).

**Signing**
- **Scripts\sign_driver.ps1** — Sign the driver with a code-signing certificate. Set `EXT4FSD_CERT_PATH` and optionally `MSIX_CERT_PASSWORD`, or pass `-CertificatePath` / `-CertificatePassword`.
- **Scripts\install_certificate.ps1** — Install the signing certificate into Trusted Publishers (required for loading self-signed drivers). Run as Administrator.

**Diagnostics**
- **Scripts\diagnose_ext2fsd.ps1** — Unified diagnostic (signature, cert stores, event log, driver status). Use for load failures or error 577. Use `-UseSigntool:$false` to skip signtool (e.g. when SDK is not installed).
- **Scripts\check_driver_load_error.ps1**, **Scripts\diagnose_error_577.ps1**, **Scripts\verify_driver_signature.ps1** — Wrappers that call `diagnose_ext2fsd.ps1`.

**Other**
- **Scripts\fix_sdk_version.ps1** — Update project files' `WindowsTargetPlatformVersion` to match the installed SDK.


Latest release
--------------

    Signed driver for Windows 10 and Windows 11:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup.exe

    Updated Ext2Mgr https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Mgr.exe
    that gives more detailed information on the type of filesystems on
    the disk. If an on disk filesystem contains new ext4 features that
    is not supported by the Windows driver it will show a '+' sign after
    the filesystem name, e.g "EXT4+". You can run this application
    together with an already installed driver.


Changes to the source code in git after latest release
------------------------------------------------------

    Most users can continue to use the latest release, it contains
    an install program and a signed driver. Below is a list of
    what has been implemented in the source code in git since then.
    If you need any of these features you can compile the driver
    or application yourself.

    Driver:

    - Corrected an error that made the debug version of the driver
      crash. A number of calls to Ext2FreePool where misstakenly
      replaced with direct calls to ExFreePool. This error can
      not happen in the release version of the driver.

    - The existing time fields in the superblock and the inodes
      is using 32-bit values for time in seconds since 1970. They
      will overflow in 2038. The ext4 filesystem has therefore
      been extended with new time fields. In the superblock they
      have names ending in "_hi" containing the high 8-bit of the
      the seconds while the existing time fields contain the lower
      32-bit. In the inodes the new fileds has names ending in
      "_extra". They contain both the high 2-bit of the seconds,
      that is bit 33 and 34 and also the nano seconds encoded as
      (nsec << 2 | epoch) The existing fields contain the low 32-bit
      of the seconds.

    - The call "query volume information" will read the fields
      s_mkfs_time and s_mkfs_time_hi from the superblock.

    - The fields s_mtime and s_mtime_hi in the superblock will be
      updated with the current time at mount time.

    - The fields s_wtime and s_wtime_hi in the superblock will be
      updated with the current time at shutdown.

    Application:

    - If an on disk filesystem contains new ext4 features that is
      not supported by the Windows driver a '+' sign will be shown
      after the filesystem name, e.g "EXT4+".

    - More filesystems are recognized. The main window with
      a list of disks and partitions will now tell the type of
      filesystem for BTRFS, XFS, BSD, LVM and RAID (MD) in
      adidition to EXT

    - The used size of swap partitions is listed as zero.

    - The donate dialog box is disabled because the information in
      it is outdated.


About
-----

    This is a branch of the Ext2Fsd project by Matt Wu where I try to
    implement support for metadata checksums and jbd2. I have also
    updated the project so it can be compiled with Visual Studio 2019
    and Visual Studio 2022.
    The current status of the development is that all metadata checksums
    is implemented and jbd2 is ported to support 64-bit blocknumbers.
    The driver is now ready to be tested!
    This work is dedicated to my mother Berit Ingegerd Branten.
    Bo Branten <bosse@accum.se>


Test
----

    To test this driver run the installation programs:
    Signed driver for Windows 10 and Windows 11:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup.exe
    Signed driver files for manual install: (even ARM/ARM64)
    https://www.accum.se/~bosse/ext2fsd/0.71/signed/
    Unsigned driver for Windows XP, Windows Vista, Windows 7 and Windows 8:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup-xp.exe

    If you compile the driver yourself you only need to run the installation
    program once, then you can copy your driver file over the old in
    \windows\system32\drivers.
    Now you can read and write ext4 filesystems using the new features
    metadata checksums and 64-bit blocknumbers from Windows.


Introduction
------------

    Ext4Fsd is an ext2/3/4 file system driver for Windows (XP/Vista/7/8/10/11).
    It's a free and open-source software, everyone can modify or distribute
    under GNU GPLv2.


Active Developers
-----------------

    Bo Branten : http://github.com/bobranten
                 http://www.accum.se/~bosse

    Thanks to Olof Lagerkvist https://github.com/LTRData
    for important help to this project!


Previous Developers
-------------------

    Matt Wu : http://github.com/matt-wu
              http://blog.dynox.cn

    KaHo Ng : http://github.com/ngkaho1234


Old Development Website
-----------------------

    Matt Wu <mattwu@163.com>
    http://www.ext2fsd.com


Supported Features by Ext4Fsd
-----------------------------

    1, flexible inode size: > 128 bytes, up to block size
    2, dir_index:    htree directory index
    3, filetype:     extra file mode in dentry
    4, large_file:   > 4G files supported
    5, sparse_super: super block backup in group descriptor
    6, uninit_bg:    fast fsck and group checksum
    7, extent:       full support with extending and shrinking.
    8, journal:      only support replay for internal journal
    9, flex_bg:      first flexible metadata group
    10, symlink and hardlink
    11, mount-as-user: specifed uid/gid by user


Read-only Ext4 Features
-----------------------

    If any of the ext4 features below is present on a filesystem
    the Windows driver will automatically mount the fs read-only:

    1, EXT4_FEATURE_RO_COMPAT_BIGALLOC
    2, EXT4_FEATURE_RO_COMPAT_QUOTA
    3, EXT4_FEATURE_RO_COMPAT_PROJECT
    4, EXT4_FEATURE_RO_COMPAT_VERITY
    5, EXT4_FEATURE_RO_COMPAT_ORPHAN_PRESENT


Unsupported Ext4 Features
---------------------------

    If any of the ext4 features below is present on a filesystem
    the Windows driver can not mount the fs at all:

    1, EXT4_FEATURE_INCOMPAT_EA_INODE (extended attributes)
    2, EXT4_FEATURE_INCOMPAT_MMP (multiple mount protection)
    3, EXT4_FEATURE_INCOMPAT_INLINE_DATA (storing small files in inode)
    4, EXT4_FEATURE_INCOMPAT_ENCRYPT
    5, EXT4_FEATURE_INCOMPAT_CASEFOLD (case insensitive file names (claimed to be used by SteamOS as default))
    6, EXT4_FEATURE_INCOMPAT_LARGEDIR (3-level htree)
