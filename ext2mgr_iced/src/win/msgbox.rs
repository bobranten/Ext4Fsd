//! Win32 MessageBox — same user-visible error pattern as Ext2Mgr's AfxMessageBox.


use windows_sys::Win32::Foundation::HWND;
use windows_sys::Win32::UI::WindowsAndMessaging::{
    MessageBoxW, SetForegroundWindow, IDYES, MB_ICONQUESTION, MB_ICONSTOP, MB_ICONWARNING, MB_OK,
    MB_SETFOREGROUND, MB_TASKMODAL, MB_YESNO,
};

fn to_wide(text: &str) -> Vec<u16> {
    text.encode_utf16().chain(std::iter::once(0)).collect()
}

fn owner() -> HWND {
    super::chrome::find_main_hwnd().unwrap_or(0)
}

fn show(text: &str, caption: &str, flags: u32) -> i32 {
    let hwnd = owner();
    if hwnd != 0 {
        unsafe {
            SetForegroundWindow(hwnd);
        }
    }
    let text_w = to_wide(text);
    let caption_w = to_wide(caption);
    // TASKMODAL + SETFOREGROUND: avoid the box appearing behind the iced window
    // (which makes the main UI look "frozen" until the user finds the MessageBox).
    unsafe {
        MessageBoxW(
            hwnd,
            text_w.as_ptr(),
            caption_w.as_ptr(),
            flags | MB_TASKMODAL | MB_SETFOREGROUND,
        )
    }
}

/// Hard failure (MB_ICONSTOP) — mount when driver/service missing, etc.
pub fn error(text: &str) {
    let _ = show(text, "Ext2 Volume Manager", MB_OK | MB_ICONSTOP);
}

/// Soft / recoverable failure (MB_ICONWARNING).
pub fn warning(text: &str) {
    let _ = show(text, "Ext2 Volume Manager", MB_OK | MB_ICONWARNING);
}

/// Yes/No question; returns true on Yes.
pub fn confirm(text: &str) -> bool {
    show(text, "Ext2 Volume Manager", MB_YESNO | MB_ICONQUESTION) == IDYES
}
