//! Ext2Fsd service / global Parameters registry (Service Management).


use std::os::windows::ffi::OsStrExt;

#[allow(dead_code)]
pub const CODEPAGES: &[&str] = &[
    "default", "utf8", "cp936", "gb2312", "cp1251", "cp437", "cp850", "cp852",
    "cp866", "cp932", "cp949", "cp950", "big5", "euc-jp", "euc-kr", "iso8859-1",
    "iso8859-15", "koi8-r",
];

pub const START_MODE_LABELS: &[&str] = &[
    "SERVICE_BOOT_START",
    "SERVICE_SYSTEM_START",
    "SERVICE_AUTO_START",
    "SERVICE_DEMAND_START",
    "SERVICE_DISABLED",
];

#[derive(Debug, Clone)]
pub struct GlobalProperty {
    pub startup: u32,
    pub readonly: bool,
    pub ext3_writable: bool,
    pub automount: bool,
    pub codepage: String,
    pub hiding_prefix: String,
    pub hiding_suffix: String,
}

impl Default for GlobalProperty {
    fn default() -> Self {
        Self {
            startup: 3, // demand start
            readonly: true,
            ext3_writable: false,
            automount: false,
            codepage: "utf8".to_string(),
            hiding_prefix: String::new(),
            hiding_suffix: String::new(),
        }
    }
}

pub fn is_driver_started() -> bool {
    open_ext2fsd().is_some()
}

fn open_ext2fsd() -> Option<windows_sys::Win32::Foundation::HANDLE> {
    let wide = to_wide(r"\\.\Ext2Fsd");
    let handle = unsafe {
        windows_sys::Win32::Storage::FileSystem::CreateFileW(
            wide.as_ptr(),
            windows_sys::Win32::Storage::FileSystem::FILE_GENERIC_READ
                | windows_sys::Win32::Storage::FileSystem::FILE_GENERIC_WRITE,
            windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
            std::ptr::null(),
            windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
            0,
            0,
        )
    };
    if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
        None
    } else {
        Some(handle)
    }
}

/// Ext2QueryDrvVersion — About dialog driver line.
pub fn query_driver_version() -> Result<(String, String, String), String> {
    const IOCTL_APP_VOLUME_PROPERTY: u32 = 0x0022_1F40;
    const EXT2_VOLUME_PROPERTY_MAGIC: u32 = 0x4556_504D;
    const APP_CMD_QUERY_VERSION: u32 = 0;
    const EXT2_FLAG_VP_SET_GLOBAL: u32 = 1;
    // EXT2_VOLUME_PROPERTY_VERSION layout
    const SIZE: usize = 4 + 4 + 4 + 0x1C + 0x20 + 0x20; // 108

    let handle = open_ext2fsd().ok_or_else(|| "Ext2Fsd: NOT started".to_string())?;
    let mut buffer = vec![0u8; SIZE];
    buffer[0..4].copy_from_slice(&EXT2_VOLUME_PROPERTY_MAGIC.to_le_bytes());
    buffer[4..8].copy_from_slice(&EXT2_FLAG_VP_SET_GLOBAL.to_le_bytes());
    buffer[8..12].copy_from_slice(&APP_CMD_QUERY_VERSION.to_le_bytes());

    let mut bytes_returned = 0u32;
    let ok = unsafe {
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
    if ok == 0 {
        return Err(format!(
            "Query version failed ({})",
            unsafe { windows_sys::Win32::Foundation::GetLastError() }
        ));
    }
    let version = c_string_field(&buffer[12..12 + 0x1C]);
    let time = c_string_field(&buffer[12 + 0x1C..12 + 0x1C + 0x20]);
    let date = c_string_field(&buffer[12 + 0x1C + 0x20..12 + 0x1C + 0x40]);
    Ok((version, date, time))
}

/// Ext2Fsd IRP statistics for the PerfStat dialog.
pub fn query_perfstat_rows() -> Result<Vec<(String, u32, u32)>, String> {
    const IOCTL_APP_QUERY_PERFSTAT: u32 = 0x0022_1F44;
    const EXT2_QUERY_PERFSTAT_MAGIC: u32 = 0x4556_504D;
    // IRP_MJ_MAXIMUM_FUNCTION = 0x1B → 28 slots * 8 = 224; 4 * 64 = 256; header 12 → 492
    const SZ_V1: usize = 492;
    const IRP_COUNT: usize = 28;

    let handle = open_ext2fsd().ok_or_else(|| "Ext2Fsd: NOT started".to_string())?;
    let mut buffer = vec![0u8; SZ_V1];
    buffer[0..4].copy_from_slice(&EXT2_QUERY_PERFSTAT_MAGIC.to_le_bytes());
    buffer[8..12].copy_from_slice(&IOCTL_APP_QUERY_PERFSTAT.to_le_bytes());

    let mut bytes_returned = 0u32;
    let ok = unsafe {
        windows_sys::Win32::System::IO::DeviceIoControl(
            handle,
            IOCTL_APP_QUERY_PERFSTAT,
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
    if ok == 0 {
        return Err(format!(
            "Query PerfStat failed ({})",
            unsafe { windows_sys::Win32::Foundation::GetLastError() }
        ));
    }

    let irp_names = [
        "CREATE", "CREATE_NAMED_PIPE", "CLOSE", "READ", "WRITE", "QUERY_INFO",
        "SET_INFO", "QUERY_EA", "SET_EA", "FLUSH", "QUERY_VOLUME", "SET_VOLUME",
        "DIRECTORY_CONTROL", "FILE_SYSTEM_CONTROL", "DEVICE_CONTROL",
        "INTERNAL_DEVICE_CONTROL", "SHUTDOWN", "LOCK_CONTROL", "CLEANUP",
        "CREATE_MAILSLOT", "QUERY_SECURITY", "SET_SECURITY", "POWER",
        "SYSTEM_CONTROL", "DEVICE_CHANGE", "QUERY_QUOTA", "SET_QUOTA", "PNP",
    ];
    let mut rows = Vec::with_capacity(IRP_COUNT);
    for (index, name) in irp_names.iter().enumerate().take(IRP_COUNT) {
        let offset = 12 + index * 8;
        if offset + 8 > buffer.len() {
            break;
        }
        let processed = u32::from_le_bytes(buffer[offset..offset + 4].try_into().unwrap());
        let current = u32::from_le_bytes(buffer[offset + 4..offset + 8].try_into().unwrap());
        rows.push(((*name).to_string(), current, processed));
    }
    Ok(rows)
}

/// Compact Ext2Fsd statistics text retained for callers that need plain text.
pub fn query_perfstat_text() -> Result<String, String> {
    let rows = query_perfstat_rows()?;
    let mut output = String::from("Ext2Fsd IRP statistics\n\n");
    output.push_str("Name                          Current   Processed\n");
    output.push_str("------------------------------------------------\n");
    for (name, current, processed) in rows {
        output.push_str(&format!("{name:<28} {current:>8} {processed:>10}\n"));
    }
    Ok(output)
}

fn c_string_field(bytes: &[u8]) -> String {
    let end = bytes.iter().position(|&byte| byte == 0).unwrap_or(bytes.len());
    String::from_utf8_lossy(&bytes[..end]).trim().to_string()
}

pub fn start_driver() -> Result<(), String> {
    control_driver(true)
}

pub fn stop_driver() -> Result<(), String> {
    control_driver(false)
}

pub fn restart_driver() -> Result<(), String> {
    let _ = stop_driver();
    // Brief pause so the service can leave RUNNING.
    std::thread::sleep(std::time::Duration::from_millis(400));
    start_driver()
}

fn control_driver(start: bool) -> Result<(), String> {
    if start && is_driver_started() {
        return Ok(());
    }
    if !start && !is_driver_started() {
        return Ok(());
    }
    unsafe {
        let manager = windows_sys::Win32::System::Services::OpenSCManagerW(
            std::ptr::null(),
            std::ptr::null(),
            windows_sys::Win32::System::Services::SC_MANAGER_CONNECT,
        );
        if manager == 0 {
            return Err(format!(
                "OpenSCManager failed ({})",
                windows_sys::Win32::Foundation::GetLastError()
            ));
        }
        let service_name = to_wide("ext2fsd");
        let access = if start {
            windows_sys::Win32::System::Services::SERVICE_START
                | windows_sys::Win32::System::Services::SERVICE_QUERY_STATUS
        } else {
            windows_sys::Win32::System::Services::SERVICE_STOP
                | windows_sys::Win32::System::Services::SERVICE_QUERY_STATUS
        };
        let service = windows_sys::Win32::System::Services::OpenServiceW(
            manager,
            service_name.as_ptr(),
            access,
        );
        if service == 0 {
            let code = windows_sys::Win32::Foundation::GetLastError();
            windows_sys::Win32::System::Services::CloseServiceHandle(manager);
            return Err(format!("OpenService(ext2fsd) failed ({code})"));
        }
        let result = if start {
            let ok = windows_sys::Win32::System::Services::StartServiceW(
                service,
                0,
                std::ptr::null(),
            );
            let code = windows_sys::Win32::Foundation::GetLastError();
            // ERROR_SERVICE_ALREADY_RUNNING = 1056
            if ok == 0 && code != 1056 {
                Err(format!("StartService failed ({code})"))
            } else {
                Ok(())
            }
        } else {
            let mut status = std::mem::zeroed::<windows_sys::Win32::System::Services::SERVICE_STATUS>();
            let ok = windows_sys::Win32::System::Services::ControlService(
                service,
                windows_sys::Win32::System::Services::SERVICE_CONTROL_STOP,
                &mut status,
            );
            let code = windows_sys::Win32::Foundation::GetLastError();
            // ERROR_SERVICE_NOT_ACTIVE = 1062
            if ok == 0 && code != 1062 {
                Err(format!("ControlService(STOP) failed ({code})"))
            } else {
                Ok(())
            }
        };
        windows_sys::Win32::System::Services::CloseServiceHandle(service);
        windows_sys::Win32::System::Services::CloseServiceHandle(manager);
        result?;
    }
    if start {
        if is_driver_started() {
            Ok(())
        } else {
            Err("Driver start requested but \\\\.\\Ext2Fsd is still unavailable".to_string())
        }
    } else {
        Ok(())
    }
}

pub fn query_global_property() -> Result<GlobalProperty, String> {
    let mut props = GlobalProperty::default();
    let service_key = open_key(
        r"SYSTEM\CurrentControlSet\Services\Ext2Fsd",
        windows_sys::Win32::System::Registry::KEY_READ,
    )?;
    if let Some(value) = query_dword(service_key, "Start") {
        props.startup = value;
    }
    close_key(service_key);

    let params_key = open_key(
        r"SYSTEM\CurrentControlSet\Services\Ext2Fsd\Parameters",
        windows_sys::Win32::System::Registry::KEY_READ,
    )?;
    if let Some(value) = query_dword(params_key, "WritingSupport") {
        props.readonly = value == 0;
    }
    if let Some(value) = query_dword(params_key, "Ext3ForceWriting") {
        props.ext3_writable = value != 0;
    }
    if let Some(value) = query_dword(params_key, "AutoMount") {
        props.automount = value != 0;
    }
    if let Some(value) = query_string(params_key, "CodePage") {
        props.codepage = value;
    }
    if let Some(value) = query_string(params_key, "HidingPrefix") {
        props.hiding_prefix = value;
    }
    if let Some(value) = query_string(params_key, "HidingSuffix") {
        props.hiding_suffix = value;
    }
    close_key(params_key);
    Ok(props)
}

pub fn set_global_property(props: &GlobalProperty) -> Result<(), String> {
    let service_key = open_key(
        r"SYSTEM\CurrentControlSet\Services\Ext2Fsd",
        windows_sys::Win32::System::Registry::KEY_SET_VALUE,
    )?;
    set_dword(service_key, "Start", props.startup)?;
    close_key(service_key);

    let params_key = open_key(
        r"SYSTEM\CurrentControlSet\Services\Ext2Fsd\Parameters",
        windows_sys::Win32::System::Registry::KEY_SET_VALUE,
    )?;
    set_dword(params_key, "WritingSupport", u32::from(!props.readonly))?;
    set_dword(
        params_key,
        "Ext3ForceWriting",
        u32::from(props.ext3_writable && !props.readonly),
    )?;
    set_dword(params_key, "AutoMount", u32::from(props.automount))?;
    set_string(params_key, "CodePage", &props.codepage)?;
    set_string(params_key, "HidingPrefix", &props.hiding_prefix)?;
    set_string(params_key, "HidingSuffix", &props.hiding_suffix)?;
    close_key(params_key);
    Ok(())
}

type Hkey = windows_sys::Win32::System::Registry::HKEY;

fn open_key(path: &str, access: u32) -> Result<Hkey, String> {
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

fn close_key(key: Hkey) {
    unsafe {
        windows_sys::Win32::System::Registry::RegCloseKey(key);
    }
}

fn query_dword(key: Hkey, name: &str) -> Option<u32> {
    let wide = to_wide(name);
    let mut data = 0u32;
    let mut data_size = std::mem::size_of::<u32>() as u32;
    let mut value_type = 0u32;
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegQueryValueExW(
            key,
            wide.as_ptr(),
            std::ptr::null_mut(),
            &mut value_type,
            (&mut data as *mut u32).cast(),
            &mut data_size,
        )
    };
    if status == 0 {
        Some(data)
    } else {
        None
    }
}

fn query_string(key: Hkey, name: &str) -> Option<String> {
    let wide = to_wide(name);
    let mut data_size = 0u32;
    let mut value_type = 0u32;
    unsafe {
        windows_sys::Win32::System::Registry::RegQueryValueExW(
            key,
            wide.as_ptr(),
            std::ptr::null_mut(),
            &mut value_type,
            std::ptr::null_mut(),
            &mut data_size,
        );
    }
    if data_size == 0 {
        return None;
    }
    let mut buffer = vec![0u8; data_size as usize];
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegQueryValueExW(
            key,
            wide.as_ptr(),
            std::ptr::null_mut(),
            &mut value_type,
            buffer.as_mut_ptr(),
            &mut data_size,
        )
    };
    if status != 0 {
        return None;
    }
    let units: Vec<u16> = buffer
        .chunks_exact(2)
        .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
        .take_while(|unit| *unit != 0)
        .collect();
    Some(String::from_utf16_lossy(&units))
}

fn set_dword(key: Hkey, name: &str, value: u32) -> Result<(), String> {
    let wide = to_wide(name);
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegSetValueExW(
            key,
            wide.as_ptr(),
            0,
            windows_sys::Win32::System::Registry::REG_DWORD,
            (&value as *const u32).cast(),
            std::mem::size_of::<u32>() as u32,
        )
    };
    if status == 0 {
        Ok(())
    } else {
        Err(format!("RegSetValueEx({name}) failed ({status})"))
    }
}

fn set_string(key: Hkey, name: &str, value: &str) -> Result<(), String> {
    let wide_name = to_wide(name);
    let wide_value = to_wide(value);
    let bytes = unsafe {
        std::slice::from_raw_parts(
            wide_value.as_ptr() as *const u8,
            wide_value.len() * 2,
        )
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
