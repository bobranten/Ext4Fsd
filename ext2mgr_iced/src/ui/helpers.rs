impl Ext2MgrApp {
    /// Extra space after the last column so its `|` resize grip stays clickable.
    const TABLE_TRAILING_PAD: f32 = 7.0;
    /// `Direction::Both` floats the H-scrollbar over content; reserve its height
    /// (iced default scroller 10px) plus a little so the last row clears it.
    const TABLE_H_SCROLLBAR_PAD: f32 = 14.0;

    fn reload_lists(&mut self) {
        #[cfg(windows)]
        {
            let _ = crate::mount::persist::scrub_partition_path_session_letters();
        }
        let (disks, volumes, disk_rows) = crate::disk::enum_disk::enumerate_all();
        self.disks = disks;
        self.volumes = volumes;
        self.disk_rows = disk_rows;
        self.autofit_column_widths();
        self.recompute_detail();
    }

    fn reload(&mut self) -> Task<Message> {
        self.reload_lists();
        Task::none()
    }

    /// Ext2Mgr `Ext2ProcessExt2Volumes` after load/refresh.
    fn automount_task(&self) -> Task<Message> {
        let volumes = self.volumes.clone();
        Task::perform(
            async move {
                tokio::task::spawn_blocking(move || {
                    crate::mount::ops::process_pending_automounts(&volumes)
                })
                .await
                .unwrap_or_default()
            },
            Message::AutomountFinished,
        )
    }

    /// Status bar + modal MessageBox (Ext2Mgr AfxMessageBox STOP).
    #[cfg(windows)]
    fn report_error(&mut self, message: impl Into<String>) {
        let message = message.into();
        self.status = message.clone();
        crate::win::msgbox::error(&message);
    }

    /// Status bar + modal MessageBox (WARNING).
    #[cfg(windows)]
    fn report_warning(&mut self, message: impl Into<String>) {
        let message = message.into();
        self.status = message.clone();
        crate::win::msgbox::warning(&message);
    }

    fn selected_volume_index(&self) -> Option<usize> {
        match self.selection {
            Selection::Volume(index) => Some(index),
            Selection::Partition { disk, part } => self
                .disks
                .get(disk)
                .and_then(|entry| entry.partitions.get(part))
                .and_then(|partition| partition.volume_index),
            _ => None,
        }
    }

    /// Drive letters for the current volume/partition selection (supports multi-mount).
    fn selection_letters(&self) -> Vec<char> {
        match self.selection {
            Selection::Volume(index) => self
                .volumes
                .get(index)
                .map(|volume| volume.letters.clone())
                .unwrap_or_default(),
            Selection::Partition { disk, part } => self
                .disks
                .get(disk)
                .and_then(|entry| entry.partitions.get(part))
                .map(|partition| partition.letters.clone())
                .unwrap_or_default(),
            Selection::Disk(_) | Selection::None => Vec::new(),
        }
    }

    fn open_mount_points_dialog(&mut self) {
        match self.selection {
            Selection::Volume(index) => {
                let selected = self
                    .volumes
                    .get(index)
                    .and_then(|volume| {
                        if volume.letters.is_empty() {
                            None
                        } else {
                            Some(0)
                        }
                    });
                self.dialog = Some(Dialog::MountPoints {
                    volume_index: Some(index),
                    disk_part: None,
                    selected,
                });
            }
            Selection::Partition { disk, part } => {
                let partition = self
                    .disks
                    .get(disk)
                    .and_then(|entry| entry.partitions.get(part));
                let Some(partition) = partition else {
                    self.status = "Select a volume or partition first.".to_string();
                    return;
                };
                let selected = if partition.letters.is_empty() {
                    None
                } else {
                    Some(0)
                };
                self.dialog = Some(Dialog::MountPoints {
                    volume_index: partition.volume_index,
                    disk_part: Some((disk, part)),
                    selected,
                });
            }
            Selection::Disk(_) | Selection::None => {
                self.status = "Select a volume or partition first.".to_string();
            }
        }
    }

    /// True when Tools → Assign Drive Letter should be available.
    /// Ext2Mgr mounts via Ext2Fsd/Ext2Srv — only EXT volumes, and only when unlettered.
    fn selection_can_assign_letter(&self) -> bool {
        match self.selection {
            Selection::Volume(index) => self.volumes.get(index).is_some_and(|volume| {
                volume.letters.is_empty() && crate::mount::ops::is_ext_family(&volume.filesystem)
            }),
            Selection::Partition { disk, part } => self
                .disks
                .get(disk)
                .and_then(|entry| entry.partitions.get(part))
                .is_some_and(|partition| {
                    partition.letters.is_empty()
                        && crate::mount::ops::is_ext_family(&partition.filesystem)
                }),
            Selection::Disk(_) | Selection::None => false,
        }
    }

    /// True when Unmount should be available (selection has at least one letter).
    fn selection_can_unmount(&self) -> bool {
        match self.selection {
            Selection::Volume(index) => self
                .volumes
                .get(index)
                .is_some_and(|volume| !volume.letters.is_empty()),
            Selection::Partition { disk, part } => self
                .disks
                .get(disk)
                .and_then(|entry| entry.partitions.get(part))
                .is_some_and(|partition| !partition.letters.is_empty()),
            Selection::Disk(_) | Selection::None => false,
        }
    }

    fn disk_row_index_for_selection(&self) -> Option<usize> {
        match self.selection {
            Selection::Disk(disk_index) => self.disk_rows.iter().position(|row| {
                matches!(row, DiskRow::DiskHeader { disk_index: candidate } if *candidate == disk_index)
            }),
            Selection::Partition { disk, part } => self.disk_rows.iter().position(|row| {
                matches!(
                    row,
                    DiskRow::Partition {
                        disk_index,
                        partition_index
                    } if *disk_index == disk && *partition_index == part
                )
            }),
            _ => None,
        }
    }

    fn apply_disk_row_selection(&mut self, row_index: usize) {
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
                DiskRow::Spacer => return,
            };
            self.recompute_detail();
        }
    }

    fn select_prev_entry(&mut self) {
        if self.dialog.is_some() {
            return;
        }
        match self.selection {
            Selection::Volume(index) => {
                if index > 0 {
                    self.selection = Selection::Volume(index - 1);
                    self.recompute_detail();
                }
            }
            Selection::Disk(_) | Selection::Partition { .. } => {
                let Some(current) = self.disk_row_index_for_selection() else {
                    return;
                };
                let mut candidate = current;
                while candidate > 0 {
                    candidate -= 1;
                    if !matches!(self.disk_rows.get(candidate), Some(DiskRow::Spacer)) {
                        self.apply_disk_row_selection(candidate);
                        return;
                    }
                }
            }
            Selection::None => {
                if let Some(last) = self.volumes.len().checked_sub(1) {
                    self.selection = Selection::Volume(last);
                    self.recompute_detail();
                }
            }
        }
    }

    fn select_next_entry(&mut self) {
        if self.dialog.is_some() {
            return;
        }
        match self.selection {
            Selection::Volume(index) => {
                if index + 1 < self.volumes.len() {
                    self.selection = Selection::Volume(index + 1);
                    self.recompute_detail();
                }
            }
            Selection::Disk(_) | Selection::Partition { .. } => {
                let Some(current) = self.disk_row_index_for_selection() else {
                    return;
                };
                let mut candidate = current + 1;
                while candidate < self.disk_rows.len() {
                    if !matches!(self.disk_rows.get(candidate), Some(DiskRow::Spacer)) {
                        self.apply_disk_row_selection(candidate);
                        return;
                    }
                    candidate += 1;
                }
            }
            Selection::None => {
                if !self.volumes.is_empty() {
                    self.selection = Selection::Volume(0);
                    self.recompute_detail();
                } else if !self.disk_rows.is_empty() {
                    let first = self
                        .disk_rows
                        .iter()
                        .position(|row| !matches!(row, DiskRow::Spacer));
                    if let Some(row_index) = first {
                        self.apply_disk_row_selection(row_index);
                    }
                }
            }
        }
    }

    fn format_size(&self, bytes: Option<u64>) -> String {
        crate::disk::enum_disk::format_size(bytes, self.size_units, self.display_bits)
    }

    fn format_free_bytes(&self, total: Option<u64>, used: Option<u64>) -> String {
        match (total, used) {
            (Some(total_bytes), Some(used_bytes)) => {
                self.format_size(Some(total_bytes.saturating_sub(used_bytes)))
            }
            _ => String::new(),
        }
    }

    fn refresh_size_display(&mut self) {
        #[cfg(windows)]
        {
            crate::win::chrome::sync_view_menu(
                self.show_properties_pane,
                matches!(self.size_units, SizeUnitStyle::Binary),
                self.display_bits,
            );
        }
        self.autofit_column_widths();
        self.recompute_detail();
    }

    fn set_size_units(&mut self, style: SizeUnitStyle) {
        if self.size_units == style {
            return;
        }
        self.size_units = style;
        self.refresh_size_display();
    }

    fn set_display_bits(&mut self, display_bits: bool) {
        if self.display_bits == display_bits {
            return;
        }
        self.display_bits = display_bits;
        self.refresh_size_display();
    }

    /// Approximate rendered width of a list cell label (size-12 text + padding).
    fn measure_text_width(label: &str) -> f32 {
        // Segoe UI ~12px is closer to ~5.5–6px average glyph width than monospace.
        const CHAR_PX: f32 = 5.6;
        const PAD_X: f32 = 8.0;
        PAD_X + label.chars().count() as f32 * CHAR_PX
    }

    fn grow_col(width: &mut f32, label: &str, min: f32) {
        *width = (*width)
            .max(min)
            .max(Self::measure_text_width(label));
    }

    /// Size each column to the widest header or row value (no Fill stretch).
    /// Header `|` grips sit at the right edge of each cell; reserve grip width once
    /// so autofit does not fight the handle.
    fn autofit_column_widths(&mut self) {
        const GRIP: f32 = 10.0;
        let mut volume = [24.0_f32, 36.0, 36.0, 56.0, 48.0, 48.0, 48.0, 64.0];
        Self::grow_col(&mut volume[1], "Volume", 36.0);
        Self::grow_col(&mut volume[2], "Type", 36.0);
        Self::grow_col(&mut volume[3], "File system", 56.0);
        Self::grow_col(&mut volume[4], "Total size", 48.0);
        // Used size / Codepage: size to cell values only; header may ellipsize.
        Self::grow_col(&mut volume[7], "Physical object", 64.0);
        for volume_entry in &self.volumes {
            Self::grow_col(
                &mut volume[1],
                &format_letters(&volume_entry.letters),
                36.0,
            );
            Self::grow_col(&mut volume[2], &volume_entry.volume_kind, 36.0);
            Self::grow_col(&mut volume[3], &volume_entry.filesystem, 56.0);
            Self::grow_col(
                &mut volume[4],
                &self.format_size(volume_entry.total_bytes),
                48.0,
            );
            Self::grow_col(
                &mut volume[5],
                &self.format_size(volume_entry.used_bytes),
                48.0,
            );
            Self::grow_col(&mut volume[6], &volume_entry.codepage, 48.0);
            Self::grow_col(&mut volume[7], &volume_entry.physical_object, 64.0);
        }
        // Icon column stays fixed; other columns reserve room for the `|` grip.
        for slot in volume.iter_mut().skip(1) {
            *slot += GRIP;
        }
        self.volume_col_widths = volume;

        let mut disk = [36.0_f32, 36.0, 56.0, 48.0, 48.0, 48.0, 72.0];
        Self::grow_col(&mut disk[1], "Type", 36.0);
        Self::grow_col(&mut disk[2], "File system", 56.0);
        Self::grow_col(&mut disk[3], "Total size", 48.0);
        // Used size / Codepage: size to cell values only; header may ellipsize.
        Self::grow_col(&mut disk[6], "Partition type", 72.0);
        for disk_entry in &self.disks {
            // Col 0: "DISK N" or indented "(X:)" / "(Y:,Z:)" — no oversized floor.
            Self::grow_col(&mut disk[0], &disk_entry.display_name, 36.0);
            Self::grow_col(&mut disk[1], &disk_entry.style, 36.0);
            Self::grow_col(
                &mut disk[3],
                &self.format_size(disk_entry.total_bytes),
                48.0,
            );
            for partition in &disk_entry.partitions {
                Self::grow_col(
                    &mut disk[0],
                    &format!("  {}", format_letters(&partition.letters)),
                    36.0,
                );
                Self::grow_col(&mut disk[1], &partition.style, 36.0);
                Self::grow_col(&mut disk[2], &partition.filesystem, 56.0);
                Self::grow_col(
                    &mut disk[3],
                    &self.format_size(partition.total_bytes),
                    48.0,
                );
                Self::grow_col(
                    &mut disk[4],
                    &self.format_size(partition.used_bytes),
                    48.0,
                );
                Self::grow_col(&mut disk[5], &partition.codepage, 48.0);
                Self::grow_col(&mut disk[6], &partition.partition_type, 72.0);
            }
        }
        for slot in &mut disk {
            *slot += GRIP;
        }
        self.disk_col_widths = disk;
    }

    fn volume_table_width(&self) -> f32 {
        self.volume_col_widths.iter().sum::<f32>() + Self::TABLE_TRAILING_PAD
    }

    fn disk_table_width(&self) -> f32 {
        self.disk_col_widths.iter().sum::<f32>() + Self::TABLE_TRAILING_PAD
    }

    /// Smallest content width that fits both tables without horizontal clipping.
    fn content_min_width(&self) -> f32 {
        const MAIN_PAD: f32 = 16.0;
        self.volume_table_width()
            .max(self.disk_table_width())
            + MAIN_PAD
    }

    fn fit_window_task(&self) -> Task<Message> {
        let size = Size::new(self.content_min_width().max(480.0), 720.0);
        window::latest().then(move |window_id| match window_id {
            Some(id) => window::resize(id, size),
            None => Task::none(),
        })
    }

    fn sync_native_menus(&self) {
        #[cfg(windows)]
        {
            if self.chrome_ready {
                crate::win::chrome::sync_mount_letter_menus(
                    self.selection_can_assign_letter(),
                    self.selection_can_unmount(),
                );
                crate::win::chrome::sync_view_menu(
                    self.show_properties_pane,
                    matches!(self.size_units, SizeUnitStyle::Binary),
                    self.display_bits,
                );
            }
        }
    }

    fn uuid_detail_suffix(uuid: &str) -> String {
        if uuid.is_empty() {
            String::new()
        } else {
            format!("\nUUID: {uuid}")
        }
    }

    /// Shared Capacity/Used/Free/UUID/Codepage block for the bottom properties pane.
    fn volume_like_detail_body(
        &self,
        headline: &str,
        total_bytes: Option<u64>,
        used_bytes: Option<u64>,
        codepage: &str,
        uuid: &str,
    ) -> String {
        format!(
            "{headline}\nCapacity: {}\nUsed: {}\nFree: {}{}\nCodepage: {}",
            self.format_size(total_bytes),
            self.format_size(used_bytes),
            self.format_free_bytes(total_bytes, used_bytes),
            Self::uuid_detail_suffix(uuid),
            codepage,
        )
    }

    fn recompute_detail(&mut self) {
        let (status_line, detail) = match self.selection {
            Selection::None => ("Ready".to_string(), "Ready".to_string()),
            Selection::Volume(index) => {
                if let Some(volume) = self.volumes.get(index) {
                    let device = if volume.physical_object.is_empty() {
                        "(no NT device name)"
                    } else {
                        volume.physical_object.as_str()
                    };
                    // Filesystem last on the headline (no separate Filesystem / NT rows).
                    let status = format!(
                        "VOLUME: {} {} {}",
                        format_letters(&volume.letters),
                        device,
                        volume.filesystem,
                    );
                    let uuid = display_volume_guid(&volume.win32_volume_name, volume.uuid);
                    let detail = self.volume_like_detail_body(
                        &status,
                        volume.total_bytes,
                        volume.used_bytes,
                        &volume.codepage,
                        &uuid,
                    );
                    (status, detail)
                } else {
                    ("Ready".to_string(), "Ready".to_string())
                }
            }
            Selection::Disk(index) => {
                if let Some(disk) = self.disks.get(index) {
                    let status = format!("DISK {}: {}", disk.index, disk.device_path);
                    let detail = format!(
                        "{status}\nStyle: {}\nCapacity: {}\nPartitions: {}",
                        disk.style,
                        self.format_size(disk.total_bytes),
                        disk.partitions.len(),
                    );
                    (status, detail)
                } else {
                    ("Ready".to_string(), "Ready".to_string())
                }
            }
            Selection::Partition { disk, part } => {
                if let Some(partition) = self
                    .disks
                    .get(disk)
                    .and_then(|entry| entry.partitions.get(part))
                {
                    // Filesystem last on the headline (no separate Filesystem / NT rows).
                    let status = format!(
                        "DISK {} PARTITION {}: {} {}",
                        self.disks[disk].index,
                        partition.number,
                        format_letters(&partition.letters),
                        partition.filesystem,
                    );
                    let partition_uuid = partition
                        .volume_index
                        .and_then(|volume_index| self.volumes.get(volume_index))
                        .and_then(|volume| volume.uuid);
                    let uuid = display_volume_guid(&partition.win32_volume_name, partition_uuid);
                    let detail = self.volume_like_detail_body(
                        &status,
                        partition.total_bytes,
                        partition.used_bytes,
                        &partition.codepage,
                        &uuid,
                    );
                    (status, detail)
                } else {
                    ("Ready".to_string(), "Ready".to_string())
                }
            }
        };
        self.status = status_line;
        self.detail_text = detail.clone();
        self.detail_editor = text_editor::Content::with_text(&detail);
        self.sync_native_menus();
    }


    fn raw_bytes_string(bytes: Option<u64>) -> String {
        bytes.map(|value| value.to_string()).unwrap_or_default()
    }

    fn raw_free_bytes_string(total: Option<u64>, used: Option<u64>) -> String {
        match (total, used) {
            (Some(total_bytes), Some(used_bytes)) => {
                total_bytes.saturating_sub(used_bytes).to_string()
            }
            _ => String::new(),
        }
    }

    /// Snapshot for the Properties dialog (IDD_PROPERTY_DIALOG field set).
    fn properties_snapshot(&self) -> PropertiesSnapshot {
        let empty = PropertiesSnapshot::default();
        match self.selection {
            Selection::None => empty,
            Selection::Volume(index) => {
                let Some(volume) = self.volumes.get(index) else {
                    return empty;
                };
                // Match SetVolume: single-extent volumes with a matching partition
                // are shown as that partition + disk.
                if volume.extents.len() == 1 {
                    let extent = &volume.extents[0];
                    if let Some((disk_index, partition_index)) =
                        self.find_partition_for_extent(extent.disk_number, extent.starting_offset)
                    {
                        return self.properties_for_partition(disk_index, partition_index);
                    }
                }
                PropertiesSnapshot {
                    sdev_title: "Volume".to_string(),
                    status: Self::volume_status_line(volume),
                    mount_points: format_letters(&volume.letters),
                    filesystem: volume.filesystem.clone(),
                    capacity_bytes: Self::raw_bytes_string(volume.total_bytes),
                    free_bytes: Self::raw_free_bytes_string(volume.total_bytes, volume.used_bytes),
                    can_change_mp: true,
                    can_mount: volume.letters.is_empty()
                        && Self::is_ext_fs(&volume.filesystem),
                    can_unmount: !volume.letters.is_empty(),
                    can_ext2: Self::is_ext_fs(&volume.filesystem),
                    ..empty
                }
            }
            Selection::Disk(index) => {
                let Some(disk) = self.disks.get(index) else {
                    return empty;
                };
                let mut snap = self.properties_disk_fields(disk);
                // Original auto-selects first partition when opening a disk.
                if !disk.partitions.is_empty() {
                    snap.apply_sdev(&self.properties_for_partition(index, 0));
                }
                snap
            }
            Selection::Partition { disk, part } => self.properties_for_partition(disk, part),
        }
    }

    fn find_partition_for_extent(
        &self,
        disk_number: u32,
        starting_offset: u64,
    ) -> Option<(usize, usize)> {
        let disk_index = self
            .disks
            .iter()
            .position(|disk| disk.index == disk_number)?;
        let partition_index = self.disks[disk_index].partitions.iter().position(|partition| {
            partition.starting_offset == starting_offset
        })?;
        Some((disk_index, partition_index))
    }

    fn is_ext_fs(filesystem: &str) -> bool {
        let upper = filesystem.to_ascii_uppercase();
        upper.starts_with("EXT2") || upper.starts_with("EXT3") || upper.starts_with("EXT4")
    }

    fn volume_status_line(volume: &VolumeEntry) -> String {
        let mut status = String::from("Online");
        if Self::is_ext_fs(&volume.filesystem) && !volume.codepage.is_empty() {
            status.push_str(",codepage:");
            status.push_str(&volume.codepage);
        }
        status
    }

    fn properties_disk_fields(&self, disk: &DiskEntry) -> PropertiesSnapshot {
        let device_type = if disk.removable {
            "Removable".to_string()
        } else if disk.style.eq_ignore_ascii_case("GPT") {
            "GUID".to_string()
        } else if disk.style.is_empty() {
            "RAW".to_string()
        } else {
            disk.style.clone()
        };
        PropertiesSnapshot {
            disk_title: disk.display_name.clone(),
            vendor: disk.vendor.clone(),
            product: disk.product.clone(),
            serial: disk.serial.clone(),
            bus_type: disk.bus_type.clone(),
            device_type,
            media_type: disk.media_type.clone(),
            disk_capacity_bytes: Self::raw_bytes_string(disk.total_bytes),
            ..PropertiesSnapshot::default()
        }
    }

    fn properties_for_partition(&self, disk_index: usize, part_index: usize) -> PropertiesSnapshot {
        let Some(disk) = self.disks.get(disk_index) else {
            return PropertiesSnapshot::default();
        };
        let Some(partition) = disk.partitions.get(part_index) else {
            return PropertiesSnapshot::default();
        };
        let mut snap = self.properties_disk_fields(disk);
        snap.sdev_title = format!("PARTITION {}", partition.number);
        snap.mount_points = format_letters(&partition.letters);
        snap.filesystem = partition.filesystem.clone();
        snap.can_change_mp = true;
        snap.can_mount =
            partition.letters.is_empty() && Self::is_ext_fs(&partition.filesystem);
        snap.can_unmount = !partition.letters.is_empty();
        snap.can_ext2 = Self::is_ext_fs(&partition.filesystem);

        let mut status = String::from("Online");
        if !partition.partition_type.is_empty() {
            status.push(',');
            status.push_str(&partition.partition_type);
        } else if !partition.style.is_empty() {
            status.push(',');
            status.push_str(&partition.style);
        }
        if snap.can_ext2 && !partition.codepage.is_empty() {
            status.push_str(",codepage:");
            status.push_str(&partition.codepage);
        }
        snap.status = status;

        if let Some(total) = partition.total_bytes {
            snap.capacity_bytes = total.to_string();
            snap.free_bytes = Self::raw_free_bytes_string(Some(total), partition.used_bytes);
            if snap.free_bytes.is_empty() {
                snap.free_bytes = "0".to_string();
            }
        } else {
            snap.capacity_bytes = partition.length.to_string();
            snap.free_bytes = "0".to_string();
        }
        snap
    }

    fn prop_row<'a>(label: &'a str, value: String) -> Element<'a, Message> {
        row![
            text(label).size(12).width(Length::Fixed(110.0)),
            text(value).size(12).width(Length::Fill),
        ]
        .spacing(6)
        .into()
    }

    fn now_clock() -> String {
        #[cfg(windows)]
        {
            let mut local = windows_sys::Win32::Foundation::SYSTEMTIME {
                wYear: 0,
                wMonth: 0,
                wDayOfWeek: 0,
                wDay: 0,
                wHour: 0,
                wMinute: 0,
                wSecond: 0,
                wMilliseconds: 0,
            };
            unsafe {
                windows_sys::Win32::System::SystemInformation::GetLocalTime(&mut local);
            }
            let months = [
                "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov",
                "Dec",
            ];
            let month = months
                .get(local.wMonth.saturating_sub(1) as usize)
                .copied()
                .unwrap_or("???");
            format!(
                "{} {:02},{:04} {:02}:{:02}:{:02}",
                month, local.wDay, local.wYear, local.wHour, local.wMinute, local.wSecond
            )
        }
        #[cfg(not(windows))]
        {
            String::new()
        }
    }

    fn scan_dead_letters_task(&mut self) -> Task<Message> {
        let volumes = self.volumes.clone();
        self.status = "Scanning for dead letters…".to_string();
        Task::perform(
            async move {
                tokio::task::spawn_blocking(move || crate::mount::dead_letters::find_dead_letters(&volumes))
                    .await
                    .unwrap_or_default()
            },
            Message::DeadLettersLoaded,
        )
    }

    fn quick_mount(&mut self) -> Task<Message> {
        #[cfg(windows)]
        {
            let Some(volume_index) = self.selected_volume_index() else {
                self.status = "Select a volume or partition to mount.".to_string();
                return Task::none();
            };
            let Some(volume) = self.volumes.get(volume_index).cloned() else {
                return Task::none();
            };
            if !volume.letters.is_empty() {
                self.status = format!(
                    "Already mounted as {}",
                    format_letters(&volume.letters)
                );
                return Task::none();
            }
            if !crate::mount::ops::is_ext_family(&volume.filesystem) {
                self.report_warning(
                    "Assign Drive Letter is only available for EXT2/3/4 volumes.",
                );
                return Task::none();
            }
            let Some(letter) = crate::mount::ops::first_free_drive_letter() else {
                self.status = "No free drive letter (C:-Z:).".to_string();
                return Task::none();
            };
            let request = crate::mount::ops::MountRequest {
                letter,
                mode: crate::mount::ops::MountMode::Temporary,
                symlink: crate::mount::ops::dos_device_target(
                    &volume.physical_object,
                    &volume.symlink,
                ),
                win32_volume_name: volume.win32_volume_name,
                uuid: volume.uuid,
                codepage: if volume.codepage.is_empty() {
                    "utf8".to_string()
                } else {
                    volume.codepage
                },
                readonly: false,
            };
            self.status = format!("Mounting {letter}:…");
            return Self::mount_volume_task(request);
        }
        #[cfg(not(windows))]
        {
            self.status = "Mount is Windows-only.".to_string();
            Task::none()
        }
    }

    #[cfg(windows)]
    fn mount_volume_task(request: crate::mount::ops::MountRequest) -> Task<Message> {
        let notify_letter = request.letter;
        Task::perform(
            async move {
                match tokio::task::spawn_blocking(move || crate::mount::ops::mount_volume(&request)).await {
                    Ok(Ok(message)) => (Some(message), None, false, true),
                    Ok(Err(err)) => {
                        let is_error = err.contains("not started");
                        (None, Some(err), is_error, false)
                    }
                    Err(join_err) => (None, Some(join_err.to_string()), true, false),
                }
            },
            move |(ok_message, err_message, is_error, notify)| Message::PipeOpFinished {
                ok_message,
                err_message,
                is_error,
                restore_mount_points: None,
                explorer_notify: if notify {
                    vec![(notify_letter, true)]
                } else {
                    Vec::new()
                },
                // Never auto-assign other volumes after a user-initiated mount.
                run_automount: false,
            },
        )
    }

    #[cfg(windows)]
    fn unmount_letters_task(
        letters: Vec<char>,
        symlink: String,
        win32_volume_name: String,
    ) -> Task<Message> {
        Task::perform(
            async move {
                match tokio::task::spawn_blocking(move || {
                    let mut removed = Vec::new();
                    let mut last_error = None;
                    for letter in letters {
                        match crate::mount::ops::unmount_letter(
                            letter,
                            &symlink,
                            &win32_volume_name,
                        ) {
                            Ok(()) => removed.push(letter),
                            Err(err) => last_error = Some(err),
                        }
                    }
                    if removed.is_empty() {
                        Err(last_error.unwrap_or_else(|| "Unmount failed.".to_string()))
                    } else {
                        let list = removed
                            .iter()
                            .map(|letter| format!("{letter}:"))
                            .collect::<Vec<_>>()
                            .join(", ");
                        Ok((format!("Unmounted {list}"), removed))
                    }
                })
                .await
                {
                    Ok(Ok((message, removed))) => (
                        Some(message),
                        None,
                        false,
                        removed.into_iter().map(|letter| (letter, false)).collect(),
                    ),
                    Ok(Err(err)) => {
                        let is_error = err.contains("not started");
                        (None, Some(err), is_error, Vec::new())
                    }
                    Err(join_err) => (None, Some(join_err.to_string()), true, Vec::new()),
                }
            },
            |(ok_message, err_message, is_error, explorer_notify)| Message::PipeOpFinished {
                ok_message,
                err_message,
                is_error,
                restore_mount_points: None,
                explorer_notify,
                run_automount: false,
            },
        )
    }

    fn unmount_selected(&mut self) -> Task<Message> {
        #[cfg(windows)]
        {
            let letters = self.selection_letters();
            if letters.is_empty() {
                self.status = "Selection has no drive letter.".to_string();
                return Task::none();
            }
            let (symlink, win32_volume_name) = if let Some(volume_index) =
                self.selected_volume_index()
            {
                self.volumes
                    .get(volume_index)
                    .map(|volume| {
                        (
                            crate::mount::ops::dos_device_target(
                                &volume.physical_object,
                                &volume.symlink,
                            ),
                            volume.win32_volume_name.clone(),
                        )
                    })
                    .unwrap_or_default()
            } else if let Selection::Partition { disk, part } = self.selection {
                self.disks
                    .get(disk)
                    .and_then(|entry| entry.partitions.get(part))
                    .map(|partition| {
                        let linked = partition
                            .volume_index
                            .and_then(|index| self.volumes.get(index));
                        let physical = linked
                            .map(|volume| volume.physical_object.as_str())
                            .unwrap_or("");
                        let volume_symlink = linked
                            .map(|volume| volume.symlink.as_str())
                            .unwrap_or(partition.symlink.as_str());
                        (
                            crate::mount::ops::dos_device_target(physical, volume_symlink),
                            linked
                                .map(|volume| volume.win32_volume_name.clone())
                                .unwrap_or_else(|| partition.win32_volume_name.clone()),
                        )
                    })
                    .unwrap_or_default()
            } else {
                (String::new(), String::new())
            };
            self.status = "Unmounting…".to_string();
            return Self::unmount_letters_task(letters, symlink, win32_volume_name);
        }
        #[cfg(not(windows))]
        {
            self.status = "Unmount is Windows-only.".to_string();
            Task::none()
        }
    }

    fn mount_target_for_letter_ops(
        &self,
    ) -> Option<(String, String, Option<[u8; 16]>, String)> {
        // dos_device_target (\Device\...), win32_volume_name, uuid, codepage
        if let Some(Dialog::MountPoints {
            volume_index,
            disk_part,
            ..
        }) = &self.dialog
        {
            if let Some(index) = *volume_index {
                return self.volumes.get(index).map(|volume| {
                    (
                        crate::mount::ops::dos_device_target(
                            &volume.physical_object,
                            &volume.symlink,
                        ),
                        volume.win32_volume_name.clone(),
                        volume.uuid,
                        volume.codepage.clone(),
                    )
                });
            }
            if let Some((disk, part)) = *disk_part {
                return self
                    .disks
                    .get(disk)
                    .and_then(|entry| entry.partitions.get(part))
                    .map(|partition| {
                        let (physical, win32, uuid, codepage, volume_symlink) = partition
                            .volume_index
                            .and_then(|index| self.volumes.get(index))
                            .map(|volume| {
                                (
                                    volume.physical_object.clone(),
                                    volume.win32_volume_name.clone(),
                                    volume.uuid,
                                    volume.codepage.clone(),
                                    volume.symlink.clone(),
                                )
                            })
                            .unwrap_or_else(|| {
                                (
                                    String::new(),
                                    String::new(),
                                    None,
                                    String::new(),
                                    partition.symlink.clone(),
                                )
                            });
                        let symlink = crate::mount::ops::dos_device_target(
                            &physical,
                            if !volume_symlink.is_empty() {
                                &volume_symlink
                            } else {
                                &partition.symlink
                            },
                        );
                        (symlink, win32, uuid, codepage)
                    });
            }
        }
        if let Some(index) = self.selected_volume_index() {
            return self.volumes.get(index).map(|volume| {
                (
                    crate::mount::ops::dos_device_target(&volume.physical_object, &volume.symlink),
                    volume.win32_volume_name.clone(),
                    volume.uuid,
                    volume.codepage.clone(),
                )
            });
        }
        if let Selection::Partition { disk, part } = self.selection {
            return self
                .disks
                .get(disk)
                .and_then(|entry| entry.partitions.get(part))
                .map(|partition| {
                    let (physical, win32, uuid, codepage, volume_symlink) = partition
                        .volume_index
                        .and_then(|index| self.volumes.get(index))
                        .map(|volume| {
                            (
                                volume.physical_object.clone(),
                                volume.win32_volume_name.clone(),
                                volume.uuid,
                                volume.codepage.clone(),
                                volume.symlink.clone(),
                            )
                        })
                        .unwrap_or_else(|| {
                            (
                                String::new(),
                                String::new(),
                                None,
                                String::new(),
                                partition.symlink.clone(),
                            )
                        });
                    let symlink = crate::mount::ops::dos_device_target(
                        &physical,
                        if !volume_symlink.is_empty() {
                            &volume_symlink
                        } else {
                            &partition.symlink
                        },
                    );
                    (symlink, win32, uuid, codepage)
                });
        }
        None
    }

    fn open_letter_picker(&mut self, replace_letter: Option<char>) {
        #[cfg(windows)]
        {
            let return_mount_points = match &self.dialog {
                Some(Dialog::MountPoints {
                    volume_index,
                    disk_part,
                    ..
                }) => Some((*volume_index, *disk_part)),
                _ => None,
            };
            let Some((mount_symlink, mount_win32, mount_uuid, mount_codepage)) =
                self.mount_target_for_letter_ops()
            else {
                self.status = "Select a volume or partition first.".to_string();
                return;
            };
            let mut available = crate::mount::ops::free_drive_letters();
            if let Some(letter) = replace_letter {
                if !available.contains(&letter) {
                    available.insert(0, letter);
                }
            }
            let selected_letter = replace_letter.or_else(|| available.first().copied());
            let existing_session_letters =
                crate::mount::persist::registry_letters_for_device(&mount_symlink);
            let existing_mountmgr_letters = if mount_win32.is_empty() {
                Vec::new()
            } else {
                let mut letters: Vec<char> = crate::mount::dead_letters::mountmgr_letters_for_volume(&mount_win32)
                    .into_iter()
                    .collect();
                letters.sort_unstable();
                letters
            };
            let mountmgr_available = !mount_win32.is_empty();
            if replace_letter.is_none() && !existing_mountmgr_letters.is_empty() {
                let listed = existing_mountmgr_letters
                    .iter()
                    .map(|entry| format!("{entry}:"))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.report_warning(format!(
                    "This volume already has Mount Manager letter {listed}.\n\
                     Windows allows only one drive letter per volume — use Change or Remove."
                ));
                return;
            }
            let mountmgr_blocked = !mountmgr_available
                || Self::assignment_blocked(&existing_mountmgr_letters, replace_letter);
            let session_blocked =
                Self::session_manager_blocked(&existing_session_letters, replace_letter);
            // Prefer the *current* persistence of the letter being Changed; do not
            // default to Mount Manager / Ext2Automount just because they are "preferred".
            let persist_mode = if let Some(letter) = replace_letter {
                if existing_mountmgr_letters.contains(&letter) {
                    crate::mount::ops::MountMode::MountManager
                } else if existing_session_letters.contains(&letter) {
                    crate::mount::ops::MountMode::PermanentRegistry
                } else if mount_uuid.is_some_and(|uuid| {
                    crate::mount::persist::query_ext2_automount_letter(&uuid) == Some(letter)
                }) {
                    crate::mount::ops::MountMode::Ext2Automount
                } else {
                    crate::mount::ops::MountMode::Temporary
                }
            } else if mountmgr_available && !mountmgr_blocked {
                crate::mount::ops::MountMode::MountManager
            } else if !session_blocked {
                crate::mount::ops::MountMode::PermanentRegistry
            } else {
                crate::mount::ops::MountMode::Temporary
            };
            self.dialog = Some(Dialog::LetterPicker {
                replace_letter,
                selected_letter,
                persist_mode,
                available,
                mount_symlink,
                mount_win32,
                mount_uuid,
                mount_codepage,
                existing_session_letters,
                existing_mountmgr_letters,
                return_mount_points,
            });
        }
        #[cfg(not(windows))]
        {
            let _ = replace_letter;
            self.status = "Letter picker is Windows-only.".to_string();
        }
    }

    fn assignment_blocked(existing_letters: &[char], replace_letter: Option<char>) -> bool {
        existing_letters.iter().any(|letter| {
            replace_letter.map(|replace| replace != *letter).unwrap_or(true)
        })
    }

    fn session_manager_blocked(
        existing_session_letters: &[char],
        replace_letter: Option<char>,
    ) -> bool {
        match replace_letter {
            None => !existing_session_letters.is_empty(),
            Some(letter) if existing_session_letters.contains(&letter) => false,
            Some(_) => !existing_session_letters.is_empty(),
        }
    }

    fn letter_source_label(
        letter: char,
        win32_volume_name: &str,
        device_nt_path: &str,
        mount_uuid: Option<[u8; 16]>,
    ) -> String {
        let in_mountmgr = !win32_volume_name.is_empty()
            && crate::mount::dead_letters::mountmgr_letters_for_volume(win32_volume_name)
                .contains(&letter);
        // Match by letter in Session Manager first — Mount Points device_path used to be
        // `\??\Volume{GUID}` while the registry stores `\Device\HarddiskVolumeN`, so
        // registry_letters_for_device never matched and every SM letter looked Temporary.
        let in_session = crate::mount::persist::registry_device_for_letter(letter).is_some_and(
            |reg_device| {
                device_nt_path.is_empty()
                    || reg_device.eq_ignore_ascii_case(device_nt_path)
                    || crate::mount::ops::query_dos_device_public(letter)
                        .is_some_and(|live| live.eq_ignore_ascii_case(&reg_device))
            },
        );
        let in_automount = !in_mountmgr
            && !in_session
            && mount_uuid.is_some_and(|uuid| {
                crate::mount::persist::query_ext2_automount_letter(&uuid) == Some(letter)
            });
        match (in_mountmgr, in_session, in_automount) {
            (true, _, _) => format!("{letter}:  Mount Manager"),
            (false, true, _) => format!("{letter}:  Session Manager (registry)"),
            (false, false, true) => format!("{letter}:  Ext2Fsd automount"),
            (false, false, false) => format!("{letter}:  Temporary / DOS device"),
        }
    }

    fn confirm_letter_picker(&mut self) -> Task<Message> {
        #[cfg(windows)]
        {
            let Some(Dialog::LetterPicker {
                replace_letter,
                selected_letter,
                persist_mode,
                mount_symlink,
                mount_win32,
                mount_uuid,
                mount_codepage,
                existing_session_letters,
                existing_mountmgr_letters,
                return_mount_points,
                ..
            }) = self.dialog.clone()
            else {
                return Task::none();
            };
            let Some(letter) = selected_letter else {
                self.status = "Choose a drive letter.".to_string();
                return Task::none();
            };
            if mount_symlink.is_empty() {
                self.report_error(
                    "Cannot assign drive letter: volume has no NT device path.\n\
                     Refresh the list, then select the volume or partition again.",
                );
                return Task::none();
            }
            if replace_letter.is_none() && !existing_mountmgr_letters.is_empty() {
                let listed = existing_mountmgr_letters
                    .iter()
                    .map(|entry| format!("{entry}:"))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.report_warning(format!(
                    "This volume already has Mount Manager letter {listed}.\n\
                     Windows allows only one drive letter per volume — use Change or Remove."
                ));
                return Task::none();
            }
            if persist_mode == crate::mount::ops::MountMode::Temporary
                && !existing_mountmgr_letters.is_empty()
                && replace_letter
                    .map(|letter| !existing_mountmgr_letters.contains(&letter))
                    .unwrap_or(true)
            {
                self.report_warning(
                    "Temporary DefineDosDevice cannot add another Explorer letter \
                     beside an existing Mount Manager letter.\n\
                     Windows allows only one drive letter per volume — use Change or Remove.",
                );
                return Task::none();
            }
            if persist_mode == crate::mount::ops::MountMode::Ext2Automount && mount_uuid.is_none() {
                self.report_warning("Ext2Fsd automount needs an EXT volume UUID.");
                return Task::none();
            }
            if persist_mode == crate::mount::ops::MountMode::MountManager && mount_win32.is_empty() {
                self.report_warning(
                    "Mount Manager needs a Win32 volume name (Volume{GUID}).\n\
                     This Ext2 volume is only reachable as a DOS device — use Temporary \
                     or Session Manager.",
                );
                return Task::none();
            }
            if persist_mode == crate::mount::ops::MountMode::MountManager
                && Self::assignment_blocked(&existing_mountmgr_letters, replace_letter)
            {
                let listed = existing_mountmgr_letters
                    .iter()
                    .map(|entry| format!("{entry}:"))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.report_warning(format!(
                    "This volume already has a Mount Manager letter ({listed}).\n\
                     Use Change on that letter, or remove it first — only one Mount Manager \
                     letter is allowed per volume."
                ));
                return Task::none();
            }
            if persist_mode == crate::mount::ops::MountMode::PermanentRegistry
                && Self::session_manager_blocked(&existing_session_letters, replace_letter)
            {
                let listed = existing_session_letters
                    .iter()
                    .map(|entry| format!("{entry}:"))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.report_warning(format!(
                    "This volume already has a Session Manager letter ({listed}).\n\
                     Use Change on that letter, or remove it first — only one Session Manager \
                     letter is allowed per volume."
                ));
                return Task::none();
            }

            let mode = persist_mode;
            let request = crate::mount::ops::MountRequest {
                letter,
                mode,
                symlink: mount_symlink.clone(),
                win32_volume_name: mount_win32.clone(),
                uuid: mount_uuid,
                codepage: if mount_codepage.is_empty() {
                    "utf8".to_string()
                } else {
                    mount_codepage
                },
                readonly: false,
            };
            let restore = return_mount_points.map(|(volume_index, disk_part)| {
                (
                    volume_index,
                    disk_part,
                    letter,
                    mount_win32.clone(),
                    mount_uuid,
                )
            });
            self.status = format!("Mounting {letter}:…");
            self.dialog = None;
            return Task::perform(
                async move {
                    let result = tokio::task::spawn_blocking(move || {
                        let mut notify = Vec::new();
                        if let Some(old_letter) = replace_letter {
                            // Session Manager in-place only: Change Temporary → Session Manager
                            // on the *same* letter writes registry without tear-down. Changing
                            // M: → G: must still unmount M: first.
                            let skip_unmount = mode
                                == crate::mount::ops::MountMode::PermanentRegistry
                                && old_letter == letter;
                            if !skip_unmount {
                                crate::mount::ops::unmount_letter(
                                    old_letter,
                                    &mount_symlink,
                                    &mount_win32,
                                )?;
                                notify.push((old_letter, false));
                            }
                        }
                        crate::mount::ops::mount_volume(&request)?;
                        notify.push((letter, true));
                        Ok::<Vec<(char, bool)>, String>(notify)
                    })
                    .await;
                    match result {
                        Ok(Ok(explorer_notify)) => (
                            Some(format!(
                                "Mounted {letter}: ({mode:?})"
                            )),
                            None,
                            false,
                            restore,
                            explorer_notify,
                        ),
                        Ok(Err(err)) => {
                            let is_error = err.contains("not started");
                            (None, Some(err), is_error, None, Vec::new())
                        }
                        Err(join_err) => {
                            (None, Some(join_err.to_string()), true, None, Vec::new())
                        }
                    }
                },
                |(ok_message, err_message, is_error, restore_mount_points, explorer_notify)| {
                    Message::PipeOpFinished {
                        ok_message,
                        err_message,
                        is_error,
                        restore_mount_points,
                        explorer_notify,
                        run_automount: false,
                    }
                },
            );
        }
        #[cfg(not(windows))]
        {
            self.dialog = None;
            Task::none()
        }
    }

    fn header_cell<'a>(
        label: &'a str,
        width: Length,
        list: ListKind,
        column: usize,
        resizable: bool,
    ) -> Element<'a, Message> {
        // `|` grip inside the cell, flush to the right border (column seam).
        const GRIP_W: f32 = 10.0;
        let display = match width {
            Length::Fixed(pixels) => {
                let label_budget = if resizable {
                    (pixels - GRIP_W).max(0.0)
                } else {
                    pixels
                };
                Self::ellipsize_for_width(label, label_budget)
            }
            _ => label.to_string(),
        };
        // Intrinsic text height — Length::Fill here collapses the header row to 0.
        let label_text = container(text(display).size(12))
            .width(Length::Fill)
            .padding([2, 4]);
        let body: Element<'_, Message> = if resizable {
            let grip = mouse_area(
                container(text("|").size(14))
                    .width(Length::Fixed(GRIP_W))
                    .height(Length::Fixed(18.0))
                    .align_x(alignment::Horizontal::Right)
                    .center_y(Length::Fixed(18.0))
                    .style(|_theme| container::Style {
                        text_color: Some(Color::from_rgb(0.45, 0.45, 0.45)),
                        ..Default::default()
                    }),
            )
            .interaction(mouse::Interaction::ResizingColumn)
            .on_press(Message::StartResize {
                list,
                column,
                cursor_x: 0.0,
            });
            row![label_text, grip].spacing(0).into()
        } else {
            label_text.into()
        };
        container(body)
            .width(width)
            .style(container::bordered_box)
            .into()
    }

    fn cell<'a>(&self, label: String, width: Length, selected: bool) -> Element<'a, Message> {
        let display = match width {
            Length::Fixed(pixels) => Self::ellipsize_for_width(&label, pixels),
            _ => label,
        };
        let content = text(display).size(12);
        let mut box_ = container(content).width(width).padding([2, 4]);
        if selected {
            let background = self.accent;
            let text_color = self.accent_text;
            box_ = box_.style(move |_theme| container::Style {
                text_color: Some(text_color),
                background: Some(Background::Color(background)),
                border: Border::default(),
                shadow: Shadow::default(),
                ..Default::default()
            });
        }
        box_.into()
    }

    /// Truncate with an ellipsis when the column is narrower than the text.
    /// Uses the same ~5.6px/char estimate as column autofit.
    fn ellipsize_for_width(label: &str, column_width: f32) -> String {
        const CHAR_PX: f32 = 5.6;
        const PAD_PX: f32 = 12.0; // cell horizontal padding
        let usable = (column_width - PAD_PX).max(CHAR_PX);
        let max_chars = (usable / CHAR_PX).floor() as usize;
        if max_chars == 0 {
            return String::new();
        }
        let char_count = label.chars().count();
        if char_count <= max_chars {
            return label.to_string();
        }
        if max_chars == 1 {
            return "…".to_string();
        }
        let keep = max_chars - 1;
        let mut out: String = label.chars().take(keep).collect();
        out.push('…');
        out
    }

    fn lerp_color(from: Color, to: Color, amount: f32) -> Color {
        Color {
            r: from.r + (to.r - from.r) * amount,
            g: from.g + (to.g - from.g) * amount,
            b: from.b + (to.b - from.b) * amount,
            a: from.a + (to.a - from.a) * amount,
        }
    }

    fn accent_gradient_rule(accent: Color, width: f32) -> Element<'static, Message> {
        const SEGMENTS: usize = 28;
        let dark = Self::lerp_color(accent, Color::BLACK, 0.55);
        let mut segments = row![].spacing(0);
        for segment_index in 0..SEGMENTS {
            let amount = segment_index as f32 / (SEGMENTS - 1) as f32;
            let color = Self::lerp_color(dark, accent, amount);
            segments = segments.push(
                container(Space::new())
                    .width(Length::Fill)
                    .height(Length::Fixed(2.0))
                    .style(move |_theme| container::Style {
                        background: Some(Background::Color(color)),
                        ..Default::default()
                    }),
            );
        }
        segments.width(Length::Fixed(width)).into()
    }

    fn volume_icon_path(volume_kind: &str) -> std::path::PathBuf {
        let name = if volume_kind.eq_ignore_ascii_case("CDROM")
            || volume_kind.eq_ignore_ascii_case("DVD")
        {
            "cdrom"
        } else if volume_kind.eq_ignore_ascii_case("Removable") {
            "floppy"
        } else if volume_kind.eq_ignore_ascii_case("Dynamic") {
            "dynamic"
        } else {
            "disk"
        };
        std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("assets/icons16")
            .join(format!("{name}.ico"))
    }
}
