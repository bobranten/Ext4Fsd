impl Ext2MgrApp {
    fn apply_native_command(&mut self, command_id: u32) -> Task<Message> {
        #[cfg(windows)]
        {
            use crate::win::chrome as chrome;
            let message = match command_id {
                x if x == chrome::ID_EXIT => {
                    std::process::exit(0);
                }
                x if x == chrome::ID_REFRESH => Some(Message::Refresh),
                x if x == chrome::ID_COPYALL => Some(Message::MenuCopyAll),
                x if x == chrome::ID_COPY => Some(Message::MenuCopyItem),
                x if x == chrome::ID_ABOUT => Some(Message::MenuAbout),
                x if x == chrome::ID_DONATE => Some(Message::MenuDonate),
                x if x == chrome::ID_DOCUMENTATION => Some(Message::MenuDocumentation),
                x if x == chrome::ID_SERVICE => Some(Message::OpenService),
                x if x == chrome::ID_CHANGE => Some(Message::OpenExt2Attrs),
                x if x == chrome::ID_DRV_LETTER => Some(Message::OpenMountPoints),
                x if x == chrome::ID_DRV_QUICK_MOUNT => Some(Message::QuickMount),
                x if x == chrome::ID_DRV_UNMOUNT => Some(Message::UnmountSelected),
                x if x == chrome::ID_PROPERTY => Some(Message::ShowProperties),
                x if x == chrome::ID_REMOVE_DEAD_LETTER => Some(Message::OpenDeadLetters),
                x if x == chrome::ID_ENABLE_AUTOSTART => Some(Message::EnableAutorun),
                x if x == chrome::ID_DISABLE_AUTOSTART => Some(Message::DisableAutorun),
                x if x == chrome::ID_PERFSTAT => Some(Message::OpenPerfStat),
                x if x == chrome::ID_CHANGE_PARTTYPE => Some(Message::OpenPartitionType),
                x if x == chrome::ID_FLUSH_BUFFER => Some(Message::FlushSelected),
                x if x == chrome::ID_VIEW_SI_UNITS => Some(Message::UseSiUnits),
                x if x == chrome::ID_VIEW_BINARY_UNITS => Some(Message::UseBinaryUnits),
                x if x == chrome::ID_VIEW_DISPLAY_BYTES => Some(Message::DisplayWithBytes),
                x if x == chrome::ID_VIEW_DISPLAY_BITS => Some(Message::DisplayWithBits),
                x if x == chrome::ID_VIEW_PROPERTIES_PANE => Some(Message::TogglePropertiesPane),
                _ => None,
            };
            if let Some(message) = message {
                self.update(message)
            } else {
                Task::none()
            }
        }
        #[cfg(not(windows))]
        {
            let _ = command_id;
            Task::none()
        }
    }

    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Tick => {
                self.clock = Self::now_clock();
                self.refresh_accent();
                #[cfg(windows)]
                {
                    if !self.chrome_ready {
                        self.chrome_ready = crate::win::chrome::ensure_attached();
                        if self.chrome_ready {
                            crate::win::chrome::sync_autorun_menu(crate::win::autorun::is_autorun_enabled());
                            self.sync_native_menus();
                        }
                    }
                    let mut tasks = Vec::new();
                    for command_id in crate::win::chrome::poll_commands() {
                        tasks.push(self.apply_native_command(command_id));
                    }
                    return Task::batch(tasks);
                }
            }
            Message::MenuCopyAll => {
                let mut text = String::new();
                text.push_str("=== Volumes ===\n");
                for volume in &self.volumes {
                    text.push_str(&format!(
                        "{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
                        format_letters(&volume.letters),
                        volume.volume_kind,
                        volume.filesystem,
                        self.format_size(volume.total_bytes),
                        self.format_size(volume.used_bytes),
                        volume.codepage,
                        volume.physical_object,
                    ));
                }
                text.push_str("\n=== Disks / Partitions ===\n");
                for disk in &self.disks {
                    text.push_str(&format!(
                        "{}\t{}\t{}\n",
                        disk.display_name,
                        disk.style,
                        self.format_size(disk.total_bytes)
                    ));
                    for partition in &disk.partitions {
                        text.push_str(&format!(
                            "  {}\t{}\t{}\t{}\t{}\t{}\t{}\n",
                            format_letters(&partition.letters),
                            partition.style,
                            partition.filesystem,
                            self.format_size(partition.total_bytes),
                            self.format_size(partition.used_bytes),
                            partition.codepage,
                            partition.partition_type,
                        ));
                    }
                }
                #[cfg(windows)]
                {
                    self.status = match crate::win::clipboard::set_text(&text) {
                        Ok(()) => format!("Copied {} bytes to clipboard.", text.len()),
                        Err(err) => format!("Clipboard copy failed: {err}"),
                    };
                }
                #[cfg(not(windows))]
                {
                    self.status = "Clipboard copy is Windows-only.".to_string();
                }
            }
            Message::MenuCopyItem => {
                let text = self.detail_text.clone();
                if text.is_empty() || text == "Ready" {
                    self.status = "Nothing selected to copy.".to_string();
                } else {
                    #[cfg(windows)]
                    {
                        self.status = match crate::win::clipboard::set_text(&text) {
                            Ok(()) => format!("Copied selected item ({} bytes).", text.len()),
                            Err(err) => format!("Clipboard copy failed: {err}"),
                        };
                    }
                    #[cfg(not(windows))]
                    {
                        self.status = "Clipboard copy is Windows-only.".to_string();
                    }
                }
            }
            Message::MenuAbout => {
                #[cfg(windows)]
                {
                    let driver_line = match crate::service::mgr::query_driver_version() {
                        Ok((version, date, _time)) => {
                            format!("Ext2Fsd: {version} ({date})")
                        }
                        Err(_) => "Ext2Fsd: NOT started !".to_string(),
                    };
                    self.dialog = Some(Dialog::About {
                        driver_line,
                        program_line: format!("Ext2Mgr: {}", env!("CARGO_PKG_VERSION")),
                    });
                }
                #[cfg(not(windows))]
                {
                    self.dialog = Some(Dialog::About {
                        driver_line: "Ext2Fsd: (non-Windows build)".to_string(),
                        program_line: format!("Ext2Mgr: {}", env!("CARGO_PKG_VERSION")),
                    });
                }
            }
            Message::MenuDonate => {
                self.dialog = Some(Dialog::Donate);
            }
            Message::MenuDocumentation => {
                #[cfg(windows)]
                {
                    match crate::win::chrome::documentation_path() {
                        Some(path) => match crate::win::chrome::shell_open(&path.to_string_lossy()) {
                            Ok(()) => {
                                self.status = format!("Opened documentation: {}", path.display());
                            }
                            Err(err) => self.report_warning(err),
                        },
                        None => {
                            self.report_warning(
                                "FAQ.txt not found (expected under assets/ or Documents/).",
                            );
                        }
                    }
                }
                #[cfg(not(windows))]
                {
                    self.status = "Documentation open is Windows-only.".to_string();
                }
            }
            Message::OpenAboutWebsite => {
                #[cfg(windows)]
                {
                    if let Err(err) =
                        crate::win::chrome::shell_open("https://github.com/bobranten/Ext4Fsd/")
                    {
                        self.report_warning(err);
                    }
                }
            }
            Message::OpenIcedPortWebsite => {
                #[cfg(windows)]
                {
                    if let Err(err) =
                        crate::win::chrome::shell_open("https://github.com/Obsidian-Jackal/Ext4Fsd")
                    {
                        self.report_warning(err);
                    }
                }
            }
            Message::OpenDonateSourceForge => {
                #[cfg(windows)]
                {
                    if let Err(err) = crate::win::chrome::shell_open(
                        "http://sourceforge.net/project/project_donations.php?group_id=43775",
                    ) {
                        self.status = err;
                    }
                }
            }
            Message::OpenDonatePayPal => {
                #[cfg(windows)]
                {
                    if let Err(err) = crate::win::chrome::shell_open(
                        "https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=m@ext2fsd.com&item_name=Donation&return=https://sourceforge.net/projects/ext2fsd",
                    ) {
                        self.status = err;
                    }
                }
            }
            Message::FlushSelected => {
                #[cfg(windows)]
                {
                    let flush_target = match self.selection {
                        Selection::Volume(index) => self.volumes.get(index).map(|volume| {
                            (
                                volume.letters.clone(),
                                volume.win32_volume_name.clone(),
                                volume.physical_object.clone(),
                            )
                        }),
                        Selection::Partition { disk, part } => self
                            .disks
                            .get(disk)
                            .and_then(|entry| entry.partitions.get(part))
                            .map(|partition| {
                                (
                                    partition.letters.clone(),
                                    partition.win32_volume_name.clone(),
                                    if partition.symlink.is_empty() {
                                        String::new()
                                    } else {
                                        partition.symlink.clone()
                                    },
                                )
                            }),
                        _ => None,
                    };
                    match flush_target {
                        Some((letters, win32, physical)) => {
                            match crate::mount::ops::flush_volume(&letters, &win32, &physical) {
                                Ok(()) => {
                                    self.status = "Flushed volume cache to disk.".to_string();
                                }
                                Err(err) => {
                                    self.report_warning(format!("Flush failed: {err}"));
                                }
                            }
                        }
                        None => {
                            self.status = "Select a volume or partition to flush.".to_string();
                        }
                    }
                }
                #[cfg(not(windows))]
                {
                    self.status = "Flush is Windows-only.".to_string();
                }
            }
            Message::EnableAutorun => {
                #[cfg(windows)]
                {
                    match crate::win::autorun::set_autorun(true) {
                        Ok(()) => {
                            crate::win::chrome::sync_autorun_menu(true);
                            self.status =
                                "Autostart enabled (HKCU Run: Ext2MgrIced).".to_string();
                        }
                        Err(err) => self.report_warning(format!("Enable autostart failed: {err}")),
                    }
                }
            }
            Message::DisableAutorun => {
                #[cfg(windows)]
                {
                    match crate::win::autorun::set_autorun(false) {
                        Ok(()) => {
                            crate::win::chrome::sync_autorun_menu(false);
                            self.status = "Autostart disabled.".to_string();
                        }
                        Err(err) => self.report_warning(format!("Disable autostart failed: {err}")),
                    }
                }
            }
            Message::OpenPerfStat => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::query_perfstat_rows() {
                        Ok(rows) => self.dialog = Some(Dialog::PerfStat { rows }),
                        Err(err) => self.report_warning(err),
                    }
                }
                #[cfg(not(windows))]
                {
                    self.status = "PerfStat is Windows-only.".to_string();
                }
            }
            Message::CopyPerfStat => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::query_perfstat_text() {
                        Ok(text) => match crate::win::clipboard::set_text(&text) {
                            Ok(()) => self.status = "Statistics copied to clipboard.".to_string(),
                            Err(err) => self.report_warning(err),
                        },
                        Err(err) => self.report_warning(err),
                    }
                }
                #[cfg(not(windows))]
                {
                    self.status = "Clipboard copy is Windows-only.".to_string();
                }
            }
            Message::OpenPartitionType => {
                match self.selection {
                    Selection::Partition { disk, part } => {
                        let Some(partition) = self
                            .disks
                            .get(disk)
                            .and_then(|entry| entry.partitions.get(part))
                        else {
                            self.status = "Partition not found.".to_string();
                            return Task::none();
                        };
                        if !partition.style.eq_ignore_ascii_case("Basic") {
                            self.status =
                                "Change Partition Type is only for MBR (Basic) partitions."
                                    .to_string();
                            return Task::none();
                        }
                        // Ext2Mgr PartitionType dialog: start from current type id if known.
                        let selected_type = 0x83u8;
                        self.dialog = Some(Dialog::PartitionType {
                            disk,
                            part,
                            selected_type,
                            note: format!(
                                "Current type text: {}. Apply writes MBR type via Ext2SetPartitionType (not yet wired to disk IOCTL).",
                                partition.partition_type
                            ),
                        });
                    }
                    _ => {
                        self.status = "Select an MBR partition first.".to_string();
                    }
                }
            }
            Message::PartitionTypePick(type_id) => {
                if let Some(Dialog::PartitionType { selected_type, .. }) = self.dialog.as_mut() {
                    *selected_type = type_id;
                }
            }
            Message::ApplyPartitionType => {
                if let Some(Dialog::PartitionType {
                    disk,
                    part,
                    selected_type,
                    ..
                }) = &self.dialog
                {
                    // Ext2Mgr shows success/failure MessageBox after Ext2SetPartitionType.
                    self.report_warning(format!(
                        "Failed to set the partition type!\n(IOCTL for disk {disk} part {part} type 0x{selected_type:02X} not wired yet)"
                    ));
                }
                self.dialog = None;
            }
            Message::SelectPrev => {
                self.select_prev_entry();
            }
            Message::SelectNext => {
                self.select_next_entry();
            }
            Message::ContextVolume(index) => {
                self.selection = Selection::Volume(index);
                self.recompute_detail();
                #[cfg(windows)]
                {
                    let volume = self.volumes.get(index);
                    let has_letter = volume.map(|entry| !entry.letters.is_empty()).unwrap_or(false);
                    let is_ext = volume
                        .map(|entry| crate::mount::ops::is_ext_family(&entry.filesystem))
                        .unwrap_or(false);
                    if let Some(command_id) = crate::win::chrome::track_list_context(
                        crate::win::chrome::ContextKind::Volume { has_letter, is_ext },
                    ) {
                        return self.apply_native_command(command_id);
                    }
                }
            }
            Message::ContextDiskRow(row_index) => {
                if let Some(row) = self.disk_rows.get(row_index) {
                    self.selection = match row {
                        DiskRow::DiskHeader { disk_index } => Selection::Disk(*disk_index),
                        DiskRow::Partition { disk_index, partition_index } => Selection::Partition {
                            disk: *disk_index,
                            part: *partition_index,
                        },
                        DiskRow::Spacer => Selection::None,
                    };
                    self.recompute_detail();
                    #[cfg(windows)]
                    {
                        let (has_letter, is_ext, is_mbr_part) = match self.selection {
                            Selection::Partition { disk, part } => {
                                let partition = self
                                    .disks
                                    .get(disk)
                                    .and_then(|entry| entry.partitions.get(part));
                                (
                                    partition.map(|entry| !entry.letters.is_empty()).unwrap_or(false),
                                    partition
                                        .map(|entry| crate::mount::ops::is_ext_family(&entry.filesystem))
                                        .unwrap_or(false),
                                    partition
                                        .map(|entry| entry.style.eq_ignore_ascii_case("MBR"))
                                        .unwrap_or(false),
                                )
                            }
                            _ => (false, false, false),
                        };
                        if let Some(command_id) = crate::win::chrome::track_list_context(
                            crate::win::chrome::ContextKind::Disk {
                                has_letter,
                                is_ext,
                                is_mbr_part,
                            },
                        ) {
                            return self.apply_native_command(command_id);
                        }
                    }
                }
            }
            Message::ShowProperties => {
                self.recompute_detail();
                self.dialog = Some(Dialog::Properties);
            }
            Message::PropertiesQuickMount => {
                self.dialog = None;
                return self.quick_mount();
            }
            Message::PropertiesUnmount => {
                self.dialog = None;
                return self.unmount_selected();
            }
            Message::UnmountSelected => {
                return self.unmount_selected();
            }
            Message::PropertiesChangeMp => {
                self.dialog = None;
                return self.update(Message::OpenMountPoints);
            }
            Message::PropertiesExt2Info => {
                self.dialog = None;
                return self.update(Message::OpenExt2Attrs);
            }
            Message::Ext2Prefix(value) => {
                if let Some(Dialog::Ext2Attrs { hiding_prefix, .. }) = self.dialog.as_mut() {
                    *hiding_prefix = value;
                }
            }
            Message::Ext2Suffix(value) => {
                if let Some(Dialog::Ext2Attrs { hiding_suffix, .. }) = self.dialog.as_mut() {
                    *hiding_suffix = value;
                }
            }
            Message::Ext2Uid(value) => {
                if let Some(Dialog::Ext2Attrs { uid, .. }) = self.dialog.as_mut() {
                    *uid = value;
                }
            }
            Message::Ext2Gid(value) => {
                if let Some(Dialog::Ext2Attrs { gid, .. }) = self.dialog.as_mut() {
                    *gid = value;
                }
            }
            Message::Ext2Euid(value) => {
                if let Some(Dialog::Ext2Attrs { euid, .. }) = self.dialog.as_mut() {
                    *euid = value;
                }
            }
            Message::StartResize { list, column, cursor_x } => {
                self.resizing = Some((list, column, cursor_x));
            }
            Message::CursorMoved(cursor_x) => {
                if let Some((list, column, last_x)) = self.resizing {
                    let delta = if last_x == 0.0 { 0.0 } else { cursor_x - last_x };
                    let widths: &mut [f32] = match list {
                        ListKind::Volume => &mut self.volume_col_widths,
                        ListKind::Disk => &mut self.disk_col_widths,
                    };
                    let col_count = widths.len();
                    let max_width = if column + 1 == col_count {
                        1200.0
                    } else {
                        600.0
                    };
                    if let Some(width) = widths.get_mut(column) {
                        *width = (*width + delta).clamp(24.0, max_width);
                    }
                    self.resizing = Some((list, column, cursor_x));
                }
            }
            Message::EndResize => self.resizing = None,
            Message::DetailEditor(action) => {
                // Selection/copy only; ignore edit actions so content stays read-only.
                if !matches!(action, text_editor::Action::Edit(_)) {
                    self.detail_editor.perform(action);
                }
            }
            Message::UseSiUnits => self.set_size_units(SizeUnitStyle::Si),
            Message::UseBinaryUnits => self.set_size_units(SizeUnitStyle::Binary),
            Message::DisplayWithBytes => self.set_display_bits(false),
            Message::DisplayWithBits => self.set_display_bits(true),
            Message::TogglePropertiesPane => {
                self.show_properties_pane = !self.show_properties_pane;
                self.sync_native_menus();
            },
            Message::Refresh => {
                #[cfg(windows)]
                {
                    self.status = crate::mount::ops::pipe_status_str();
                }
                #[cfg(not(windows))]
                {
                    self.status = "Refreshed.".to_string();
                }
                return self.reload();
            }
            Message::SelectVolume(index) => {
                self.selection = Selection::Volume(index);
                self.recompute_detail();
            }
            Message::SelectDiskRow(row_index) => {
                if let Some(row) = self.disk_rows.get(row_index) {
                    self.selection = match row {
                        DiskRow::DiskHeader { disk_index } => Selection::Disk(*disk_index),
                        DiskRow::Partition {
                            disk_index,
                            partition_index,
                        } => Selection::Partition {
                            disk: *disk_index,
                            part: *partition_index,
                        },
                        DiskRow::Spacer => Selection::None,
                    };
                    self.recompute_detail();
                }
            }
            Message::QuickMount => return self.quick_mount(),
            Message::OpenMountPoints => {
                self.open_mount_points_dialog();
            }
            Message::MountPointsSelect(index) => {
                if let Some(Dialog::MountPoints { selected, .. }) = self.dialog.as_mut() {
                    *selected = Some(index);
                }
            }
            Message::OpenLetterPicker { replace_letter } => {
                self.open_letter_picker(replace_letter);
            }
            Message::CloseDialog => self.dialog = None,
            Message::DialogAbsorbClick => {}
            Message::PickLetter(letter) => {
                if let Some(Dialog::LetterPicker {
                    selected_letter, ..
                }) = self.dialog.as_mut()
                {
                    *selected_letter = Some(letter);
                }
            }
            Message::SetPersistMode(mode) => {
                if let Some(Dialog::LetterPicker {
                    persist_mode,
                    mount_uuid,
                    mount_win32,
                    replace_letter,
                    existing_session_letters,
                    existing_mountmgr_letters,
                    ..
                }) = self.dialog.as_mut()
                {
                    if mode == crate::mount::ops::MountMode::Ext2Automount && mount_uuid.is_none() {
                        // Ext2Fsd automount requires a volume UUID.
                    } else if mode == crate::mount::ops::MountMode::MountManager
                        && (mount_win32.is_empty()
                            || Self::assignment_blocked(
                                existing_mountmgr_letters,
                                *replace_letter,
                            ))
                    {
                        // Needs Volume{GUID}; at most one Mount Manager letter.
                    } else if mode == crate::mount::ops::MountMode::PermanentRegistry
                        && Self::session_manager_blocked(
                            existing_session_letters,
                            *replace_letter,
                        )
                    {
                        // One Session Manager letter per volume on Add / convert.
                    } else if mode == crate::mount::ops::MountMode::Temporary
                        && !existing_mountmgr_letters.is_empty()
                        && replace_letter
                            .map(|letter| !existing_mountmgr_letters.contains(&letter))
                            .unwrap_or(true)
                    {
                        // Temporary beside MountMgr is not an Explorer letter.
                    } else {
                        *persist_mode = mode;
                    }
                }
            }
            Message::ConfirmLetterPicker => return self.confirm_letter_picker(),
            Message::PipeOpFinished {
                ok_message,
                err_message,
                is_error,
                restore_mount_points,
                explorer_notify,
                run_automount,
            } => {
                #[cfg(windows)]
                {
                    for (letter, arrival) in explorer_notify {
                        crate::mount::ops::notify_explorer_letter(letter, arrival);
                    }
                }
                if let Some(err) = err_message {
                    if is_error {
                        self.report_error(err);
                    } else {
                        self.report_warning(err);
                    }
                } else if let Some(message) = ok_message {
                    self.status = message;
                    self.reload_lists();
                    if let Some((
                        volume_index,
                        disk_part,
                        letter,
                        mount_win32,
                        mount_uuid,
                    )) = restore_mount_points
                    {
                        let mut new_volume_index = volume_index;
                        if !mount_win32.is_empty() || mount_uuid.is_some() {
                            if let Some(found) = self.volumes.iter().position(|volume| {
                                (!mount_win32.is_empty()
                                    && volume.win32_volume_name == mount_win32)
                                    || (mount_uuid.is_some() && volume.uuid == mount_uuid)
                            }) {
                                new_volume_index = Some(found);
                            }
                        }
                        // Prefer disk/partition identity so we don't jump to another
                        // volume after reload reorders the volume list.
                        let letters = if let Some((disk, part)) = disk_part {
                            self.disks
                                .get(disk)
                                .and_then(|entry| entry.partitions.get(part))
                                .map(|partition| partition.letters.clone())
                                .unwrap_or_default()
                        } else if let Some(index) = new_volume_index {
                            self.volumes
                                .get(index)
                                .map(|volume| volume.letters.clone())
                                .unwrap_or_default()
                        } else {
                            Vec::new()
                        };
                        let selected = letters
                            .iter()
                            .position(|entry| *entry == letter)
                            .or_else(|| {
                                if letters.is_empty() {
                                    None
                                } else {
                                    Some(0)
                                }
                            });
                        self.dialog = Some(Dialog::MountPoints {
                            volume_index: new_volume_index,
                            disk_part,
                            selected,
                        });
                    }
                    if run_automount {
                        return self.automount_task();
                    }
                }
            }
            Message::AutomountFinished(report) => {
                if report.did_mount() {
                    let dialog = self.dialog.clone();
                    self.reload_lists();
                    // Keep Mount Points on the same disk/partition / uuid after
                    // volume list reorder from remounts.
                    if let Some(Dialog::MountPoints {
                        volume_index,
                        disk_part,
                        selected,
                    }) = dialog
                    {
                        let mut new_volume_index = volume_index;
                        if let Some((disk, part)) = disk_part {
                            if let Some(index) = self
                                .disks
                                .get(disk)
                                .and_then(|entry| entry.partitions.get(part))
                                .and_then(|partition| partition.volume_index)
                            {
                                new_volume_index = Some(index);
                            }
                        }
                        self.dialog = Some(Dialog::MountPoints {
                            volume_index: new_volume_index,
                            disk_part,
                            selected,
                        });
                    }
                }
                if let Some(summary) = report.summary() {
                    self.status = summary;
                }
            }
            Message::RemoveLetter(letter) => {
                #[cfg(windows)]
                {
                    // Never call DeleteVolumeMountPoint / pipe unmount on the UI thread —
                    // Ext2 DOS-only letters hang on `X:\` and MessageBox then chimes.
                    let (symlink, win32, uuid, restore) = match &self.dialog {
                        Some(Dialog::MountPoints {
                            volume_index,
                            disk_part,
                            ..
                        }) => {
                            let (symlink, win32, uuid) = if let Some(index) = volume_index {
                                self.volumes.get(*index).map_or_else(
                                    || (String::new(), String::new(), None),
                                    |volume| {
                                        (
                                            crate::mount::ops::dos_device_target(
                                                &volume.physical_object,
                                                &volume.symlink,
                                            ),
                                            volume.win32_volume_name.clone(),
                                            volume.uuid,
                                        )
                                    },
                                )
                            } else if let Some((disk, part)) = disk_part {
                                self.disks
                                    .get(*disk)
                                    .and_then(|entry| entry.partitions.get(*part))
                                    .map_or_else(
                                        || (String::new(), String::new(), None),
                                        |partition| {
                                            let linked = partition
                                                .volume_index
                                                .and_then(|index| self.volumes.get(index));
                                            let uuid = linked.and_then(|volume| volume.uuid);
                                            let physical = linked
                                                .map(|volume| volume.physical_object.as_str())
                                                .unwrap_or("");
                                            let volume_symlink = linked
                                                .map(|volume| volume.symlink.as_str())
                                                .unwrap_or(partition.symlink.as_str());
                                            (
                                                crate::mount::ops::dos_device_target(
                                                    physical,
                                                    volume_symlink,
                                                ),
                                                linked
                                                    .map(|volume| volume.win32_volume_name.clone())
                                                    .unwrap_or_else(|| {
                                                        partition.win32_volume_name.clone()
                                                    }),
                                                uuid,
                                            )
                                        },
                                    )
                            } else {
                                (String::new(), String::new(), None)
                            };
                            (
                                symlink,
                                win32.clone(),
                                uuid,
                                Some((*volume_index, *disk_part, letter, win32, uuid)),
                            )
                        }
                        _ => {
                            let (symlink, win32, uuid) = self
                                .selected_volume_index()
                                .and_then(|index| self.volumes.get(index))
                                .map(|volume| {
                                    (
                                        crate::mount::ops::dos_device_target(
                                            &volume.physical_object,
                                            &volume.symlink,
                                        ),
                                        volume.win32_volume_name.clone(),
                                        volume.uuid,
                                    )
                                })
                                .unwrap_or_else(|| (String::new(), String::new(), None));
                            (symlink, win32, uuid, None)
                        }
                    };
                    self.status = format!("Removing {letter}:…");
                    self.dialog = None;
                    return Task::perform(
                        async move {
                            let result = tokio::task::spawn_blocking(move || {
                                crate::mount::ops::unmount_letter_ex(
                                    letter, &symlink, &win32, uuid,
                                )
                            })
                            .await;
                            match result {
                                Ok(Ok(())) => (
                                    Some(format!("Removed {letter}:")),
                                    None,
                                    false,
                                    restore,
                                    vec![(letter, false)],
                                    false,
                                ),
                                Ok(Err(err)) => {
                                    (None, Some(err), false, None, Vec::new(), false)
                                }
                                Err(join_err) => (
                                    None,
                                    Some(join_err.to_string()),
                                    true,
                                    None,
                                    Vec::new(),
                                    false,
                                ),
                            }
                        },
                        |(
                            ok_message,
                            err_message,
                            is_error,
                            restore_mount_points,
                            explorer_notify,
                            run_automount,
                        )| {
                            Message::PipeOpFinished {
                                ok_message,
                                err_message,
                                is_error,
                                restore_mount_points,
                                explorer_notify,
                                run_automount,
                            }
                        },
                    );
                }
                #[cfg(not(windows))]
                {
                    let _ = letter;
                }
            }
            Message::OpenService => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::query_global_property() {
                        Ok(props) => {
                            self.service_draft = props;
                            self.dialog = Some(Dialog::Service);
                        }
                        Err(err) => {
                            self.report_error(format!(
                                "Cannot query Ext2Fsd service !\n{err}"
                            ));
                        }
                    }
                }
                #[cfg(not(windows))]
                {
                    self.status = "Service Management is Windows-only.".to_string();
                }
            }
            Message::StartService => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::start_driver() {
                        Ok(()) => {
                            self.status = "Ext2Fsd started.".to_string();
                            return self.reload();
                        }
                        Err(err) => self.report_error(err),
                    }
                }
            }
            Message::StopService => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::stop_driver() {
                        Ok(()) => {
                            self.status = "Ext2Fsd stopped.".to_string();
                            return self.reload();
                        }
                        Err(err) => self.report_error(err),
                    }
                }
            }
            Message::RestartService => {
                #[cfg(windows)]
                {
                    match crate::service::mgr::restart_driver() {
                        Ok(()) => {
                            self.status = "Ext2Fsd restarted.".to_string();
                            return self.reload();
                        }
                        Err(err) => self.report_error(err),
                    }
                }
            }
            Message::SaveService => {
                #[cfg(windows)]
                {
                    if self.service_draft.codepage.trim().is_empty() {
                        self.report_warning("You must select a codepage type.");
                        return Task::none();
                    }
                    if !crate::win::msgbox::confirm(
                        "Current service settings will be overwritten,\ndo you want continue ?",
                    ) {
                        return Task::none();
                    }
                    match crate::service::mgr::set_global_property(&self.service_draft) {
                        Ok(()) => {
                            self.status =
                                "Ext2 service settings updated successfully !".to_string();
                            self.dialog = None;
                        }
                        Err(err) => {
                            self.report_warning(format!(
                                "Failed to save the service settings !\n{err}"
                            ));
                        }
                    }
                }
            }
            Message::ServiceStartup(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.startup = value;
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServiceReadonly(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.readonly = value;
                    if value {
                        self.service_draft.ext3_writable = false;
                    }
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServiceExt3Writable(value) => {
                #[cfg(windows)]
                {
                    if !self.service_draft.readonly {
                        self.service_draft.ext3_writable = value;
                    }
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServiceAutomount(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.automount = value;
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServiceCodepage(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.codepage = value;
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServicePrefix(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.hiding_prefix = value;
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::ServiceSuffix(value) => {
                #[cfg(windows)]
                {
                    self.service_draft.hiding_suffix = value;
                }
                #[cfg(not(windows))]
                {
                    let _ = value;
                }
            }
            Message::OpenExt2Attrs => {
                #[cfg(windows)]
                {
                    let Some(volume_index) = self.selected_volume_index() else {
                        self.status = "Select an EXT volume/partition first.".to_string();
                        return Task::none();
                    };
                    let Some(volume) = self.volumes.get(volume_index).cloned() else {
                        return Task::none();
                    };
                    if !crate::mount::ops::is_ext_family(&volume.filesystem) {
                        self.status = "Ext2 Management requires an EXT2/3/4 volume.".to_string();
                        return Task::none();
                    }
                    if !crate::service::mgr::is_driver_started() {
                        self.report_error("Ext2Fsd service isn't started.");
                        // Still open the dialog so settings can be reviewed offline,
                        // matching Ext2Mgr which shows the STOP box then continues.
                    }
                    let mut available = crate::mount::ops::free_drive_letters();
                    for letter in &volume.letters {
                        if !available.contains(letter) {
                            available.insert(0, *letter);
                        }
                    }
                    let fixed = crate::mount::persist::query_registry_letter_for_device(
                        &volume.physical_object,
                    );
                    self.dialog = Some(Dialog::Ext2Attrs {
                        volume_index,
                        readonly: false,
                        codepage: if volume.codepage.is_empty() {
                            "utf8".to_string()
                        } else {
                            volume.codepage.clone()
                        },
                        fixmount: fixed.is_some(),
                        automount: volume.uuid.is_some() && fixed.is_none(),
                        letter: fixed.or_else(|| volume.letters.first().copied())
                            .or_else(|| available.first().copied()),
                        available,
                        hiding_prefix: String::new(),
                        hiding_suffix: String::new(),
                        uid: "----".to_string(),
                        gid: "----".to_string(),
                        euid: "----".to_string(),
                    });
                }
                #[cfg(not(windows))]
                {
                    self.status = "Ext2 Management is Windows-only.".to_string();
                }
            }
            Message::Ext2Readonly(value) => {
                if let Some(Dialog::Ext2Attrs { readonly, .. }) = self.dialog.as_mut() {
                    *readonly = value;
                }
            }
            Message::Ext2Codepage(value) => {
                if let Some(Dialog::Ext2Attrs { codepage, .. }) = self.dialog.as_mut() {
                    *codepage = value;
                }
            }
            Message::Ext2Fixmount(value) => {
                if let Some(Dialog::Ext2Attrs {
                    fixmount,
                    automount,
                    ..
                }) = self.dialog.as_mut()
                {
                    *fixmount = value;
                    if value {
                        *automount = false;
                    }
                }
            }
            Message::Ext2Automount(value) => {
                if let Some(Dialog::Ext2Attrs {
                    fixmount,
                    automount,
                    ..
                }) = self.dialog.as_mut()
                {
                    *automount = value;
                    if value {
                        *fixmount = false;
                    }
                }
            }
            Message::Ext2Letter(letter) => {
                if let Some(Dialog::Ext2Attrs {
                    letter: selected, ..
                }) = self.dialog.as_mut()
                {
                    *selected = Some(letter);
                }
            }
            Message::SaveExt2Attrs => {
                #[cfg(windows)]
                {
                    let Some(Dialog::Ext2Attrs {
                        volume_index,
                        readonly,
                        codepage,
                        fixmount,
                        automount,
                        letter,
                        ..
                    }) = self.dialog.clone()
                    else {
                        return Task::none();
                    };
                    let Some(volume) = self.volumes.get(volume_index).cloned() else {
                        return Task::none();
                    };
                    let Some(letter) = letter else {
                        self.status = "Choose a drive letter.".to_string();
                        return Task::none();
                    };
                    if fixmount {
                        let device = crate::mount::ops::dos_device_target(
                            &volume.physical_object,
                            &volume.symlink,
                        );
                        if let Err(err) =
                            crate::mount::persist::set_registry_mount_point(letter, &device)
                        {
                            self.report_warning(format!(
                                "Failed to save the Ext2 settings !\n{err}"
                            ));
                            return Task::none();
                        }
                        let request = crate::mount::ops::MountRequest {
                            letter,
                            mode: crate::mount::ops::MountMode::Temporary,
                            symlink: device,
                            win32_volume_name: volume.win32_volume_name,
                            uuid: volume.uuid,
                            codepage: codepage.clone(),
                            readonly,
                        };
                        match crate::mount::ops::mount_volume(&request) {
                            Ok(message) => {
                                self.status = format!(
                                    "{message} Fixed mount also written to Session Manager."
                                );
                                self.dialog = None;
                                return self.reload();
                            }
                            Err(err) => {
                                self.report_warning(format!(
                                    "Failed to save the Ext2 settings !\n{err}"
                                ));
                            }
                        }
                    } else if automount {
                        let request = crate::mount::ops::MountRequest {
                            letter,
                            mode: crate::mount::ops::MountMode::Ext2Automount,
                            symlink: crate::mount::ops::dos_device_target(
                                &volume.physical_object,
                                &volume.symlink,
                            ),
                            win32_volume_name: volume.win32_volume_name,
                            uuid: volume.uuid,
                            codepage,
                            readonly,
                        };
                        match crate::mount::ops::mount_volume(&request) {
                            Ok(message) => {
                                self.status = message;
                                self.dialog = None;
                                return self.reload();
                            }
                            Err(err) => {
                                self.report_warning(format!(
                                    "Failed to save the Ext2 settings !\n{err}"
                                ));
                            }
                        }
                    } else {
                        self.report_warning(
                            "Enable Fixed mount or Ext2Fsd automount before Save.",
                        );
                    }
                }
            }
            Message::OpenDeadLetters => {
                return self.scan_dead_letters_task();
            }
            Message::DeadLettersLoaded(entries) => {
                self.dialog = Some(Dialog::DeadLetters {
                    entries,
                    selected: None,
                    also_remove_permanent: false,
                    pending_remove: None,
                });
                self.status = "Ready".to_string();
            }
            Message::SelectDeadLetter(index) => {
                if let Some(Dialog::DeadLetters { selected, .. }) = self.dialog.as_mut() {
                    *selected = Some(index);
                }
            }
            Message::DeadAlsoRemovePermanent(value) => {
                if let Some(Dialog::DeadLetters {
                    also_remove_permanent,
                    ..
                }) = self.dialog.as_mut()
                {
                    *also_remove_permanent = value;
                }
            }
            Message::RemoveSelectedDeadLetter => {
                let Some(Dialog::DeadLetters {
                    entries,
                    selected,
                    ..
                }) = &self.dialog
                else {
                    return Task::none();
                };
                let Some(index) = *selected else {
                    self.status = "Select a dead letter first.".to_string();
                    return Task::none();
                };
                let Some(entry) = entries.get(index).cloned() else {
                    return Task::none();
                };
                if let Some(Dialog::DeadLetters {
                    pending_remove, ..
                }) = self.dialog.as_mut()
                {
                    *pending_remove = Some(entry);
                }
            }
            Message::CancelDeadLetterRemove => {
                if let Some(Dialog::DeadLetters {
                    pending_remove, ..
                }) = self.dialog.as_mut()
                {
                    *pending_remove = None;
                }
            }
            Message::ConfirmDeadLetterRemove => {
                let Some(Dialog::DeadLetters {
                    pending_remove,
                    also_remove_permanent,
                    ..
                }) = self.dialog.clone()
                else {
                    return Task::none();
                };
                let Some(entry) = pending_remove else {
                    return Task::none();
                };
                match crate::mount::dead_letters::remove_dead_letter(
                    entry.letter,
                    &entry.symlink,
                    also_remove_permanent,
                ) {
                    Ok(()) => {
                        self.status = format!("Removed dead letter {}:", entry.letter);
                        self.reload_lists();
                        return self.scan_dead_letters_task();
                    }
                    Err(err) => {
                        if let Some(Dialog::DeadLetters {
                            pending_remove, ..
                        }) = self.dialog.as_mut()
                        {
                            *pending_remove = None;
                        }
                        self.report_error(err);
                    }
                }
            }
        }
        Task::none()
    }
}
