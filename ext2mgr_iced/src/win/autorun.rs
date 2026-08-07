//! File / Tools menu: autostart this iced manager at logon (HKCU Run).


use std::os::windows::ffi::OsStrExt;

const RUN_KEY: &str = r"Software\Microsoft\Windows\CurrentVersion\Run";
const VALUE_NAME: &str = "Ext2MgrIced";

pub fn is_autorun_enabled() -> bool {
    reg_query_run_value().is_some()
}

pub fn set_autorun(enabled: bool) -> Result<(), String> {
    if enabled {
        let exe = std::env::current_exe()
            .map_err(|err| format!("current_exe: {err}"))?;
        let path = exe.to_string_lossy();
        // Quote path for Run key (spaces).
        let value = format!("\"{path}\"");
        reg_set_run_value(&value)
    } else {
        reg_delete_run_value()
    }
}

fn reg_query_run_value() -> Option<String> {
    let key = open_run_key(false)?;
    let wide_name = to_wide(VALUE_NAME);
    let mut value_type = 0u32;
    let mut data = vec![0u8; 1024];
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
    let units: Vec<u16> = data[..data_len as usize]
        .chunks_exact(2)
        .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
        .take_while(|unit| *unit != 0)
        .collect();
    let text = String::from_utf16_lossy(&units);
    if text.is_empty() {
        None
    } else {
        Some(text)
    }
}

fn reg_set_run_value(value: &str) -> Result<(), String> {
    let key = open_run_key(true).ok_or_else(|| "Open Run key failed".to_string())?;
    let wide_name = to_wide(VALUE_NAME);
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
    close_key(key);
    if status == 0 {
        Ok(())
    } else {
        Err(format!("RegSetValueEx(Run) failed ({status})"))
    }
}

fn reg_delete_run_value() -> Result<(), String> {
    let key = open_run_key(true).ok_or_else(|| "Open Run key failed".to_string())?;
    let wide_name = to_wide(VALUE_NAME);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegDeleteValueW(key, wide_name.as_ptr())
    };
    close_key(key);
    // ERROR_FILE_NOT_FOUND = 2
    if status == 0 || status == 2 {
        Ok(())
    } else {
        Err(format!("RegDeleteValue(Run) failed ({status})"))
    }
}

fn open_run_key(write: bool) -> Option<windows_sys::Win32::System::Registry::HKEY> {
    let access = if write {
        windows_sys::Win32::System::Registry::KEY_SET_VALUE
            | windows_sys::Win32::System::Registry::KEY_QUERY_VALUE
    } else {
        windows_sys::Win32::System::Registry::KEY_READ
    };
    let wide = to_wide(RUN_KEY);
    let mut key = 0;
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegOpenKeyExW(
            windows_sys::Win32::System::Registry::HKEY_CURRENT_USER,
            wide.as_ptr(),
            0,
            access,
            &mut key,
        )
    };
    if status == 0 {
        Some(key)
    } else {
        None
    }
}

fn close_key(key: windows_sys::Win32::System::Registry::HKEY) {
    unsafe {
        windows_sys::Win32::System::Registry::RegCloseKey(key);
    }
}

fn to_wide(value: &str) -> Vec<u16> {
    std::ffi::OsStr::new(value)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect()
}
