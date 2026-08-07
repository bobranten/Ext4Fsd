//! Mount / unmount helpers.
//!
//! Persistence modes (same idea as Ext2Mgr’s letter picker):
//! - **Temporary** — `DefineDosDevice` only (Ext2Srv, or local when elevated). Lost after reboot.
//! - **Mount Manager** — `SetVolumeMountPoint` (Windows MountMgr). Survives reboot; Explorer-native.
//! - **Session Manager DOS Devices** — registry under
//!   `HKLM\...\Session Manager\DOS Devices`. Survives reboot. Classic also calls
//!   `DefineDosDevice` afterward so the letter appears immediately; that assign is
//!   not the same option as Temporary.
//! - **Ext2Fsd automount** — `Volumes\{UUID}` registry; remounted on refresh.
//!
//! Device path for registry / DefineDosDevice is `\Device\HarddiskVolumeN`
//! (Ext2Mgr `Volume->Name`).


use std::mem::size_of;
use std::os::windows::ffi::OsStrExt;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MountMode {
    /// Option: Mount via DefineDosDevice only (lost after reboot).
    Temporary,
    /// Option: Windows Mount Manager (`SetVolumeMountPoint`).
    MountManager,
    /// Option: Session Manager DOS Devices registry (survives reboot).
    PermanentRegistry,
    /// Ext2Fsd `Volumes\{UUID}` automount — config persists; remounted by manager/driver.
    Ext2Automount,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct MountRequest {
    pub letter: char,
    pub mode: MountMode,
    /// NT path written to Session Manager and/or passed to DefineDosDevice
    /// (`\Device\HarddiskVolumeN`).
    pub symlink: String,
    pub win32_volume_name: String,
    /// Required for Ext2Automount.
    pub uuid: Option<[u8; 16]>,
    pub codepage: String,
    pub readonly: bool,
}

pub fn free_drive_letters() -> Vec<char> {
    let mask = unsafe { windows_sys::Win32::Storage::FileSystem::GetLogicalDrives() };
    let mut letters = Vec::new();
    for letter_index in 2u32..26 {
        if mask & (1 << letter_index) == 0 {
            letters.push(char::from(b'A' + letter_index as u8));
        }
    }
    letters
}

pub fn first_free_drive_letter() -> Option<char> {
    free_drive_letters().into_iter().next()
}

/// Prefer `physical_object` when it is `\Device\...` (Ext2Mgr `Volume->Name`).
pub fn dos_device_target(physical_object: &str, symlink: &str) -> String {
    if physical_object.starts_with(r"\Device\") {
        physical_object.to_string()
    } else if symlink.starts_with(r"\Device\") {
        symlink.to_string()
    } else if !physical_object.is_empty() {
        physical_object.to_string()
    } else {
        symlink.to_string()
    }
}

pub fn mount_volume(request: &MountRequest) -> Result<String, String> {
    if request.symlink.contains(r"\Partition")
        && !request.symlink.to_ascii_uppercase().contains(r"\HARDDISKVOLUME")
    {
        return Err(format!(
            "Refusing drive letter {}: target {} is a partition path, not \
             \\Device\\HarddiskVolumeN. Remount / refresh so Ext2Fsd exposes the volume.",
            request.letter, request.symlink
        ));
    }
    // Classic AddMountPoint: try Ext2SetExt2Property before Assign, but still
    // DefineDosDevice when QUERY/SET fails — unlettered RAW volumes are not yet
    // Ext2Fsd VCBs (QUERY_PROPERTY3 → ERROR_INVALID_PARAMETER / 87).
    let _ = set_driver_automount_letter(request);

    let message = match request.mode {
        MountMode::Temporary => {
            define_temporary(request.letter, &request.symlink)?;
            format!(
                "Mounted {}: temporary via DefineDosDevice (lost after reboot)",
                request.letter
            )
        }
        MountMode::MountManager => {
            assign_via_mount_manager(
                request.letter,
                &request.symlink,
                &request.win32_volume_name,
            )?;
            // Explorer notify is done on the iced UI thread (PipeOpFinished).
            format!(
                "Mounted {}: permanent via Mount Manager (SetVolumeMountPoint)",
                request.letter
            )
        }
        MountMode::PermanentRegistry => {
            // Registry is the Session Manager option; DefineDosDevice is only the
            // immediate bind (same sequence as Ext2Mgr AddMountPoint + bRegistry).
            // If the letter already targets this device, only write the registry —
            // classic still EndDialog after SetRegistry even when Assign is a no-op.
            let reg_result =
                super::persist::set_registry_mount_point(request.letter, &request.symlink);
            reg_result?;
            let already_live = query_dos_device(request.letter)
                .is_some_and(|live| live.eq_ignore_ascii_case(&request.symlink));
            if !already_live {
                define_temporary(request.letter, &request.symlink)?;
            }
            // Explorer notify is done on the iced UI thread (PipeOpFinished).
            format!(
                "Mounted {}: Session Manager DOS Devices (registry{})",
                request.letter,
                if already_live {
                    "; letter already live"
                } else {
                    "; immediate DefineDosDevice bind"
                }
            )
        }
        MountMode::Ext2Automount => {
            let uuid = request.uuid.ok_or_else(|| {
                "Ext2Fsd automount needs an EXT volume UUID (select an EXT2/3/4 volume)".to_string()
            })?;
            super::persist::store_ext2_automount(
                &uuid,
                request.letter,
                &request.codepage,
                request.readonly,
            )?;
            define_temporary(request.letter, &request.symlink)?;
            format!(
                "Mounted {}: Ext2Fsd automount saved under Volumes\\{} (persists; remounted on refresh)",
                request.letter,
                super::persist::format_volume_uuid(&uuid)
            )
        }
    };
    // Do not call MountMgr VOLUME_ARRIVAL_NOTIFICATION here — classic
    // Ext2AssignDrvLetter(Temporary) does not, and ArrivalNotify can invent a
    // second Mount Manager letter (G:/H:) beside the one the user chose.
    // After DefineDosDevice, Ext2Fsd may have claimed the volume — retry SET_PROPERTY3
    // via \\.\X: (classic path when the volume is already an Ext2 VCB).
    let _ = set_driver_automount_letter(request);
    let attrs = letter_root_attrs(request.letter);
    if attrs.starts_with("fail") {
        return Err(format!(
            "Drive letter {0}: is bound to {1} but {0}:\\ is not accessible ({attrs}). \
             Ext2Fsd did not mount a usable filesystem (compare with classic Ext2Mgr).",
            request.letter, request.symlink
        ));
    }
    // Volumes\{UUID} is written only by MountMode::Ext2Automount (and classic
    // AddMountPoint registry store) — not for Temporary / Mount Manager / Session.
    Ok(message)
}

/// Remove a drive letter. Only calls `DeleteVolumeMountPoint` when `win32_volume_name`
/// is set and Mount Manager actually lists this letter for that GUID — never open
/// `X:\` for pure Ext2 DefineDosDevice / Session Manager binds (hangs / freezes).
///
/// Prefer the live `QueryDosDevice` target over `symlink_hint`. Volume rows often
/// carry `\??\Volume{GUID}` while Temporary/Session binds use `\Device\HarddiskVolumeN`;
/// `DDD_EXACT_MATCH_ON_REMOVE` with the GUID path fails instantly with
/// "Failed to remove drive letter".
pub fn unmount_letter(
    letter: char,
    symlink_hint: &str,
    win32_volume_name: &str,
) -> Result<(), String> {
    unmount_letter_ex(letter, symlink_hint, win32_volume_name, None)
}

pub fn unmount_letter_ex(
    letter: char,
    symlink_hint: &str,
    win32_volume_name: &str,
    volume_uuid: Option<[u8; 16]>,
) -> Result<(), String> {
    let _ = super::persist::clear_registry_mount_point(letter);
    if let Some(uuid) = volume_uuid {
        if super::persist::query_ext2_automount_letter(&uuid) == Some(letter.to_ascii_uppercase())
        {
            let _ = super::persist::clear_ext2_automount_mount_point(&uuid);
        }
    }

    let letter_upper = letter.to_ascii_uppercase();
    let mountmgr_owns = !win32_volume_name.is_empty()
        && super::dead_letters::mountmgr_letters_for_volume(win32_volume_name)
            .contains(&letter_upper);
    if mountmgr_owns {
        let mount_root = format!(r"{letter}:\");
        let _ = delete_volume_mount_point(&mount_root);
    }

    if query_dos_device(letter).is_none() {
        return finish_unmount_ok(letter);
    }

    let mut targets = Vec::new();
    if let Some(live) = query_dos_device(letter) {
        push_unique_target(&mut targets, live);
    }
    if !symlink_hint.is_empty() {
        push_unique_target(&mut targets, symlink_hint.to_string());
    }
    if let Ok(query) = super::pipe::with_shared_client(|client| client.query_drive(letter as u8)) {
        if query.result && !query.symlink.is_empty() {
            push_unique_target(&mut targets, query.symlink);
        }
    }

    // Vista+: classic removes via Ext2Srv first (global DOS devices). Local
    // DefineDosDevice only affects the elevated session.
    let mut last_pipe_err = None;
    for target in &targets {
        match super::pipe::with_shared_client(|client| {
            client.remove_drive(letter as u8, target)
        }) {
            Ok(true) | Ok(false) if query_dos_device(letter).is_none() => {
                return finish_unmount_ok(letter);
            }
            Ok(_) => {}
            Err(err) => {
                if query_dos_device(letter).is_none() {
                    return finish_unmount_ok(letter);
                }
                last_pipe_err = Some(err.to_string());
            }
        }
    }
    for target in &targets {
        if define_dos_device_local_remove(letter, target) && query_dos_device(letter).is_none() {
            return finish_unmount_ok(letter);
        }
    }
    // Same loose fallback as dead-letter remove (no EXACT_MATCH).
    if define_dos_device_local_remove_loose(letter) && query_dos_device(letter).is_none() {
        return finish_unmount_ok(letter);
    }

    if query_dos_device(letter).is_none() {
        finish_unmount_ok(letter)
    } else if let Some(err) = last_pipe_err {
        Err(format!("Failed to remove drive letter {letter}:\n{err}"))
    } else {
        let tried = if targets.is_empty() {
            "(no DOS target)".to_string()
        } else {
            targets.join(", ")
        };
        Err(format!(
            "Failed to remove drive letter {letter}:\n(tried DefineDosDevice/Ext2Srv against {tried})"
        ))
    }
}

fn finish_unmount_ok(letter: char) -> Result<(), String> {
    // Explorer notify is done on the iced UI thread (PipeOpFinished).
    let _ = letter;
    Ok(())
}

fn push_unique_target(targets: &mut Vec<String>, candidate: String) {
    if candidate.is_empty() {
        return;
    }
    if targets
        .iter()
        .any(|existing| existing.eq_ignore_ascii_case(&candidate))
    {
        return;
    }
    targets.push(candidate);
}

pub fn pipe_status_str() -> String {
    match super::pipe::health_check() {
        Ok(()) => "Ext2Srv pipe: connected.".to_string(),
        Err(err) => format!("Ext2Srv pipe: {err} (is Ext2Srv running?)"),
    }
}

fn define_temporary(letter: char, symlink: &str) -> Result<(), String> {
    // Classic `Ext2DefineDosDevice` / `CanDoLocalMount()`: on Vista+ always use
    // Ext2Srv. Elevated `DefineDosDevice` without the service is session-private
    // and invisible to Explorer (medium IL).
    let ok = super::pipe::with_shared_client(|client| client.define_drive(letter as u8, symlink))
        .map_err(|err| format!("Ext2Fsd service is not started.\n(Ext2Srv pipe: {err})"))?;
    let query = query_dos_device(letter).unwrap_or_default();
    if ok {
        if query.is_empty() {
            return Err(format!(
                "Failed to assign new drive letter {letter}: Ext2Srv reported success but QueryDosDevice is empty"
            ));
        }
        // Explorer notify is done on the iced UI thread (PipeOpFinished).
        Ok(())
    } else {
        Err(format!("Failed to assign new drive letter {letter}:"))
    }
}

/// Ext2Mgr `Ext2DrvNotify` — call from the **UI thread** after assign/remove.
pub fn notify_explorer_letter(letter: char, arrival: bool) {
    drv_notify(letter, arrival);
}

/// Ext2Mgr `Ext2DrvNotify` — wake Explorer after DefineDosDevice add/remove.
fn drv_notify(letter: char, arrival: bool) {
    #[repr(C)]
    struct DevBroadcastVolume {
        size: u32,
        device_type: u32,
        reserved: u32,
        unitmask: u32,
        flags: u16,
    }

    const DBT_DEVTYP_VOLUME: u32 = 0x0000_0002;
    const DBTF_NET: u16 = 0x0001;
    const DBT_DEVICEARRIVAL: usize = 0x8000;
    const DBT_DEVICEREMOVECOMPLETE: usize = 0x8004;
    const WM_DEVICECHANGE: u32 = 0x0219;
    const BSM_APPLICATIONS: u32 = 0x0000_0008;
    const BSF_IGNORECURRENTTASK: u32 = 0x0000_0002;
    const BSF_FORCEIFHUNG: u32 = 0x0000_0020;
    const BSF_NOHANG: u32 = 0x0000_0008;
    const BSF_NOTIMEOUTIFNOTHUNG: u32 = 0x0000_0040;
    const SHCNE_DRIVEADD: i32 = 0x0000_0100;
    const SHCNE_DRIVEREMOVED: i32 = 0x0000_0080;
    const SHCNF_PATHW: u32 = 0x0005;

    #[link(name = "user32")]
    extern "system" {
        fn BroadcastSystemMessageW(
            flags: u32,
            recipients: *mut u32,
            message: u32,
            wparam: usize,
            lparam: isize,
        ) -> i32;
    }
    #[link(name = "shell32")]
    extern "system" {
        fn SHChangeNotify(event_id: i32, flags: u32, item1: *const u16, item2: *const u16);
    }

    let letter = letter.to_ascii_uppercase();
    if !letter.is_ascii_alphabetic() {
        return;
    }
    let unit = (letter as u8 - b'A') as u32;
    let mut dbv = DevBroadcastVolume {
        size: size_of::<DevBroadcastVolume>() as u32,
        device_type: DBT_DEVTYP_VOLUME,
        reserved: 0,
        unitmask: 1u32 << unit,
        flags: DBTF_NET,
    };
    let mut recipients = BSM_APPLICATIONS;
    let event = if arrival {
        DBT_DEVICEARRIVAL
    } else {
        DBT_DEVICEREMOVECOMPLETE
    };
    unsafe {
        BroadcastSystemMessageW(
            BSF_IGNORECURRENTTASK | BSF_FORCEIFHUNG | BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
            &mut recipients,
            WM_DEVICECHANGE,
            event,
            &mut dbv as *mut DevBroadcastVolume as isize,
        );
    }
    // Shell often ignores DBT alone for DefineDosDevice letters — also poke Explorer.
    let root = format!(r"{letter}:\");
    let root_w = to_wide(&root);
    unsafe {
        SHChangeNotify(
            if arrival {
                SHCNE_DRIVEADD
            } else {
                SHCNE_DRIVEREMOVED
            },
            SHCNF_PATHW,
            root_w.as_ptr(),
            std::ptr::null(),
        );
    }
}

fn set_driver_automount_letter(request: &MountRequest) -> Result<(), String> {
    const IOCTL_APP_VOLUME_PROPERTY: u32 = 0x0022_1F40;
    const EXT2_VOLUME_PROPERTY_MAGIC: u32 = 0x4556_504D;
    const APP_CMD_QUERY_PROPERTY3: u32 = 0x0000_0006;
    const APP_CMD_SET_PROPERTY3: u32 = 0x0000_0007;
    const EXT2_VPROP3_AUTOMOUNT: u64 = 1;
    const DRV_LETTER_OFF: usize = 64;
    const FLAGS2_OFF: usize = 132;
    const PROP3_SIZE: usize = 268;

    let mut last_err = String::from("no open path");
    let mut saw_ioctl_err = false;
    for open_path in volume_open_paths_for_property(request) {
        let handle = match open_volume_for_ioctl(&open_path) {
            Some(handle) => handle,
            None => {
                let open_err = format!("CreateFile({open_path}) failed ({})",
                    unsafe { windows_sys::Win32::Foundation::GetLastError() });
                if !saw_ioctl_err {
                    last_err = open_err;
                }
                continue;
            }
        };
        let mut buffer = vec![0u8; PROP3_SIZE];
        buffer[0..4].copy_from_slice(&EXT2_VOLUME_PROPERTY_MAGIC.to_le_bytes());
        buffer[8..12].copy_from_slice(&APP_CMD_QUERY_PROPERTY3.to_le_bytes());
        let mut bytes_returned = 0u32;
        let query_ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_APP_VOLUME_PROPERTY,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        if query_ok == 0 {
            last_err = format!(
                "QUERY_PROPERTY3({open_path}) failed ({})",
                unsafe { windows_sys::Win32::Foundation::GetLastError() }
            );
            saw_ioctl_err = true;
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            continue;
        }

        let letter = request.letter.to_ascii_uppercase() as u8;
        buffer[DRV_LETTER_OFF] = letter | 0x80;
        let flags2 = u64::from_le_bytes(buffer[FLAGS2_OFF..FLAGS2_OFF + 8].try_into().unwrap_or([0; 8]));
        buffer[FLAGS2_OFF..FLAGS2_OFF + 8]
            .copy_from_slice(&(flags2 | EXT2_VPROP3_AUTOMOUNT).to_le_bytes());
        buffer[0..4].copy_from_slice(&EXT2_VOLUME_PROPERTY_MAGIC.to_le_bytes());
        buffer[8..12].copy_from_slice(&APP_CMD_SET_PROPERTY3.to_le_bytes());

        let set_ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_APP_VOLUME_PROPERTY,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        if set_ok != 0 {
            return Ok(());
        }
        last_err = format!(
            "SET_PROPERTY3({open_path}) failed ({})",
            unsafe { windows_sys::Win32::Foundation::GetLastError() }
        );
        saw_ioctl_err = true;
    }
    Err(last_err)
}

fn volume_open_paths_for_property(request: &MountRequest) -> Vec<String> {
    let mut paths = Vec::new();
    // Prefer live letter once DefineDosDevice has bound it (Ext2Fsd VCB path).
    if query_dos_device(request.letter).is_some() {
        paths.push(format!(r"\\.\{}:", request.letter));
    }
    if request.symlink.starts_with(r"\Device\") {
        let trimmed = request.symlink.trim_start_matches('\\');
        paths.push(format!(r"\\?\GLOBALROOT\{trimmed}"));
        if let Some(rest) = request.symlink.strip_prefix(r"\Device\") {
            paths.push(format!(r"\\.\{rest}"));
        }
    }
    let win32 = request.win32_volume_name.trim_end_matches('\\');
    if !win32.is_empty() {
        paths.push(win32.to_string());
    }
    if query_dos_device(request.letter).is_none() {
        paths.push(format!(r"\\.\{}:", request.letter));
    }
    paths
}

fn open_volume_for_ioctl(path: &str) -> Option<windows_sys::Win32::Foundation::HANDLE> {
    const GENERIC_READ: u32 = 0x8000_0000;
    const GENERIC_WRITE: u32 = 0x4000_0000;
    let wide = to_wide(path);
    for access in [GENERIC_READ | GENERIC_WRITE, GENERIC_READ, 0u32] {
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                access,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle != windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            return Some(handle);
        }
    }
    None
}

fn letter_root_attrs(letter: char) -> String {
    let root = format!(r"{letter}:\");
    let wide = to_wide(&root);
    let attrs = unsafe {
        windows_sys::Win32::Storage::FileSystem::GetFileAttributesW(wide.as_ptr())
    };
    if attrs == u32::MAX {
        format!(
            "fail_err={}",
            unsafe { windows_sys::Win32::Foundation::GetLastError() }
        )
    } else {
        format!("ok=0x{attrs:x}")
    }
}

/// Ext2Mgr `Ext2AssignDrvLetter(..., bMountMgr=TRUE)` / `Ext2InsertMountPoint`.
fn assign_via_mount_manager(
    letter: char,
    symlink: &str,
    win32_volume_name: &str,
) -> Result<(), String> {
    let mount_root = format!(r"{letter}:\");
    let volume_name = if !win32_volume_name.is_empty() {
        let mut name = win32_volume_name.to_string();
        if !name.ends_with('\\') {
            name.push('\\');
        }
        name
    } else {
        // Classic: temporary DOS bind → query GUID name → remove DOS → SetVolumeMountPoint.
        define_temporary(letter, symlink)?;
        let probed = match volume_name_for_mount_point(&mount_root) {
            Ok(name) => name,
            Err(err) => {
                let _ = super::pipe::with_shared_client(|client| {
                    client.remove_drive(letter as u8, symlink)
                });
                let _ = define_dos_device_local_remove(letter, symlink);
                return Err(err);
            }
        };
        let _ = super::pipe::with_shared_client(|client| {
            client.remove_drive(letter as u8, symlink)
        });
        if query_dos_device(letter).is_some() {
            let _ = define_dos_device_local_remove(letter, symlink);
            let _ = define_dos_device_local_remove_loose(letter);
        }
        probed
    };
    set_volume_mount_point(&mount_root, &volume_name)
}

fn volume_name_for_mount_point(mount_root: &str) -> Result<String, String> {
    let wide = to_wide(mount_root);
    let mut name = vec![0u16; 256];
    let ok = unsafe {
        windows_sys::Win32::Storage::FileSystem::GetVolumeNameForVolumeMountPointW(
            wide.as_ptr(),
            name.as_mut_ptr(),
            name.len() as u32,
        )
    };
    if ok == 0 {
        let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
        return Err(format!(
            "GetVolumeNameForVolumeMountPoint({mount_root}) failed ({code})"
        ));
    }
    let end = name
        .iter()
        .position(|&unit| unit == 0)
        .unwrap_or(name.len());
    Ok(String::from_utf16_lossy(&name[..end]))
}

fn set_volume_mount_point(mount_root: &str, volume_name: &str) -> Result<(), String> {
    let root_w = to_wide(mount_root);
    let volume_w = to_wide(volume_name);
    let ok = unsafe {
        windows_sys::Win32::Storage::FileSystem::SetVolumeMountPointW(
            root_w.as_ptr(),
            volume_w.as_ptr(),
        )
    };
    if ok == 0 {
        let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
        Err(format!(
            "SetVolumeMountPoint({mount_root}) failed ({code})"
        ))
    } else {
        Ok(())
    }
}

fn to_wide(value: &str) -> Vec<u16> {
    std::ffi::OsStr::new(value)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect()
}

fn define_dos_device_local_remove(letter: char, symlink: &str) -> bool {
    let dos = format!("{letter}:");
    let dos_wide = to_wide(&dos);
    let target_wide = to_wide(symlink);
    let flags = super::pipe::DDD_RAW_TARGET_PATH
        | super::pipe::DDD_REMOVE_DEFINITION
        | super::pipe::DDD_EXACT_MATCH_ON_REMOVE;
    unsafe {
        windows_sys::Win32::Storage::FileSystem::DefineDosDeviceW(
            flags,
            dos_wide.as_ptr(),
            target_wide.as_ptr(),
        ) != 0
    }
}

fn define_dos_device_local_remove_loose(letter: char) -> bool {
    let dos = format!("{letter}:");
    let dos_wide = to_wide(&dos);
    unsafe {
        windows_sys::Win32::Storage::FileSystem::DefineDosDeviceW(
            super::pipe::DDD_REMOVE_DEFINITION,
            dos_wide.as_ptr(),
            std::ptr::null(),
        ) != 0
    }
}

fn query_dos_device(letter: char) -> Option<String> {
    let dos = format!("{letter}:");
    let wide = to_wide(&dos);
    let mut target = vec![0u16; 512];
    let written = unsafe {
        windows_sys::Win32::Storage::FileSystem::QueryDosDeviceW(
            wide.as_ptr(),
            target.as_mut_ptr(),
            target.len() as u32,
        )
    };
    if written == 0 {
        None
    } else {
        let end = target.iter().position(|&unit| unit == 0).unwrap_or(target.len());
        Some(String::from_utf16_lossy(&target[..end]))
    }
}

/// Public wrapper for UI labeling (Session Manager vs Temporary).
pub fn query_dos_device_public(letter: char) -> Option<String> {
    query_dos_device(letter)
}

fn delete_volume_mount_point(mount_root: &str) -> Result<(), String> {
    let wide = to_wide(mount_root);
    let ok = unsafe {
        windows_sys::Win32::Storage::FileSystem::DeleteVolumeMountPointW(wide.as_ptr())
    };
    if ok == 0 {
        let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
        Err(format!("DeleteVolumeMountPoint failed ({code})"))
    } else {
        Ok(())
    }
}

pub fn is_ext_family(filesystem: &str) -> bool {
    let upper = filesystem.to_ascii_uppercase();
    upper.starts_with("EXT2") || upper.starts_with("EXT3") || upper.starts_with("EXT4")
}

#[derive(Debug, Clone, Default)]
pub struct AutomountReport {
    pub mounted: Vec<String>,
    pub already_mounted: usize,
    pub errors: Vec<String>,
}

impl AutomountReport {
    pub fn summary(&self) -> Option<String> {
        if self.mounted.is_empty() && self.errors.is_empty() {
            return None;
        }
        let mut parts = Vec::new();
        if !self.mounted.is_empty() {
            parts.push(format!(
                "Auto-mounted {} volume(s): {}",
                self.mounted.len(),
                self.mounted.join("; ")
            ));
        }
        if self.already_mounted > 0 {
            parts.push(format!(
                "{} already mounted (skipped)",
                self.already_mounted
            ));
        }
        if !self.errors.is_empty() {
            parts.push(format!("Auto-mount errors: {}", self.errors.join("; ")));
        }
        Some(parts.join(". "))
    }

    pub fn did_mount(&self) -> bool {
        !self.mounted.is_empty()
    }
}

/// Ext2Mgr `Ext2ProcessExt2Volumes` (+ dormant Session Manager re-bind).
///
/// Priority when nothing is live yet:
/// 1. Session Manager DOS Devices (user's permanent registry choice, e.g. P:)
/// 2. Ext2Fsd `Volumes\{UUID}` MountPoint=
/// 3. Driver `EVP.DrvLetter` (classic Ext2ProcessExt2Property)
///
/// EVP used to run first and `continue`, so a stale H: overrode a just-written
/// Session Manager P: after Change — Explorer then only saw H: on upstream refresh.
pub fn process_pending_automounts(
    volumes: &[crate::disk::enum_disk::VolumeEntry],
) -> AutomountReport {
    let mut report = AutomountReport::default();
    let free = free_drive_letters();

    for volume in volumes {
        let device = dos_device_target(&volume.physical_object, &volume.symlink);
        let already_live = !volume.letters.is_empty()
            || (!volume.win32_volume_name.is_empty()
                && !crate::mount::dead_letters::mountmgr_letters_for_volume(
                    &volume.win32_volume_name,
                )
                .is_empty())
            || (!device.is_empty() && device_has_live_dos_letter(&device));

        if already_live {
            report.already_mounted += 1;
            continue;
        }

        // 1) Session Manager registry letter(s) for this NT device.
        if !device.is_empty() {
            let session_letters = super::persist::registry_letters_for_device(&device);
            if !session_letters.is_empty() {
                for letter in session_letters {
                    if query_dos_device(letter).is_some() {
                        report.already_mounted += 1;
                        continue;
                    }
                    match define_temporary(letter, &device) {
                        Ok(()) => report
                            .mounted
                            .push(format!("{letter}: (Session Manager → {device})")),
                        Err(err) => report.errors.push(err),
                    }
                }
                continue;
            }
        }

        if !is_ext_family(&volume.filesystem) {
            continue;
        }

        let symlink = device.clone();
        if symlink.is_empty() {
            continue;
        }

        // 2) Ext2Fsd Volumes\{UUID} MountPoint=X:
        if let Some(uuid) = volume.uuid {
            if let Some(preferred) = super::persist::query_ext2_automount_letter(&uuid) {
                let letter = pick_automount_letter(preferred, &free, &mut report, &format!(
                    "{} automount",
                    super::persist::format_volume_uuid(&uuid)
                ));
                if let Some(letter) = letter {
                    match define_temporary(letter, &symlink) {
                        Ok(()) => report.mounted.push(format!(
                            "{letter}: (Ext2Fsd automount → {symlink})"
                        )),
                        Err(err) => report.errors.push(err),
                    }
                }
                continue;
            }
        }

        // 3) Driver EVP.DrvLetter (classic Ext2ProcessExt2Property).
        if let Some(preferred) = crate::disk::enum_disk::query_volume_fixed_drv_letter(volume) {
            let letter = pick_automount_letter(
                preferred,
                &free,
                &mut report,
                &format!("EVP.DrvLetter={preferred}"),
            );
            if let Some(letter) = letter {
                match define_temporary(letter, &symlink) {
                    Ok(()) => report.mounted.push(format!(
                        "{letter}: (Ext2Fsd EVP.DrvLetter → {symlink})"
                    )),
                    Err(err) => report.errors.push(err),
                }
            }
        }
    }

    report
}

fn device_has_live_dos_letter(device_nt_path: &str) -> bool {
    for index in 0u8..26 {
        let letter = char::from(b'A' + index);
        if query_dos_device(letter)
            .is_some_and(|target| target.eq_ignore_ascii_case(device_nt_path))
        {
            return true;
        }
    }
    false
}

fn pick_automount_letter(
    preferred: char,
    free: &[char],
    report: &mut AutomountReport,
    context: &str,
) -> Option<char> {
    if free.contains(&preferred) && query_dos_device(preferred).is_none() {
        return Some(preferred);
    }
    if let Some(fallback) = free
        .iter()
        .copied()
        .find(|candidate| query_dos_device(*candidate).is_none())
    {
        return Some(fallback);
    }
    report
        .errors
        .push(format!("{context}: no free letter"));
    None
}

/// Ext2Mgr OnFlush → Ext2FlushVolume: FlushFileBuffers on the volume handle.
pub fn flush_volume(
    letters: &[char],
    win32_volume_name: &str,
    physical_object: &str,
) -> Result<(), String> {
    use std::os::windows::ffi::OsStrExt;
    const GENERIC_READ: u32 = 0x8000_0000;
    const GENERIC_WRITE: u32 = 0x4000_0000;

    let mut paths = Vec::new();
    if let Some(letter) = letters.first() {
        paths.push(format!(r"\\.\{letter}:"));
    }
    let trimmed = win32_volume_name.trim_end_matches('\\');
    if !trimmed.is_empty() {
        paths.push(trimmed.to_string());
    }
    if !physical_object.is_empty() {
        let rest = physical_object.trim_start_matches('\\');
        paths.push(format!(r"\\?\GLOBALROOT\{rest}"));
    }
    if paths.is_empty() {
        return Err("No path to flush".to_string());
    }

    let mut last_error = String::from("Flush failed");
    for path in paths {
        let wide: Vec<u16> = std::ffi::OsStr::new(&path)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect();
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                GENERIC_READ | GENERIC_WRITE,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
            last_error = format!("Open {path} failed ({code})");
            continue;
        }
        let ok = unsafe { windows_sys::Win32::Storage::FileSystem::FlushFileBuffers(handle) };
        let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        if ok != 0 {
            return Ok(());
        }
        last_error = format!("FlushFileBuffers({path}) failed ({code})");
    }
    Err(last_error)
}
