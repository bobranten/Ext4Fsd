//! Win32 chrome, accent, autorun, clipboard, message boxes.

#[cfg(windows)]
pub mod chrome;
#[cfg(windows)]
pub mod accent;
#[cfg(windows)]
pub mod autorun;
#[cfg(windows)]
pub mod clipboard;
#[cfg(windows)]
pub mod msgbox;
