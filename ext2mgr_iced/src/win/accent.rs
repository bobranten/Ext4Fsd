//! Live Windows accent / highlight color for list selection.


use iced::Color;

/// Prefer Settings accent, then DWM colorization, then classic highlight.
pub fn read_accent_color() -> Color {
    if let Some(color) = accent_from_registry(
        r"Software\Microsoft\Windows\CurrentVersion\Explorer\Accent",
        "AccentColorMenu",
        ColorLayout::Abgr,
    ) {
        return color;
    }
    if let Some(color) = accent_from_registry(
        r"Software\Microsoft\Windows\DWM",
        "AccentColor",
        ColorLayout::Abgr,
    ) {
        return color;
    }
    if let Some(color) = accent_from_registry(
        r"Software\Microsoft\Windows\DWM",
        "ColorizationColor",
        ColorLayout::Aarrggbb,
    ) {
        return color;
    }
    sys_highlight_color()
}

pub fn contrasting_text(background: Color) -> Color {
    let luminance = 0.2126 * background.r + 0.7152 * background.g + 0.0722 * background.b;
    if luminance > 0.55 {
        Color::from_rgb(0.05, 0.05, 0.05)
    } else {
        Color::WHITE
    }
}

enum ColorLayout {
    /// Windows accent DWORD: 0xAABBGGRR
    Abgr,
    /// DWM ColorizationColor: 0xAARRGGBB
    Aarrggbb,
}

fn accent_from_registry(subkey: &str, value_name: &str, layout: ColorLayout) -> Option<Color> {
    use std::os::windows::ffi::OsStrExt;

    let wide_path: Vec<u16> = std::ffi::OsStr::new(subkey)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();
    let mut key = 0;
    let status = unsafe {
        windows_sys::Win32::System::Registry::RegOpenKeyExW(
            windows_sys::Win32::System::Registry::HKEY_CURRENT_USER,
            wide_path.as_ptr(),
            0,
            windows_sys::Win32::System::Registry::KEY_READ,
            &mut key,
        )
    };
    if status != 0 {
        return None;
    }
    let wide_name: Vec<u16> = std::ffi::OsStr::new(value_name)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();
    let mut value_type = 0u32;
    let mut data = [0u8; 4];
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
    unsafe {
        windows_sys::Win32::System::Registry::RegCloseKey(key);
    }
    if status != 0 || data_len < 4 {
        return None;
    }
    let dword = u32::from_le_bytes(data);
    if dword == 0 {
        return None;
    }
    Some(match layout {
        ColorLayout::Abgr => Color::from_rgb8(
            (dword & 0xFF) as u8,
            ((dword >> 8) & 0xFF) as u8,
            ((dword >> 16) & 0xFF) as u8,
        ),
        ColorLayout::Aarrggbb => Color::from_rgb8(
            ((dword >> 16) & 0xFF) as u8,
            ((dword >> 8) & 0xFF) as u8,
            (dword & 0xFF) as u8,
        ),
    })
}

fn sys_highlight_color() -> Color {
    // COLOR_HIGHLIGHT = 13
    let bgr = unsafe { windows_sys::Win32::Graphics::Gdi::GetSysColor(13) };
    Color::from_rgb8(
        (bgr & 0xFF) as u8,
        ((bgr >> 8) & 0xFF) as u8,
        ((bgr >> 16) & 0xFF) as u8,
    )
}
