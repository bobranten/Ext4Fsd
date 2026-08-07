//! Win32 chrome fallback: menu bar, tray, TrackPopupMenu, HWND helpers.
//! Matches Ext2Mgr IDR_MENU / IDR_TRAY / Shell_NotifyIcon / TrackPopupMenu.


use std::sync::Mutex;

use windows_sys::Win32::Foundation::{HWND, LPARAM, LRESULT, POINT, WPARAM};
use windows_sys::Win32::UI::Shell::{
    Shell_NotifyIconW, NIF_ICON, NIF_MESSAGE, NIF_TIP, NIM_ADD, NIM_DELETE, NOTIFYICONDATAW,
};
use windows_sys::Win32::UI::WindowsAndMessaging::{
    AppendMenuW, CheckMenuItem, CreateMenu, CreatePopupMenu, DefWindowProcW, DestroyMenu,
    DrawMenuBar, EnableMenuItem, FindWindowW, GetCursorPos, GetSystemMetrics, GetWindowLongPtrW,
    IsWindow, LoadImageW, PostMessageW, SetForegroundWindow, SetMenu, SetWindowLongPtrW,
    SetWindowPos, ShowWindow, TrackPopupMenu, GWLP_WNDPROC, HICON, HMENU, IMAGE_ICON,
    LR_DEFAULTSIZE, LR_LOADFROMFILE, MF_BYCOMMAND, MF_CHECKED, MF_ENABLED, MF_GRAYED, MF_POPUP,
    MF_SEPARATOR, MF_STRING, MF_UNCHECKED, SC_MINIMIZE, SM_CXSMICON, SM_CYSMICON, SWP_FRAMECHANGED,
    SWP_NOMOVE, SWP_NOSIZE, SWP_NOZORDER, SW_HIDE, SW_RESTORE, SW_SHOW, TPM_LEFTALIGN,
    TPM_RETURNCMD, TPM_RIGHTBUTTON, WM_CLOSE, WM_COMMAND, WM_DESTROY, WM_NULL, WM_SYSCOMMAND,
    WM_USER,
};

pub const ID_ABOUT: u32 = 32771;
pub const ID_CHANGE: u32 = 32773;
pub const ID_REFRESH: u32 = 32774;
pub const ID_EXIT: u32 = 32775;
pub const ID_SERVICE: u32 = 32776;
pub const ID_PROPERTY: u32 = 32778;
pub const ID_COPY: u32 = 32779;
pub const ID_SHOW_MAIN: u32 = 32780;
pub const ID_DRV_LETTER: u32 = 32781;
pub const ID_DONATE: u32 = 32782;
pub const ID_ENABLE_AUTOSTART: u32 = 32788;
pub const ID_DISABLE_AUTOSTART: u32 = 32789;
pub const ID_PERFSTAT: u32 = 32790;
pub const ID_COPYALL: u32 = 32792;
pub const ID_FLUSH_BUFFER: u32 = 32794;
pub const ID_CHANGE_PARTTYPE: u32 = 32795;
pub const ID_REMOVE_DEAD_LETTER: u32 = 32796;
pub const ID_DRV_QUICK_MOUNT: u32 = 32797;
/// Unmount / remove assigned drive letter(s) for the selection.
pub const ID_DRV_UNMOUNT: u32 = 32799;
/// Help → Documentation (opens FAQ); avoids redundant “Help → Help”.
pub const ID_DOCUMENTATION: u32 = 32800;
pub const ID_VIEW_SI_UNITS: u32 = 32810;
pub const ID_VIEW_BINARY_UNITS: u32 = 32811;
pub const ID_VIEW_DISPLAY_BYTES: u32 = 32812;
pub const ID_VIEW_DISPLAY_BITS: u32 = 32813;
/// View → show/hide the properties pane under Disks / Partitions.
pub const ID_VIEW_PROPERTIES_PANE: u32 = 32814;

const WM_TRAY_ICON_NOTIFY: u32 = WM_USER + 100;
const TRAY_UID: u32 = 0xE220_1001;

static PENDING: Mutex<Vec<u32>> = Mutex::new(Vec::new());
static STATE: Mutex<ChromeState> = Mutex::new(ChromeState::new());

struct ChromeState {
    hwnd: usize,
    old_wnd_proc: Option<isize>,
    tray_added: bool,
    menu: usize,
    icon: usize,
}

unsafe impl Send for ChromeState {}

impl ChromeState {
    const fn new() -> Self {
        Self {
            hwnd: 0,
            old_wnd_proc: None,
            tray_added: false,
            menu: 0,
            icon: 0,
        }
    }
}

fn to_wide(text: &str) -> Vec<u16> {
    text.encode_utf16().chain(std::iter::once(0)).collect()
}

pub fn window_title() -> &'static str {
    "Ext2 Volume Manager"
}

pub fn find_main_hwnd() -> Option<HWND> {
    let title = to_wide(window_title());
    let hwnd = unsafe { FindWindowW(std::ptr::null(), title.as_ptr()) };
    if hwnd == 0 || unsafe { IsWindow(hwnd) } == 0 {
        None
    } else {
        Some(hwnd)
    }
}

fn icon_path() -> Option<std::path::PathBuf> {
    let mut candidates = vec![
        std::path::PathBuf::from("assets/Ext2Mgr.ico"),
        std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("assets/Ext2Mgr.ico"),
    ];
    if let Ok(exe) = std::env::current_exe() {
        if let Some(parent) = exe.parent() {
            candidates.push(parent.join("assets/Ext2Mgr.ico"));
        }
    }
    candidates.into_iter().find(|path| path.is_file())
}

fn load_app_icon() -> HICON {
    let Some(path) = icon_path() else {
        return 0;
    };
    let wide = to_wide(&path.to_string_lossy());
    let cx = unsafe { GetSystemMetrics(SM_CXSMICON) };
    let cy = unsafe { GetSystemMetrics(SM_CYSMICON) };
    unsafe {
        LoadImageW(
            0,
            wide.as_ptr(),
            IMAGE_ICON,
            cx,
            cy,
            LR_LOADFROMFILE | LR_DEFAULTSIZE,
        ) as HICON
    }
}

fn append_string(menu: HMENU, id: u32, label: &str) {
    let wide = to_wide(label);
    unsafe {
        AppendMenuW(menu, MF_STRING, id as usize, wide.as_ptr());
    }
}

fn build_main_menu() -> HMENU {
    unsafe {
        let menu = CreateMenu();

        let file = CreatePopupMenu();
        append_string(file, ID_ENABLE_AUTOSTART, "&Enable Ext2Mgr autostart");
        append_string(file, ID_DISABLE_AUTOSTART, "&Disable Ext2Mgr autostart");
        AppendMenuW(file, MF_SEPARATOR, 0, std::ptr::null());
        append_string(file, ID_EXIT, "E&xit");
        let file_label = to_wide("&File");
        AppendMenuW(menu, MF_POPUP, file as usize, file_label.as_ptr());

        let edit = CreatePopupMenu();
        append_string(edit, ID_COPYALL, "&Copy Everything to Clipboard");
        let edit_label = to_wide("&Edit");
        AppendMenuW(menu, MF_POPUP, edit as usize, edit_label.as_ptr());

        let view = CreatePopupMenu();
        append_string(view, ID_VIEW_PROPERTIES_PANE, "Show &properties pane");
        AppendMenuW(view, MF_SEPARATOR, 0, std::ptr::null());
        append_string(view, ID_VIEW_SI_UNITS, "Use &SI units");
        append_string(view, ID_VIEW_BINARY_UNITS, "Use &binary units");
        AppendMenuW(view, MF_SEPARATOR, 0, std::ptr::null());
        append_string(view, ID_VIEW_DISPLAY_BYTES, "Display with b&ytes");
        append_string(view, ID_VIEW_DISPLAY_BITS, "Display with b&its");
        let view_label = to_wide("&View");
        AppendMenuW(menu, MF_POPUP, view as usize, view_label.as_ptr());

        let tools = CreatePopupMenu();
        append_string(tools, ID_REFRESH, "&Reload and Refresh\tF5");
        AppendMenuW(tools, MF_SEPARATOR, 0, std::ptr::null());
        append_string(tools, ID_SERVICE, "&Service Management\tF7");
        append_string(tools, ID_PERFSTAT, "Ext2Fsd S&tatistics\tF8");
        AppendMenuW(tools, MF_SEPARATOR, 0, std::ptr::null());
        append_string(tools, ID_REMOVE_DEAD_LETTER, "Remove &Dead Letters\tF9");
        AppendMenuW(tools, MF_SEPARATOR, 0, std::ptr::null());
        append_string(tools, ID_CHANGE, "&Ext2 Volume Management\tF3");
        append_string(tools, ID_DRV_LETTER, "&Mountpoint Management\tF10");
        AppendMenuW(tools, MF_SEPARATOR, 0, std::ptr::null());
        append_string(tools, ID_FLUSH_BUFFER, "&Flush Cache to Disk\tF11");
        append_string(tools, ID_CHANGE_PARTTYPE, "&Change Partition Type\tF12");
        AppendMenuW(tools, MF_SEPARATOR, 0, std::ptr::null());
        append_string(tools, ID_PROPERTY, "Show &Properties\tF6");
        append_string(tools, ID_DRV_QUICK_MOUNT, "Assign Drive Letter\tF4");
        append_string(tools, ID_DRV_UNMOUNT, "&Unmount");
        let tools_label = to_wide("&Tools");
        AppendMenuW(menu, MF_POPUP, tools as usize, tools_label.as_ptr());

        let help = CreatePopupMenu();
        append_string(help, ID_DOCUMENTATION, "&Documentation\tF1");
        AppendMenuW(help, MF_SEPARATOR, 0, std::ptr::null());
        append_string(help, ID_ABOUT, "&About\tF2");
        AppendMenuW(help, MF_SEPARATOR, 0, std::ptr::null());
        append_string(help, ID_DONATE, "&Donate");
        let help_label = to_wide("&Help");
        AppendMenuW(menu, MF_POPUP, help as usize, help_label.as_ptr());
        menu
    }
}

fn tray_add(hwnd: HWND, icon: HICON) -> bool {
    let mut data: NOTIFYICONDATAW = unsafe { std::mem::zeroed() };
    data.cbSize = std::mem::size_of::<NOTIFYICONDATAW>() as u32;
    data.hWnd = hwnd;
    data.uID = TRAY_UID;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = WM_TRAY_ICON_NOTIFY;
    data.hIcon = icon;
    let tip = "Ext2 Volume Manager";
    for (index, unit) in tip.encode_utf16().take(127).enumerate() {
        data.szTip[index] = unit;
    }
    unsafe { Shell_NotifyIconW(NIM_ADD, &data) != 0 }
}

fn tray_remove(hwnd: HWND) {
    let mut data: NOTIFYICONDATAW = unsafe { std::mem::zeroed() };
    data.cbSize = std::mem::size_of::<NOTIFYICONDATAW>() as u32;
    data.hWnd = hwnd;
    data.uID = TRAY_UID;
    unsafe {
        let _ = Shell_NotifyIconW(NIM_DELETE, &data);
    }
}

fn push_command(id: u32) {
    if let Ok(mut pending) = PENDING.lock() {
        pending.push(id);
    }
}

pub fn poll_commands() -> Vec<u32> {
    PENDING
        .lock()
        .map(|mut pending| pending.drain(..).collect())
        .unwrap_or_default()
}

type WndProc = unsafe extern "system" fn(HWND, u32, WPARAM, LPARAM) -> LRESULT;

unsafe extern "system" fn subclass_proc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    match msg {
        WM_COMMAND => {
            let id = (wparam as u32) & 0xFFFF;
            if id != 0 {
                push_command(id);
            }
            return 0;
        }
        WM_SYSCOMMAND => {
            let command = (wparam as u32) & 0xFFF0;
            if command == SC_MINIMIZE {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
        }
        // Backup if the shell minimizes without a clean SC_MINIMIZE path.
        0x0005 /* WM_SIZE */ => {
            if wparam == 1 /* SIZE_MINIMIZED */ {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
        }
        x if x == WM_TRAY_ICON_NOTIFY => {
            let mouse = (lparam as u32) & 0xFFFF;
            // WM_LBUTTONUP / WM_RBUTTONUP
            if mouse == 0x0202 {
                ShowWindow(hwnd, SW_SHOW);
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                return 0;
            }
            if mouse == 0x0205 {
                if let Some(id) = track_tray_menu(hwnd) {
                    if id == ID_SHOW_MAIN {
                        ShowWindow(hwnd, SW_SHOW);
                        ShowWindow(hwnd, SW_RESTORE);
                        SetForegroundWindow(hwnd);
                    } else if id == ID_EXIT {
                        PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    } else {
                        push_command(id);
                    }
                }
                return 0;
            }
        }
        WM_DESTROY => {
            if let Ok(mut state) = STATE.lock() {
                if state.tray_added {
                    tray_remove(hwnd);
                    state.tray_added = false;
                }
            }
        }
        _ => {}
    }

    let old = STATE
        .lock()
        .ok()
        .and_then(|state| state.old_wnd_proc)
        .unwrap_or(0);
    if old != 0 {
        let proc: WndProc = std::mem::transmute(old);
        proc(hwnd, msg, wparam, lparam)
    } else {
        DefWindowProcW(hwnd, msg, wparam, lparam)
    }
}

fn track_tray_menu(hwnd: HWND) -> Option<u32> {
    unsafe {
        let menu = CreatePopupMenu();
        append_string(menu, ID_SHOW_MAIN, "Show Main Window");
        AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
        append_string(menu, ID_SERVICE, "Service Management");
        append_string(menu, ID_PERFSTAT, "Ext2Fsd Statistics");
        AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
        append_string(menu, ID_ABOUT, "About Ext2Mgr");
        AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
        append_string(menu, ID_EXIT, "Exit ...");
        let mut point = POINT { x: 0, y: 0 };
        GetCursorPos(&mut point);
        SetForegroundWindow(hwnd);
        let id = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
            point.x,
            point.y,
            0,
            hwnd,
            std::ptr::null(),
        ) as u32;
        PostMessageW(hwnd, WM_NULL, 0, 0);
        DestroyMenu(menu);
        if id == 0 {
            None
        } else {
            Some(id)
        }
    }
}

/// Attach menu + tray + subclass once the iced HWND exists.
pub fn ensure_attached() -> bool {
    let Some(hwnd) = find_main_hwnd() else {
        return false;
    };

    {
        let state = match STATE.lock() {
            Ok(state) => state,
            Err(_) => return false,
        };
        if state.hwnd == hwnd as usize && state.old_wnd_proc.is_some() {
            return true;
        }
    }

    {
        let mut state = match STATE.lock() {
            Ok(state) => state,
            Err(_) => return false,
        };
        if state.hwnd != 0 && state.hwnd != hwnd as usize && state.tray_added {
            tray_remove(state.hwnd as HWND);
            state.tray_added = false;
        }
    }

    let icon = load_app_icon();
    let menu = build_main_menu();

    // Never hold STATE across these calls: they can synchronously dispatch window
    // messages to subclass_proc, which also locks STATE.
    let previous_wnd_proc = unsafe {
        SetMenu(hwnd, menu);
        DrawMenuBar(hwnd);
        SetWindowPos(
            hwnd,
            0,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED,
        );
        GetWindowLongPtrW(hwnd, GWLP_WNDPROC)
    };

    unsafe {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, subclass_proc as *const () as isize);
    }

    let tray_ok = icon != 0 && tray_add(hwnd, icon);

    {
        let mut state = match STATE.lock() {
            Ok(state) => state,
            Err(_) => return false,
        };
        state.old_wnd_proc = Some(previous_wnd_proc);
        state.hwnd = hwnd as usize;
        state.menu = menu as usize;
        state.icon = icon as usize;
        state.tray_added = tray_ok;
    }
    true
}

/// Check View menu items (properties pane, SI/binary, bytes/bits).
pub fn sync_view_menu(show_properties_pane: bool, use_binary: bool, display_bits: bool) {
    let Some((menu, hwnd)) = (|| {
        let state = STATE.lock().ok()?;
        if state.menu == 0 {
            return None;
        }
        Some((state.menu as HMENU, state.hwnd as HWND))
    })() else {
        return;
    };

    unsafe {
        CheckMenuItem(
            menu,
            ID_VIEW_PROPERTIES_PANE,
            MF_BYCOMMAND
                | if show_properties_pane {
                    MF_CHECKED
                } else {
                    MF_UNCHECKED
                },
        );
        CheckMenuItem(
            menu,
            ID_VIEW_SI_UNITS,
            MF_BYCOMMAND
                | if use_binary {
                    MF_UNCHECKED
                } else {
                    MF_CHECKED
                },
        );
        CheckMenuItem(
            menu,
            ID_VIEW_BINARY_UNITS,
            MF_BYCOMMAND
                | if use_binary {
                    MF_CHECKED
                } else {
                    MF_UNCHECKED
                },
        );
        CheckMenuItem(
            menu,
            ID_VIEW_DISPLAY_BYTES,
            MF_BYCOMMAND
                | if display_bits {
                    MF_UNCHECKED
                } else {
                    MF_CHECKED
                },
        );
        CheckMenuItem(
            menu,
            ID_VIEW_DISPLAY_BITS,
            MF_BYCOMMAND
                | if display_bits {
                    MF_CHECKED
                } else {
                    MF_UNCHECKED
                },
        );
        if hwnd != 0 {
            DrawMenuBar(hwnd);
        }
    }
}

/// Open a local file or URL with the shell (FAQ, project site, donate links).
pub fn shell_open(path_or_url: &str) -> Result<(), String> {
    let wide = to_wide(path_or_url);
    let open = to_wide("open");
    let result = unsafe {
        windows_sys::Win32::UI::Shell::ShellExecuteW(
            0,
            open.as_ptr(),
            wide.as_ptr(),
            std::ptr::null(),
            std::ptr::null(),
            SW_SHOW as i32,
        )
    };
    // ShellExecute returns > 32 on success (cast as HINSTANCE / isize).
    if result as isize > 32 {
        Ok(())
    } else {
        Err(format!("ShellExecute failed ({result})"))
    }
}

/// Resolve FAQ.txt next to the exe, under assets/, or beside the source tree.
pub fn documentation_path() -> Option<std::path::PathBuf> {
    let mut candidates = Vec::new();
    if let Ok(exe) = std::env::current_exe() {
        if let Some(parent) = exe.parent() {
            candidates.push(parent.join("Documents").join("FAQ.txt"));
            candidates.push(parent.join("FAQ.txt"));
            candidates.push(parent.join("assets").join("FAQ.txt"));
        }
    }
    candidates.push(
        std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("assets/FAQ.txt"),
    );
    candidates.push(
        std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("../Ext4Fsd/FAQ.txt"),
    );
    candidates.into_iter().find(|path| path.is_file())
}

/// Enable or gray File → Ext2Mgr autostart menu items.
pub fn sync_autorun_menu(autorun_enabled: bool) {
    let Some((menu, hwnd)) = (|| {
        let state = STATE.lock().ok()?;
        if state.menu == 0 {
            return None;
        }
        Some((state.menu as HMENU, state.hwnd as HWND))
    })() else {
        return;
    };

    unsafe {
        if autorun_enabled {
            EnableMenuItem(menu, ID_ENABLE_AUTOSTART, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(menu, ID_DISABLE_AUTOSTART, MF_BYCOMMAND | MF_ENABLED);
        } else {
            EnableMenuItem(menu, ID_ENABLE_AUTOSTART, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(menu, ID_DISABLE_AUTOSTART, MF_BYCOMMAND | MF_GRAYED);
        }
        if hwnd != 0 {
            DrawMenuBar(hwnd);
        }
    }
}

/// Tools → Assign Drive Letter / Unmount enable state from selection letters.
pub fn sync_mount_letter_menus(can_assign: bool, can_unmount: bool) {
    let Some((menu, hwnd)) = (|| {
        let state = STATE.lock().ok()?;
        if state.menu == 0 {
            return None;
        }
        Some((state.menu as HMENU, state.hwnd as HWND))
    })() else {
        return;
    };

    unsafe {
        EnableMenuItem(
            menu,
            ID_DRV_QUICK_MOUNT,
            MF_BYCOMMAND
                | if can_assign {
                    MF_ENABLED
                } else {
                    MF_GRAYED
                },
        );
        EnableMenuItem(
            menu,
            ID_DRV_UNMOUNT,
            MF_BYCOMMAND
                | if can_unmount {
                    MF_ENABLED
                } else {
                    MF_GRAYED
                },
        );
        if hwnd != 0 {
            DrawMenuBar(hwnd);
        }
    }
}

pub enum ContextKind {
    Volume { has_letter: bool, is_ext: bool },
    Disk {
        has_letter: bool,
        is_ext: bool,
        is_mbr_part: bool,
    },
}

/// Synchronous TrackPopupMenu at cursor; returns selected command id.
pub fn track_list_context(kind: ContextKind) -> Option<u32> {
    let _ = ensure_attached();
    let hwnd = find_main_hwnd()?;
    unsafe {
        let menu = CreatePopupMenu();
        match kind {
            ContextKind::Volume {
                has_letter,
                is_ext,
            } => {
                if is_ext && !has_letter {
                    append_string(menu, ID_DRV_QUICK_MOUNT, "Assign Drive Letter\tF4");
                } else if has_letter {
                    append_string(menu, ID_DRV_UNMOUNT, "Unmount");
                }
                append_string(menu, ID_DRV_LETTER, "Change Drive Letter\tF10");
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                if is_ext {
                    append_string(menu, ID_CHANGE, "Ext2 Volume Management\tF3");
                    AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                }
                append_string(menu, ID_FLUSH_BUFFER, "Flush Cache to Disk\tF11");
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                append_string(menu, ID_COPY, "Copy Item to Clipboard");
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
            }
            ContextKind::Disk {
                has_letter,
                is_ext,
                is_mbr_part,
            } => {
                if is_ext && !has_letter {
                    append_string(menu, ID_DRV_QUICK_MOUNT, "Assign Drive Letter\tF4");
                } else if has_letter {
                    append_string(menu, ID_DRV_UNMOUNT, "Unmount");
                }
                append_string(menu, ID_DRV_LETTER, "Change Drive Letter\tF10");
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                if is_ext {
                    append_string(menu, ID_CHANGE, "Ext2 Volume Management\tF3");
                    AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                }
                append_string(menu, ID_FLUSH_BUFFER, "Flush Cache to Disk\tF11");
                if is_mbr_part {
                    append_string(menu, ID_CHANGE_PARTTYPE, "Change Partition Type\tF12");
                }
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
                append_string(menu, ID_COPY, "Copy Item to Clipboard");
                AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
            }
        }
        append_string(menu, ID_REFRESH, "Reload and Refresh\tF5");
        AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
        append_string(menu, ID_PROPERTY, "Show Properties\tF6");
        AppendMenuW(menu, MF_SEPARATOR, 0, std::ptr::null());
        append_string(menu, ID_SERVICE, "Service Management\tF7");
        append_string(menu, ID_PERFSTAT, "Ext2Fsd Statistics\tF8");
        append_string(menu, ID_REMOVE_DEAD_LETTER, "Remove Dead Letters\tF9");

        let mut point = POINT { x: 0, y: 0 };
        GetCursorPos(&mut point);
        SetForegroundWindow(hwnd);
        let id = TrackPopupMenu(
            menu,
            TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
            point.x,
            point.y,
            0,
            hwnd,
            std::ptr::null(),
        ) as u32;
        PostMessageW(hwnd, WM_NULL, 0, 0);
        DestroyMenu(menu);
        if id == 0 {
            None
        } else {
            Some(id)
        }
    }
}
