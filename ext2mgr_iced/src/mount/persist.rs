//! Persistent mount registry helpers (Session Manager DOS Devices + Ext2Fsd Volumes).


use std::os::windows::ffi::OsStrExt;

const DOS_DEVICES_KEY: &str =
    r"SYSTEM\CurrentControlSet\Control\Session Manager\DOS Devices";
const VOLUMES_KEY: &str = r"SYSTEM\CurrentControlSet\Services\Ext2Fsd\Volumes";

/// Format Ext2Mgr-style UUID value name: `{AA-BB-...-FF}`.
pub fn format_volume_uuid(uuid: &[u8; 16]) -> String {
    let mut out = String::from("{");
    for (index, byte) in uuid.iter().enumerate() {
        if index == 0 {
            out.push_str(&format!("{byte:02X}"));
        } else if index == 15 {
            out.push_str(&format!("-{byte:02X}}}"));
        } else {
            out.push_str(&format!("-{byte:02X}"));
        }
    }
    out
}

/// Persist letter in Session Manager DOS Devices (survives reboot; may need reboot
/// to appear - Ext2Mgr calls this "fixed mount").
pub fn set_registry_mount_point(letter: char, device_nt_path: &str) -> Result<(), String> {
    let key = open_key(DOS_DEVICES_KEY, true)?;
    let value_name = format!("{letter}:");
    let result = set_string(key, &value_name, device_nt_path);
    close_key(key);
    result
}

pub fn clear_registry_mount_point(letter: char) -> Result<(), String> {
    let key = open_key(DOS_DEVICES_KEY, true)?;
    let value_name = format!("{letter}:");
    let wide = to_wide(&value_name);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegDeleteValueW(key, wide.as_ptr())
    };
    close_key(key);
    // ERROR_FILE_NOT_FOUND = 2 → already gone
    if status == 0 || status == 2 {
        Ok(())
    } else {
        Err(format!("RegDeleteValue({value_name}) failed ({status})"))
    }
}

/// Drop Session Manager letters that target `\Device\HarddiskN\PartitionM`
/// (invalid Ext2Fsd mount targets left by a prior iced bug). Real mounts use
/// `\Device\HarddiskVolumeN`.
pub fn scrub_partition_path_session_letters() -> Vec<char> {
    let mut removed = Vec::new();
    for (letter, device) in registry_dos_device_entries() {
        let upper = device.to_ascii_uppercase();
        if upper.contains(r"\PARTITION") && !upper.contains(r"\HARDDISKVOLUME") {
            if clear_registry_mount_point(letter).is_ok() {
                removed.push(letter);
            }
        }
    }
    removed
}

/// Drive-letter Session Manager DOS Devices entries (`X:` → `\Device\…`).
pub fn registry_dos_device_entries() -> Vec<(char, String)> {
    let Ok(key) = open_key(DOS_DEVICES_KEY, false) else {
        return Vec::new();
    };
    let mut index = 0u32;
    let mut entries = Vec::new();
    loop {
        let mut name = vec![0u16; 64];
        let mut name_len = name.len() as u32;
        let mut value_type = 0u32;
        let mut data = vec![0u8; 512];
        let mut data_len = data.len() as u32;
        let status = unsafe {
            windows_sys::Win32::System::Registry::RegEnumValueW(
                key,
                index,
                name.as_mut_ptr(),
                &mut name_len,
                std::ptr::null_mut(),
                &mut value_type,
                data.as_mut_ptr(),
                &mut data_len,
            )
        };
        if status != 0 {
            break;
        }
        index += 1;
        let name_string = wide_to_string(&name[..name_len as usize]);
        let Some(letter) = name_string
            .chars()
            .next()
            .filter(|ch| ch.is_ascii_alphabetic())
        else {
            continue;
        };
        if !name_string.ends_with(':') {
            continue;
        }
        let data_units: Vec<u16> = data[..data_len as usize]
            .chunks_exact(2)
            .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
            .take_while(|unit| *unit != 0)
            .collect();
        let device = String::from_utf16_lossy(&data_units);
        if device.is_empty() {
            continue;
        }
        entries.push((letter.to_ascii_uppercase(), device));
    }
    close_key(key);
    entries.sort_by(|left, right| left.0.cmp(&right.0));
    entries.dedup_by(|left, right| left.0 == right.0);
    entries
}

/// Target device for a Session Manager letter (`X:` → `\Device\…`), if any.
pub fn registry_device_for_letter(letter: char) -> Option<String> {
    let want = letter.to_ascii_uppercase();
    registry_dos_device_entries()
        .into_iter()
        .find(|(entry, _)| *entry == want)
        .map(|(_, device)| device)
}

/// All Session Manager DOS Devices letters that map to `device_nt_path`.
pub fn registry_letters_for_device(device_nt_path: &str) -> Vec<char> {
    let mut letters: Vec<char> = registry_dos_device_entries()
        .into_iter()
        .filter(|(_, device)| device.eq_ignore_ascii_case(device_nt_path))
        .map(|(letter, _)| letter)
        .collect();
    letters.sort_unstable();
    letters.dedup();
    letters
}

pub fn query_registry_letter_for_device(device_nt_path: &str) -> Option<char> {
    registry_letters_for_device(device_nt_path).into_iter().next()
}

/// Store Ext2Fsd per-volume automount blob under Volumes\{UUID}.
pub fn store_ext2_automount(
    uuid: &[u8; 16],
    letter: char,
    codepage: &str,
    readonly: bool,
) -> Result<(), String> {
    let key = create_key(VOLUMES_KEY)?;
    let value_name = format_volume_uuid(uuid);
    let mut data = String::new();
    if readonly {
        data.push_str("Readonly;");
    }
    data.push_str(&format!("MountPoint={letter}:;"));
    if !codepage.is_empty() {
        data.push_str(&format!("CodePage={codepage};"));
    }
    let result = set_string(key, &value_name, &data);
    close_key(key);
    result
}

/// Remove Ext2Fsd `Volumes\{UUID}` so refresh/automount does not immediately
/// rebind a letter the user just removed from Mount Points.
pub fn clear_ext2_automount_mount_point(uuid: &[u8; 16]) -> Result<(), String> {
    let key = open_key(VOLUMES_KEY, true)?;
    let value_name = format_volume_uuid(uuid);
    let wide = to_wide(&value_name);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegDeleteValueW(key, wide.as_ptr())
    };
    close_key(key);
    if status == 0 || status == 2 {
        Ok(())
    } else {
        Err(format!("RegDeleteValue({value_name}) failed ({status})"))
    }
}

/// Preferred drive letter from Ext2Fsd `Volumes\{UUID}` (`MountPoint=X:`), if any.
pub fn query_ext2_automount_letter(uuid: &[u8; 16]) -> Option<char> {
    let key = open_key(VOLUMES_KEY, false).ok()?;
    let value_name = format_volume_uuid(uuid);
    let wide_name = to_wide(&value_name);
    let mut value_type = 0u32;
    let mut data = vec![0u8; 512];
    let mut data_len = data.len() as u32;
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegQueryValueExW(
            key,
            wide_name.as_ptr(),
            std::ptr::null_mut(),
            &mut value_type,
            data.as_mut_ptr(),
            &mut data_len,
        )
    };
    close_key(key);
    if status != 0 || data_len < 2 {
        return None;
    }
    let data_units: Vec<u16> = data[..data_len as usize]
        .chunks_exact(2)
        .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
        .take_while(|unit| *unit != 0)
        .collect();
    let blob = String::from_utf16_lossy(&data_units);
    for part in blob.split(';') {
        let part = part.trim();
        let Some(value) = part
            .strip_prefix("MountPoint=")
            .or_else(|| part.strip_prefix("mountpoint="))
        else {
            continue;
        };
        let value = value.trim();
        let mut chars = value.chars();
        if let (Some(letter), Some(':')) = (chars.next(), chars.next()) {
            if letter.is_ascii_alphabetic() {
                return Some(letter.to_ascii_uppercase());
            }
        }
    }
    None
}

type Hkey = windows_sys::Win32::System::Registry::HKEY;

fn open_key(path: &str, write: bool) -> Result<Hkey, String> {
    let access = if write {
        windows_sys::Win32::System::Registry::KEY_ALL_ACCESS
    } else {
        windows_sys::Win32::System::Registry::KEY_READ
    };
    let mut key = 0;
    let wide = to_wide(path);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegOpenKeyExW(
            windows_sys::Win32::System::Registry::HKEY_LOCAL_MACHINE,
            wide.as_ptr(),
            0,
            access,
            &mut key,
        )
    };
    if status != 0 {
        Err(format!("RegOpenKeyEx({path}) failed ({status}) - run elevated?"))
    } else {
        Ok(key)
    }
}

fn create_key(path: &str) -> Result<Hkey, String> {
    let mut key = 0;
    let mut disposition = 0u32;
    let wide = to_wide(path);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegCreateKeyExW(
            windows_sys::Win32::System::Registry::HKEY_LOCAL_MACHINE,
            wide.as_ptr(),
            0,
            std::ptr::null(),
            0,
            windows_sys::Win32::System::Registry::KEY_ALL_ACCESS,
            std::ptr::null(),
            &mut key,
            &mut disposition,
        )
    };
    if status != 0 {
        Err(format!("RegCreateKeyEx({path}) failed ({status}) - run elevated?"))
    } else {
        Ok(key)
    }
}

fn close_key(key: Hkey) {
    unsafe {
        windows_sys::Win32::System::Registry::RegCloseKey(key);
    }
}

fn set_string(key: Hkey, name: &str, value: &str) -> Result<(), String> {
    let wide_name = to_wide(name);
    let wide_value = to_wide(value);
    let bytes = unsafe {
        std::slice::from_raw_parts(wide_value.as_ptr() as *const u8, wide_value.len() * 2)
    };
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegSetValueExW(
            key,
            wide_name.as_ptr(),
            0,
            windows_sys::Win32::System::Registry::REG_SZ,
            bytes.as_ptr(),
            bytes.len() as u32,
        )
    };
    if status == 0 {
        Ok(())
    } else {
        Err(format!("RegSetValueEx({name}) failed ({status})"))
    }
}

fn to_wide(value: &str) -> Vec<u16> {
    std::ffi::OsStr::new(value)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect()
}

fn wide_to_string(wide: &[u16]) -> String {
    let end = wide.iter().position(|&unit| unit == 0).unwrap_or(wide.len());
    String::from_utf16_lossy(&wide[..end])
}
