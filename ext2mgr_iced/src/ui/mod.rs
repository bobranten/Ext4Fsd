//! Iced GUI: window state, tables, dialogs, and message update loop.
//!
//! Split with `include!` (one module) so Dialog fields / `impl Ext2MgrApp`
//! methods stay visible across files:
//! - `types.rs` — Selection, Dialog, Message
//! - `helpers.rs` — mount/letter/detail/column helpers
//! - `view.rs` — tables + dialogs + shell layout
//! - `update.rs` — Message handling + native command Tasks
//!
//! Domain logic lives under `crate::disk`, `crate::mount`, `crate::service`, `crate::win`.

use crate::disk::enum_disk::{
    display_volume_guid, format_letters, DiskEntry, DiskRow, SizeUnitStyle, VolumeEntry,
};
use iced::widget::{
    button, checkbox, column, container, image, mouse_area, pick_list, radio, row, scrollable,
    stack, text, text_editor, text_input, Column, Space,
};
use iced::{
    alignment, keyboard, mouse, time, window, Background, Border, Color, Element, Event, Length,
    Shadow, Size, Subscription, Task, Theme,
};
use iced::keyboard::key::Named;

#[cfg(all(test, windows))]
mod gating_probe;

include!("types.rs");

pub struct Ext2MgrApp {
    status: String,
    clock: String,
    disks: Vec<DiskEntry>,
    volumes: Vec<VolumeEntry>,
    disk_rows: Vec<DiskRow>,
    selection: Selection,
    dialog: Option<Dialog>,
    detail_text: String,
    /// Selectable/copyable properties pane under the disk list.
    detail_editor: text_editor::Content,
    /// Volume: icon, letters, type, fs, total, used, codepage, physical.
    volume_col_widths: [f32; 8],
    /// Disk: name, type, fs, total, used, codepage, partition type.
    disk_col_widths: [f32; 7],
    resizing: Option<(ListKind, usize, f32)>,
    chrome_ready: bool,
    /// Windows accent (Settings / DWM); drives selection highlight.
    accent: Color,
    accent_text: Color,
    /// View → SI vs binary capacity labels (KB vs KiB, etc.).
    size_units: SizeUnitStyle,
    /// View → display capacities as bits (×8) instead of bytes.
    display_bits: bool,
    /// View → show the properties pane under Disks / Partitions.
    show_properties_pane: bool,
    #[cfg(windows)]
    service_draft: crate::service::mgr::GlobalProperty,
}

include!("helpers.rs");
include!("view.rs");
include!("update.rs");

impl Ext2MgrApp {
    pub fn new() -> (Self, Task<Message>) {
        let (disks, volumes, disk_rows) = crate::disk::enum_disk::enumerate_all();
        #[cfg(windows)]
        let accent = crate::win::accent::read_accent_color();
        #[cfg(not(windows))]
        let accent = Color::from_rgb(0.0, 0.45, 0.85);
        let accent_text = {
            #[cfg(windows)]
            {
                crate::win::accent::contrasting_text(accent)
            }
            #[cfg(not(windows))]
            {
                Color::WHITE
            }
        };
        let mut app = Self {
            status: "Ready".to_string(),
            clock: Self::now_clock(),
            disks,
            volumes,
            disk_rows,
            selection: Selection::None,
            dialog: None,
            detail_text: "Ready".to_string(),
            detail_editor: text_editor::Content::with_text("Ready"),
            // Seed widths; autofit_column_widths replaces these from live data.
            volume_col_widths: [24.0, 60.0, 60.0, 80.0, 80.0, 70.0, 70.0, 120.0],
            disk_col_widths: [36.0, 60.0, 80.0, 80.0, 70.0, 70.0, 120.0],
            resizing: None,
            chrome_ready: false,
            accent,
            accent_text,
            size_units: SizeUnitStyle::Si,
            display_bits: false,
            show_properties_pane: true,
            #[cfg(windows)]
            service_draft: {
                crate::service::mgr::query_global_property().unwrap_or_default()
            },
        };
        app.autofit_column_widths();
        app.recompute_detail();
        #[cfg(windows)]
        {
            crate::win::chrome::sync_autorun_menu(crate::win::autorun::is_autorun_enabled());
        }
        let startup = app.fit_window_task();
        (app, startup)
    }

    pub fn title(&self) -> String {
        "Ext2 Volume Manager".to_string()
    }

    pub fn theme(&self) -> Theme {
        Theme::custom(
            "WindowsAccent",
            iced::theme::Palette {
                background: Color::from_rgb(0.97, 0.97, 0.97),
                text: Color::from_rgb(0.1, 0.1, 0.1),
                primary: self.accent,
                success: Color::from_rgb(0.15, 0.55, 0.25),
                danger: Color::from_rgb(0.75, 0.2, 0.2),
                warning: Color::from_rgb(0.85, 0.55, 0.1),
            },
        )
    }

    pub fn refresh_accent(&mut self) {
        #[cfg(windows)]
        {
            let accent = crate::win::accent::read_accent_color();
            if accent != self.accent {
                self.accent = accent;
                self.accent_text = crate::win::accent::contrasting_text(accent);
            }
        }
    }

    pub fn subscription(&self) -> Subscription<Message> {
        let tick = time::every(std::time::Duration::from_secs(1)).map(|_| Message::Tick);
        let input = iced::event::listen_with(|event, _status, _id| match event {
            Event::Mouse(mouse::Event::CursorMoved { position }) => {
                Some(Message::CursorMoved(position.x))
            }
            Event::Mouse(mouse::Event::ButtonReleased(mouse::Button::Left)) => {
                Some(Message::EndResize)
            }
            Event::Keyboard(keyboard::Event::KeyPressed { key, modifiers, .. }) => {
                if modifiers.control() || modifiers.alt() || modifiers.logo() {
                    return None;
                }
                match key {
                    keyboard::Key::Named(Named::ArrowUp) => Some(Message::SelectPrev),
                    keyboard::Key::Named(Named::ArrowDown) => Some(Message::SelectNext),
                    keyboard::Key::Named(Named::F1) => Some(Message::MenuDocumentation),
                    keyboard::Key::Named(Named::F2) => Some(Message::MenuAbout),
                    keyboard::Key::Named(Named::F3) => Some(Message::OpenExt2Attrs),
                    keyboard::Key::Named(Named::F4) => Some(Message::QuickMount),
                    keyboard::Key::Named(Named::F5) => Some(Message::Refresh),
                    keyboard::Key::Named(Named::F6) => Some(Message::ShowProperties),
                    keyboard::Key::Named(Named::F7) => Some(Message::OpenService),
                    keyboard::Key::Named(Named::F8) => Some(Message::OpenPerfStat),
                    keyboard::Key::Named(Named::F9) => Some(Message::OpenDeadLetters),
                    keyboard::Key::Named(Named::F10) => Some(Message::OpenMountPoints),
                    keyboard::Key::Named(Named::F11) => Some(Message::FlushSelected),
                    keyboard::Key::Named(Named::F12) => Some(Message::OpenPartitionType),
                    _ => None,
                }
            }
            _ => None,
        });
        Subscription::batch(vec![tick, input])
    }

}

