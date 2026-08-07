//! Ext2Mgr (Iced port) — Ext2/Ext4 volume manager GUI.
//!
//! Layout:
//! - [`ui`] — iced window / tables / dialogs
//! - [`disk`] — volume/disk enumeration + FS probes
//! - [`mount`] — Ext2Srv pipe, mount modes, persist, dead letters
//! - [`service`] — Ext2Fsd service settings
//! - [`win`] — native chrome, accent, autorun, clipboard, msgbox

mod disk;
#[cfg(windows)]
mod mount;
#[cfg(windows)]
mod service;
#[cfg(windows)]
mod win;
mod ui;

use iced::window;
use iced::Size;

fn load_window_icon() -> Option<window::Icon> {
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("assets/Ext2Mgr.ico");
    let file = ::image::open(&path).ok()?.into_rgba8();
    let width = file.width();
    let height = file.height();
    window::icon::from_rgba(file.into_raw(), width, height).ok()
}

fn main() -> iced::Result {
    iced::application(ui::Ext2MgrApp::new, ui::Ext2MgrApp::update, ui::Ext2MgrApp::view)
        .title(ui::Ext2MgrApp::title)
        .subscription(ui::Ext2MgrApp::subscription)
        .theme(ui::Ext2MgrApp::theme)
        .window(window::Settings {
            size: Size::new(720.0, 720.0),
            icon: load_window_icon(),
            ..Default::default()
        })
        .run()
}
