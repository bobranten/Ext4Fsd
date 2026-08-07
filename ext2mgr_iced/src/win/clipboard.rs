//! Win32 clipboard — CF_UNICODETEXT via OpenClipboard.


use std::os::windows::ffi::OsStrExt;

pub fn set_text(text: &str) -> Result<(), String> {
    let wide: Vec<u16> = std::ffi::OsStr::new(text)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();
    let bytes = wide.len() * 2;

    let opened = unsafe { windows_sys::Win32::System::DataExchange::OpenClipboard(0) };
    if opened == 0 {
        return Err(format!(
            "OpenClipboard failed ({})",
            unsafe { windows_sys::Win32::Foundation::GetLastError() }
        ));
    }
    unsafe {
        windows_sys::Win32::System::DataExchange::EmptyClipboard();
    }

    let handle = unsafe {
        windows_sys::Win32::System::Memory::GlobalAlloc(
            windows_sys::Win32::System::Memory::GMEM_MOVEABLE,
            bytes,
        )
    };
    if handle.is_null() {
        unsafe {
            windows_sys::Win32::System::DataExchange::CloseClipboard();
        }
        return Err("GlobalAlloc failed".to_string());
    }
    let ptr = unsafe { windows_sys::Win32::System::Memory::GlobalLock(handle) };
    if ptr.is_null() {
        unsafe {
            windows_sys::Win32::Foundation::GlobalFree(handle);
            windows_sys::Win32::System::DataExchange::CloseClipboard();
        }
        return Err("GlobalLock failed".to_string());
    }
    unsafe {
        std::ptr::copy_nonoverlapping(wide.as_ptr() as *const u8, ptr as *mut u8, bytes);
        windows_sys::Win32::System::Memory::GlobalUnlock(handle);
        // CF_UNICODETEXT = 13
        windows_sys::Win32::System::DataExchange::SetClipboardData(13, handle as isize);
        windows_sys::Win32::System::DataExchange::CloseClipboard();
    }
    Ok(())
}
