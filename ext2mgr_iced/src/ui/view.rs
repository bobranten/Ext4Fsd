impl Ext2MgrApp {
    fn view_volume_table(&self) -> Element<'_, Message> {
        let widths = self.volume_col_widths;
        let table_width = self.volume_table_width();
        let header = row![
            Self::header_cell("", Length::Fixed(widths[0]), ListKind::Volume, 0, false),
            Self::header_cell(
                "Volume",
                Length::Fixed(widths[1]),
                ListKind::Volume,
                1,
                true
            ),
            Self::header_cell(
                "Type",
                Length::Fixed(widths[2]),
                ListKind::Volume,
                2,
                true
            ),
            Self::header_cell(
                "File system",
                Length::Fixed(widths[3]),
                ListKind::Volume,
                3,
                true
            ),
            Self::header_cell(
                "Total size",
                Length::Fixed(widths[4]),
                ListKind::Volume,
                4,
                true
            ),
            Self::header_cell(
                "Used size",
                Length::Fixed(widths[5]),
                ListKind::Volume,
                5,
                true
            ),
            Self::header_cell(
                "Codepage",
                Length::Fixed(widths[6]),
                ListKind::Volume,
                6,
                true
            ),
            Self::header_cell(
                "Physical object",
                Length::Fixed(widths[7]),
                ListKind::Volume,
                7,
                true
            ),
            Space::new().width(Self::TABLE_TRAILING_PAD),
        ]
        .spacing(0)
        .width(Length::Fixed(table_width));

        let rows: Element<'_, Message> = if self.volumes.is_empty() {
            text("No volumes").size(14).into()
        } else {
            Column::with_children(
                self.volumes
                    .iter()
                    .enumerate()
                    .map(|(index, volume)| {
                        let selected = self.selection == Selection::Volume(index);
                        let physical = if volume.physical_object.is_empty() {
                            String::new()
                        } else {
                            volume.physical_object.clone()
                        };
                        let icon = image(image::Handle::from_path(Self::volume_icon_path(
                            &volume.volume_kind,
                        )))
                            .width(Length::Fixed(16.0))
                            .height(Length::Fixed(16.0));
                        let icon_cell = {
                            let mut box_ = container(icon)
                                .width(Length::Fixed(widths[0]))
                                .padding([2, 2]);
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
                            box_
                        };
                        let line = row![
                            icon_cell,
                            self.cell(
                                format_letters(&volume.letters),
                                Length::Fixed(widths[1]),
                                selected
                            ),
                            self.cell(
                                volume.volume_kind.clone(),
                                Length::Fixed(widths[2]),
                                selected
                            ),
                            self.cell(
                                volume.filesystem.clone(),
                                Length::Fixed(widths[3]),
                                selected
                            ),
                            self.cell(
                                self.format_size(volume.total_bytes),
                                Length::Fixed(widths[4]),
                                selected
                            ),
                            self.cell(
                                self.format_size(volume.used_bytes),
                                Length::Fixed(widths[5]),
                                selected
                            ),
                            self.cell(
                                volume.codepage.clone(),
                                Length::Fixed(widths[6]),
                                selected
                            ),
                            self.cell(
                                physical,
                                Length::Fixed(widths[7]),
                                selected
                            ),
                            Space::new().width(Self::TABLE_TRAILING_PAD),
                        ]
                        .spacing(0)
                        .width(Length::Fixed(table_width));
                        mouse_area(
                            button(line)
                                .padding(0)
                                .style(button::text)
                                .width(Length::Fixed(table_width))
                                .on_press(Message::SelectVolume(index)),
                        )
                        .on_right_press(Message::ContextVolume(index))
                        .into()
                    })
                    .collect::<Vec<_>>(),
            )
            .spacing(0)
            .width(Length::Fixed(table_width))
            .into()
        };

        let table_body = column![
            header,
            rows,
            Space::new().height(Self::TABLE_H_SCROLLBAR_PAD),
        ]
        .spacing(0)
        .width(Length::Fixed(table_width));

        column![
            text("Volumes").size(14),
            scrollable(table_body)
                .direction(scrollable::Direction::Both {
                    vertical: scrollable::Scrollbar::default(),
                    horizontal: scrollable::Scrollbar::default(),
                })
                .width(Length::Fill)
                .height(Length::Fill),
        ]
        .spacing(2)
        .width(Length::Fill)
        .height(Length::FillPortion(3))
        .into()
    }

    fn view_disk_table(&self) -> Element<'_, Message> {
        let widths = self.disk_col_widths;
        let table_width = self.disk_table_width();
        let header = row![
            Self::header_cell("", Length::Fixed(widths[0]), ListKind::Disk, 0, true),
            Self::header_cell("Type", Length::Fixed(widths[1]), ListKind::Disk, 1, true),
            Self::header_cell(
                "File system",
                Length::Fixed(widths[2]),
                ListKind::Disk,
                2,
                true
            ),
            Self::header_cell(
                "Total size",
                Length::Fixed(widths[3]),
                ListKind::Disk,
                3,
                true
            ),
            Self::header_cell(
                "Used size",
                Length::Fixed(widths[4]),
                ListKind::Disk,
                4,
                true
            ),
            Self::header_cell(
                "Codepage",
                Length::Fixed(widths[5]),
                ListKind::Disk,
                5,
                true
            ),
            Self::header_cell(
                "Partition type",
                Length::Fixed(widths[6]),
                ListKind::Disk,
                6,
                true
            ),
            Space::new().width(Self::TABLE_TRAILING_PAD),
        ]
        .spacing(0)
        .width(Length::Fixed(table_width));

        let rows: Element<'_, Message> = if self.disk_rows.is_empty() {
            text("No disks (open failed - try Refresh as Administrator)")
                .size(14)
                .into()
        } else {
            Column::with_children(
                self.disk_rows
                    .iter()
                    .enumerate()
                    .map(|(row_index, row)| {
                        let line: Element<'_, Message> = match row {
                            DiskRow::Spacer => row![
                                self.cell(String::new(), Length::Fixed(table_width), false)
                            ]
                            .spacing(0)
                            .width(Length::Fixed(table_width))
                            .into(),
                            DiskRow::DiskHeader { disk_index } => {
                                let disk = &self.disks[*disk_index];
                                let selected = self.selection == Selection::Disk(*disk_index);
                                let header = row![
                                    self.cell(
                                        disk.display_name.clone(),
                                        Length::Fixed(widths[0]),
                                        selected
                                    ),
                                    self.cell(
                                        disk.style.clone(),
                                        Length::Fixed(widths[1]),
                                        selected
                                    ),
                                    self.cell(String::new(), Length::Fixed(widths[2]), selected),
                                    self.cell(
                                        self.format_size(disk.total_bytes),
                                        Length::Fixed(widths[3]),
                                        selected
                                    ),
                                    self.cell(String::new(), Length::Fixed(widths[4]), selected),
                                    self.cell(String::new(), Length::Fixed(widths[5]), selected),
                                    self.cell(String::new(), Length::Fixed(widths[6]), selected),
                                    Space::new().width(Self::TABLE_TRAILING_PAD),
                                ]
                                .spacing(0)
                                .width(Length::Fixed(table_width));
                                column![
                                    header,
                                    Self::accent_gradient_rule(self.accent, table_width),
                                ]
                                .spacing(0)
                                .width(Length::Fixed(table_width))
                                .into()
                            }
                            DiskRow::Partition {
                                disk_index,
                                partition_index,
                            } => {
                                let partition =
                                    &self.disks[*disk_index].partitions[*partition_index];
                                let selected = self.selection
                                    == Selection::Partition {
                                        disk: *disk_index,
                                        part: *partition_index,
                                    };
                                row![
                                    self.cell(
                                        format!("  {}", format_letters(&partition.letters)),
                                        Length::Fixed(widths[0]),
                                        selected
                                    ),
                                    self.cell(
                                        partition.style.clone(),
                                        Length::Fixed(widths[1]),
                                        selected
                                    ),
                                    self.cell(
                                        partition.filesystem.clone(),
                                        Length::Fixed(widths[2]),
                                        selected
                                    ),
                                    self.cell(
                                        self.format_size(partition.total_bytes),
                                        Length::Fixed(widths[3]),
                                        selected
                                    ),
                                    self.cell(
                                        self.format_size(partition.used_bytes),
                                        Length::Fixed(widths[4]),
                                        selected
                                    ),
                                    self.cell(
                                        partition.codepage.clone(),
                                        Length::Fixed(widths[5]),
                                        selected
                                    ),
                                    self.cell(
                                        partition.partition_type.clone(),
                                        Length::Fixed(widths[6]),
                                        selected
                                    ),
                                    Space::new().width(Self::TABLE_TRAILING_PAD),
                                ]
                                .spacing(0)
                                .width(Length::Fixed(table_width))
                                .into()
                            }
                        };
                        mouse_area(
                            button(line)
                                .padding(0)
                                .style(button::text)
                                .width(Length::Fixed(table_width))
                                .on_press(Message::SelectDiskRow(row_index)),
                        )
                        .on_right_press(Message::ContextDiskRow(row_index))
                        .into()
                    })
                    .collect::<Vec<_>>(),
            )
            .spacing(0)
            .width(Length::Fixed(table_width))
            .into()
        };

        let table_body = column![
            header,
            rows,
            Space::new().height(Self::TABLE_H_SCROLLBAR_PAD),
        ]
        .spacing(0)
        .width(Length::Fixed(table_width));

        column![
            text("Disks / Partitions").size(14),
            scrollable(table_body)
                .direction(scrollable::Direction::Both {
                    vertical: scrollable::Scrollbar::default(),
                    horizontal: scrollable::Scrollbar::default(),
                })
                .width(Length::Fill)
                .height(Length::Fill),
        ]
        .spacing(2)
        .width(Length::Fill)
        .height(Length::FillPortion(3))
        .into()
    }

    fn view_dialog(&self) -> Option<Element<'_, Message>> {
        match &self.dialog {
            None => None,
            Some(Dialog::LetterPicker {
                selected_letter,
                persist_mode,
                available,
                replace_letter,
                mount_uuid,
                mount_win32,
                existing_session_letters,
                existing_mountmgr_letters,
                ..
            }) => {
                let title = if replace_letter.is_some() {
                    "Change drive letter"
                } else {
                    "Assign drive letter"
                };
                let letter_picker: Element<'_, Message> = if available.is_empty() {
                    text("No free letters").into()
                } else {
                    let choices = available
                        .iter()
                        .map(|letter| format!("{letter}:"))
                        .collect::<Vec<_>>();
                    pick_list(
                        choices,
                        selected_letter.map(|letter| format!("{letter}:")),
                        |picked| Message::PickLetter(picked.chars().next().unwrap_or_default()),
                    )
                    .into()
                };
                let can_automount = mount_uuid.is_some();
                let automount_row: Element<'_, Message> = if can_automount {
                    radio(
                        "Ext2Fsd automount: Ext2Mgr stores Volumes\\{UUID}; remounted when Ext2Mgr or the driver refreshes",
                        crate::mount::ops::MountMode::Ext2Automount,
                        Some(*persist_mode),
                        Message::SetPersistMode,
                    )
                    .into()
                } else {
                    text(
                        "Ext2Fsd automount (needs EXT UUID) — not available for this volume",
                    )
                    .size(13)
                    .style(|_| iced::widget::text::Style {
                        color: Some(Color::from_rgb(0.55, 0.55, 0.55)),
                    })
                    .into()
                };
                let mountmgr_available = !mount_win32.is_empty();
                let mountmgr_blocked = !mountmgr_available
                    || Self::assignment_blocked(
                        existing_mountmgr_letters,
                        *replace_letter,
                    );
                let temporary_blocked = !existing_mountmgr_letters.is_empty()
                    && replace_letter
                        .map(|letter| !existing_mountmgr_letters.contains(&letter))
                        .unwrap_or(true);
                let mountmgr_row: Element<'_, Message> = if !mountmgr_available {
                    text(
                        "Automatic mount via Mount Manager — not available \
                         (no Win32 Volume{GUID} for this DOS-device volume).",
                    )
                    .size(13)
                    .style(|_| iced::widget::text::Style {
                        color: Some(Color::from_rgb(0.55, 0.55, 0.55)),
                    })
                    .into()
                } else if mountmgr_blocked {
                    let listed = existing_mountmgr_letters
                        .iter()
                        .map(|entry| format!("{entry}:"))
                        .collect::<Vec<_>>()
                        .join(", ");
                    text(format!(
                        "Automatic mount via Mount Manager — already set as {listed}. \
                         Use Change on that letter, or remove it first (one per volume)."
                    ))
                    .size(13)
                    .style(|_| iced::widget::text::Style {
                        color: Some(Color::from_rgb(0.55, 0.55, 0.55)),
                    })
                    .into()
                } else {
                    radio(
                        "Automatic mount via Mount Manager (survives reboot; Explorer-native)",
                        crate::mount::ops::MountMode::MountManager,
                        Some(*persist_mode),
                        Message::SetPersistMode,
                    )
                    .into()
                };
                let session_blocked = Self::session_manager_blocked(
                    existing_session_letters,
                    *replace_letter,
                );
                let session_row: Element<'_, Message> = if session_blocked {
                    let listed = existing_session_letters
                        .iter()
                        .map(|entry| format!("{entry}:"))
                        .collect::<Vec<_>>()
                        .join(", ");
                    text(format!(
                        "Permanent Session Manager — already set as {listed}. \
                         Use Change on that letter, or remove it first (one per volume)."
                    ))
                    .size(13)
                    .style(|_| iced::widget::text::Style {
                        color: Some(Color::from_rgb(0.55, 0.55, 0.55)),
                    })
                    .into()
                } else {
                    radio(
                        "Permanent: Session Manager DOS Devices (survives reboot)",
                        crate::mount::ops::MountMode::PermanentRegistry,
                        Some(*persist_mode),
                        Message::SetPersistMode,
                    )
                    .into()
                };
                let temporary_row: Element<'_, Message> = if temporary_blocked {
                    text(
                        "Temporary DefineDosDevice — not available beside an existing \
                         Mount Manager letter.",
                    )
                    .size(13)
                    .style(|_| iced::widget::text::Style {
                        color: Some(Color::from_rgb(0.55, 0.55, 0.55)),
                    })
                    .into()
                } else {
                    radio(
                        "Temporary via Ext2Srv (lost after reboot)",
                        crate::mount::ops::MountMode::Temporary,
                        Some(*persist_mode),
                        Message::SetPersistMode,
                    )
                    .into()
                };

                Some(
                    container(
                        column![
                            text(title).size(18),
                            text("Enter or select a new drive letter").size(13),
                            letter_picker,
                            text("Persistence").size(14),
                            temporary_row,
                            mountmgr_row,
                            session_row,
                            automount_row,
                            text("Temporary = DefineDosDevice only. Mount Manager = SetVolumeMountPoint (same as Disk Management). Session Manager = registry DOS Devices. Device path \\Device\\HarddiskVolumeN. Permanent modes need Administrator.")
                                .size(12),
                            row![
                                button("OK").on_press(Message::ConfirmLetterPicker),
                                button("Cancel").on_press(Message::CloseDialog),
                            ]
                            .spacing(10),
                        ]
                        .spacing(8)
                        .padding(16)
                        .width(Length::Fixed(520.0)),
                    )
                    .style(container::bordered_box)
                    .into(),
                )
            }
            Some(Dialog::MountPoints {
                volume_index,
                disk_part,
                selected,
            }) => {
                let (mut letters, win32_name, device_path, mount_uuid) = {
                    if let Some(index) = *volume_index {
                        self.volumes
                            .get(index)
                            .map(|volume| {
                                (
                                    volume.letters.clone(),
                                    volume.win32_volume_name.clone(),
                                    crate::mount::ops::dos_device_target(
                                        &volume.physical_object,
                                        &volume.symlink,
                                    ),
                                    volume.uuid,
                                )
                            })
                            .unwrap_or_default()
                    } else if let Some((disk, part)) = *disk_part {
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
                                    partition.letters.clone(),
                                    linked
                                        .map(|volume| volume.win32_volume_name.clone())
                                        .unwrap_or_else(|| partition.win32_volume_name.clone()),
                                    crate::mount::ops::dos_device_target(
                                        physical,
                                        volume_symlink,
                                    ),
                                    linked.and_then(|volume| volume.uuid),
                                )
                            })
                            .unwrap_or_default()
                    } else {
                        (Vec::new(), String::new(), String::new(), None)
                    }
                };
                // Include dormant Session Manager letters (registry only, not live DOS yet).
                if !device_path.is_empty() {
                    for letter in crate::mount::persist::registry_letters_for_device(&device_path) {
                        if !letters.contains(&letter) {
                            letters.push(letter);
                        }
                    }
                    letters.sort_unstable();
                }
                let selected_index = *selected;
                let selected_letter = selected_index.and_then(|index| letters.get(index).copied());
                let list: Element<'_, Message> = if letters.is_empty() {
                    text("(no mount points)").into()
                } else {
                    Column::with_children(
                        letters
                            .iter()
                            .enumerate()
                            .map(|(index, letter)| {
                                let label = Self::letter_source_label(
                                    *letter,
                                    &win32_name,
                                    &device_path,
                                    mount_uuid,
                                );
                                let row_button = if selected_index == Some(index) {
                                    button(text(label))
                                        .on_press(Message::MountPointsSelect(index))
                                        .style(button::primary)
                                } else {
                                    button(text(label))
                                        .on_press(Message::MountPointsSelect(index))
                                        .style(button::secondary)
                                };
                                row_button.width(Length::Fill).into()
                            })
                            .collect::<Vec<_>>(),
                    )
                    .spacing(4)
                    .into()
                };
                let mut change_btn = button("Change");
                let mut remove_btn = button("Remove");
                if let Some(letter) = selected_letter {
                    change_btn = change_btn.on_press(Message::OpenLetterPicker {
                        replace_letter: Some(letter),
                    });
                    remove_btn = remove_btn.on_press(Message::RemoveLetter(letter));
                }
                // Mount Manager volumes: one drive letter only — Add is Change/Remove territory.
                let mountmgr_has_letter = !win32_name.is_empty()
                    && !crate::mount::dead_letters::mountmgr_letters_for_volume(&win32_name).is_empty();
                let add_btn = if mountmgr_has_letter {
                    button("Add").width(Length::Fixed(88.0))
                } else {
                    button("Add")
                        .on_press(Message::OpenLetterPicker {
                            replace_letter: None,
                        })
                        .width(Length::Fixed(88.0))
                };
                let actions = column![
                    add_btn,
                    change_btn.width(Length::Fixed(88.0)),
                    remove_btn.width(Length::Fixed(88.0)),
                ]
                .spacing(8);
                Some(
                    container(
                        column![
                            text("Change Drive Letters").size(18),
                            text("Mountpoints:").size(13),
                            row![
                                container(scrollable(list).height(Length::Fixed(140.0)))
                                    .width(Length::Fill)
                                    .padding(4)
                                    .style(container::bordered_box),
                                actions,
                            ]
                            .spacing(10)
                            .align_y(alignment::Vertical::Top),
                            button("Done").on_press(Message::CloseDialog),
                        ]
                        .spacing(10)
                        .padding(16)
                        .width(Length::Fixed(360.0)),
                    )
                    .style(container::bordered_box)
                    .into(),
                )
            }
            Some(Dialog::Service) => {
                #[cfg(windows)]
                {
                    let started = crate::service::mgr::is_driver_started();
                    let draft = &self.service_draft;
                    Some(
                        container({
                                let start_modes = crate::service::mgr::START_MODE_LABELS
                                    .iter()
                                    .map(|label| (*label).to_string())
                                    .collect::<Vec<_>>();
                                let startup_and_codepage = row![
                                    column![
                                        text("Startup mode").size(13),
                                        pick_list(
                                            start_modes,
                                            crate::service::mgr::START_MODE_LABELS
                                                .get(draft.startup as usize)
                                                .map(|label| (*label).to_string()),
                                            |picked| {
                                                Message::ServiceStartup(
                                                    crate::service::mgr::START_MODE_LABELS
                                                        .iter()
                                                        .position(|label| *label == picked)
                                                        .unwrap_or(3) as u32,
                                                )
                                            },
                                        )
                                    ]
                                    .width(Length::FillPortion(1)),
                                    column![
                                        text("Codepage").size(13),
                                        pick_list(
                                            CODE_PAGES.to_vec(),
                                            Some(draft.codepage.as_str()),
                                            |picked| Message::ServiceCodepage(picked.to_string()),
                                        )
                                    ]
                                    .width(Length::FillPortion(1)),
                                ]
                                .spacing(12);
                                let service_controls = if started {
                                    row![
                                        button("Restart Ext2Fsd").on_press(Message::RestartService),
                                        button("Stop Ext2Fsd").on_press(Message::StopService),
                                    ]
                                    .spacing(8)
                                } else {
                                    row![button("Start Ext2Fsd").on_press(Message::StartService)]
                                };
                                column![
                                    text("Service Management").size(18),
                                    row![
                                        text(if started {
                                            "Ext2Fsd: started"
                                        } else {
                                            "Ext2Fsd: NOT started"
                                        })
                                        .width(Length::Fill),
                                        service_controls,
                                    ]
                                    .align_y(iced::Alignment::Center),
                                    startup_and_codepage,
                                    checkbox(draft.readonly).label("Readonly")
                                        .on_toggle(Message::ServiceReadonly),
                                    checkbox(draft.ext3_writable && !draft.readonly).label("Ext3 writable")
                                        .on_toggle(Message::ServiceExt3Writable),
                                    checkbox(draft.automount).label("Assign drive letter automatically")
                                        .on_toggle(Message::ServiceAutomount),
                                    text_input("Hiding prefix", &draft.hiding_prefix)
                                        .on_input(Message::ServicePrefix),
                                    text_input("Hiding suffix", &draft.hiding_suffix)
                                        .on_input(Message::ServiceSuffix),
                                    row![
                                        button("Save").on_press(Message::SaveService),
                                        button("Cancel").on_press(Message::CloseDialog),
                                    ]
                                    .spacing(10),
                                ]
                                .spacing(8)
                                .padding(16)
                                .width(Length::Fixed(560.0))
                            })
                        .style(container::bordered_box)
                        .into(),
                    )
                }
                #[cfg(not(windows))]
                {
                    None
                }
            }
            Some(Dialog::Ext2Attrs {
                volume_index,
                readonly,
                codepage,
                fixmount,
                automount,
                letter,
                available,
                hiding_prefix,
                hiding_suffix,
                uid,
                gid,
                euid,
            }) => {
                let can_automount = self
                    .volumes
                    .get(*volume_index)
                    .is_some_and(|volume| volume.uuid.is_some());
                let letters: Element<'_, Message> = Column::with_children(
                    available
                        .iter()
                        .map(|drive| {
                            radio(
                                format!("{drive}:"),
                                *drive,
                                *letter,
                                Message::Ext2Letter,
                            )
                            .into()
                        })
                        .collect::<Vec<_>>(),
                )
                .spacing(2)
                .into();
                let codepage_picks: Element<'_, Message> = Column::with_children(
                    CODE_PAGES
                        .iter()
                        .enumerate()
                        .map(|(index, page)| {
                            let selected = CODE_PAGES
                                .iter()
                                .position(|candidate| *candidate == codepage.as_str());
                            radio(
                                (*page).to_string(),
                                index,
                                selected,
                                |picked| {
                                    Message::Ext2Codepage(CODE_PAGES[picked].to_string())
                                },
                            )
                            .into()
                        })
                        .collect::<Vec<_>>(),
                )
                .spacing(2)
                .into();
                let automount_label = if can_automount {
                    "Automatically mount via Ext2Mgr"
                } else {
                    "Automatically mount via Ext2Mgr (needs EXT UUID)"
                };
                let automount_box: Element<'_, Message> = {
                    let box_ = checkbox(*automount && can_automount).label(automount_label);
                    if can_automount {
                        box_.on_toggle(Message::Ext2Automount).into()
                    } else {
                        box_.into()
                    }
                };
                Some(
                    container(
                        column![
                            text("Ext2/3 Volume Settings").size(18),
                            text("Volume attribute").size(14),
                            checkbox(*readonly).label("Mount volume in readonly mode")
                                .on_toggle(Message::Ext2Readonly),
                            text("Codepage").size(13),
                            scrollable(codepage_picks).height(Length::Fixed(100.0)),
                            text_input("codepage", codepage).on_input(Message::Ext2Codepage),
                            text("Mount point and drive letter").size(14),
                            automount_box,
                            checkbox(*fixmount).label(
                                "Mountpoint for fixed disk, need reboot"
                            )
                            .on_toggle(Message::Ext2Fixmount),
                            text("Letter").size(13),
                            scrollable(letters).height(Length::Fixed(90.0)),
                            text("Hiding filter patterns").size(14),
                            text_input("Hiding files with prefix", hiding_prefix)
                                .on_input(Message::Ext2Prefix),
                            text_input("Hiding files with suffix", hiding_suffix)
                                .on_input(Message::Ext2Suffix),
                            text("Mounting as User").size(14),
                            row![
                                text("UID:").size(13),
                                text_input("----", uid).width(Length::Fixed(70.0))
                                    .on_input(Message::Ext2Uid),
                                text("GID:").size(13),
                                text_input("----", gid).width(Length::Fixed(70.0))
                                    .on_input(Message::Ext2Gid),
                                text("EUID:").size(13),
                                text_input("----", euid).width(Length::Fixed(70.0))
                                    .on_input(Message::Ext2Euid),
                            ]
                            .spacing(6),
                            row![
                                button("Apply").on_press(Message::SaveExt2Attrs),
                                button("Cancel").on_press(Message::CloseDialog),
                            ]
                            .spacing(10),
                        ]
                        .spacing(6)
                        .padding(16)
                        .width(Length::Fixed(560.0)),
                    )
                    .style(container::bordered_box)
                    .into(),
                )
            }
            Some(Dialog::DeadLetters {
                entries,
                selected,
                also_remove_permanent,
                pending_remove,
            }) => {
                if let Some(entry) = pending_remove {
                    Some(
                        container(
                            column![
                                text("Remove Dead Letters").size(18),
                                text(format!("{}:  {}", entry.letter, entry.symlink)).size(13),
                                text(
                                    "Warning: this drive letter might still be in use.\n\
                                     Are you sure it is a real dead drive letter?"
                                )
                                .size(13),
                                row![
                                    button("Yes").on_press(Message::ConfirmDeadLetterRemove),
                                    button("No").on_press(Message::CancelDeadLetterRemove),
                                ]
                                .spacing(10),
                            ]
                            .spacing(10)
                            .padding(16)
                            .width(Length::Fixed(560.0)),
                        )
                        .style(container::bordered_box)
                        .into(),
                    )
                } else {
                    let list: Element<'_, Message> = if entries.is_empty() {
                        text("No dead letters found.").into()
                    } else {
                        Column::with_children(
                            entries
                                .iter()
                                .enumerate()
                                .map(|(index, entry)| {
                                    radio(
                                        format!("{}:  {}", entry.letter, entry.symlink),
                                        index,
                                        *selected,
                                        Message::SelectDeadLetter,
                                    )
                                    .into()
                                })
                                .collect::<Vec<_>>(),
                        )
                        .spacing(4)
                        .into()
                    };
                    let show_permanent = selected
                        .and_then(|index| entries.get(index))
                        .is_some_and(|entry| entry.may_have_permanent);
                    let mut body = column![
                        text("Remove Dead Letters").size(18),
                        scrollable(list).height(Length::Fixed(200.0)),
                    ]
                    .spacing(8);
                    if show_permanent {
                        body = body.push(
                            checkbox(*also_remove_permanent)
                                .label("Also remove permanent Mount Manager letter")
                                .on_toggle(Message::DeadAlsoRemovePermanent),
                        );
                    }
                    body = body.push(
                        row![
                            button("Reload").on_press(Message::OpenDeadLetters),
                            button("Remove").on_press(Message::RemoveSelectedDeadLetter),
                            button("Exit").on_press(Message::CloseDialog),
                        ]
                        .spacing(10),
                    );
                    Some(
                        container(body.padding(16).width(Length::Fixed(560.0)))
                            .style(container::bordered_box)
                            .into(),
                    )
                }
            }
            Some(Dialog::Properties) => {
                let snap = self.properties_snapshot();
                let disk_box = container(
                    column![
                        text(if snap.disk_title.is_empty() {
                            "DISK".to_string()
                        } else {
                            snap.disk_title.clone()
                        })
                        .size(13),
                        Self::prop_row("Vendor:", snap.vendor.clone()),
                        Self::prop_row("Product:", snap.product.clone()),
                        Self::prop_row("Serial:", snap.serial.clone()),
                        Self::prop_row("Bus:", snap.bus_type.clone()),
                        Self::prop_row("Type:", snap.device_type.clone()),
                        Self::prop_row("Media:", snap.media_type.clone()),
                        Self::prop_row("Capacity (bytes):", snap.disk_capacity_bytes.clone()),
                    ]
                    .spacing(4)
                    .padding(8),
                )
                .width(Length::Fill)
                .style(container::bordered_box);

                let mut mount_row = row![
                    text("Mount points:").size(12).width(Length::Fixed(110.0)),
                    text(snap.mount_points.clone())
                        .size(12)
                        .width(Length::Fill),
                ]
                .spacing(6)
                .align_y(alignment::Vertical::Center);
                if snap.can_change_mp {
                    mount_row = mount_row
                        .push(button("Change").on_press(Message::PropertiesChangeMp));
                }
                if snap.can_mount {
                    mount_row =
                        mount_row.push(button("Mount").on_press(Message::PropertiesQuickMount));
                }
                if snap.can_unmount {
                    mount_row =
                        mount_row.push(button("Unmount").on_press(Message::PropertiesUnmount));
                }

                let mut fs_row = row![
                    text("File system:").size(12).width(Length::Fixed(110.0)),
                    text(snap.filesystem.clone())
                        .size(12)
                        .width(Length::Fill),
                ]
                .spacing(6)
                .align_y(alignment::Vertical::Center);
                if snap.can_ext2 {
                    fs_row = fs_row.push(
                        button("Ext2 Properties").on_press(Message::PropertiesExt2Info),
                    );
                }

                let sdev_box = container(
                    column![
                        text(if snap.sdev_title.is_empty() {
                            "Volume".to_string()
                        } else {
                            snap.sdev_title.clone()
                        })
                        .size(13),
                        Self::prop_row("Status:", snap.status.clone()),
                        mount_row,
                        fs_row,
                        Self::prop_row("Capacity:", snap.capacity_bytes.clone()),
                        Self::prop_row("Free space:", snap.free_bytes.clone()),
                    ]
                    .spacing(4)
                    .padding(8),
                )
                .width(Length::Fill)
                .style(container::bordered_box);

                // Overlay already draws one bordered card; keep only the two
                // section boxes (disk / volume) inside — no nested outer chrome.
                Some(
                    column![
                        text("Properties").size(18),
                        disk_box,
                        sdev_box,
                        button("Exit").on_press(Message::CloseDialog),
                    ]
                    .spacing(10)
                    .padding(16)
                    .width(Length::Fixed(460.0))
                    .into(),
                )
            }
            Some(Dialog::About {
                driver_line,
                program_line,
            }) => Some(
                container(
                    column![
                        text("About Ext2 Volume Manager").size(18),
                        text(driver_line).size(13),
                        row![
                            text(format!("{program_line} — ")).size(13),
                            button(text("Iced port by Obsidian Jackal").size(13))
                                .padding(0)
                                .style(button::text)
                                .on_press(Message::OpenIcedPortWebsite),
                        ]
                        .spacing(0)
                        .align_y(alignment::Vertical::Center),
                        text("Ext2/Ext4 volume manager for Windows (port of Ext2Mgr).")
                            .size(12),
                        row![
                            button("Original project website")
                                .on_press(Message::OpenAboutWebsite),
                            button("Donate").on_press(Message::MenuDonate),
                            button("OK").on_press(Message::CloseDialog),
                        ]
                        .spacing(8),
                    ]
                    .spacing(10)
                    .padding(16)
                    .width(Length::Fixed(520.0)),
                )
                .style(container::bordered_box)
                .into(),
            ),
            Some(Dialog::Donate) => Some(
                container(
                    column![
                        text("Contribute to Ext2Fsd Group").size(18),
                        text("Donation declaration").size(14),
                        scrollable(
                            text(
                                "Ext2Fsd is an open source software. It acts as a bridge \
between Windows and Linux, making life easier to access Linux partitions under Windows systems.\n\n\
Currently there are still lots of jobs left to make a fully functional file system driver, \
such as complete ext3 support, Linux LVM, and ongoing Windows support.\n\n\
Any help will be highly appreciated. Thanks and best wishes.\n\n\
Yours sincerely,\nMatt",
                            )
                            .size(12),
                        )
                        .height(Length::Fixed(220.0)),
                        text("Click to donate").size(14),
                        button("Donate via SourceForge.net")
                            .on_press(Message::OpenDonateSourceForge),
                        button("Donate via PayPal.com").on_press(Message::OpenDonatePayPal),
                        button("OK").on_press(Message::CloseDialog),
                    ]
                    .spacing(8)
                    .padding(16)
                    .width(Length::Fixed(480.0)),
                )
                .style(container::bordered_box)
                .into(),
            ),
            Some(Dialog::PerfStat { rows }) => {
                let header = row![
                    text("Name").width(Length::Fixed(280.0)),
                    text("Current").width(Length::Fixed(100.0)),
                    text("Processed").width(Length::Fixed(120.0)),
                ];
                let table = Column::with_children(
                    rows.iter()
                        .map(|(name, current, processed)| {
                            row![
                                text(name).width(Length::Fixed(280.0)),
                                text(current.to_string()).width(Length::Fixed(100.0)),
                                text(processed.to_string()).width(Length::Fixed(120.0)),
                            ]
                            .into()
                        })
                        .collect::<Vec<_>>(),
                )
                .spacing(3);
                Some(
                container(
                    column![
                        text("Ext2Fsd Statistics").size(18),
                        header,
                        scrollable(table).height(Length::Fixed(360.0)),
                        row![
                            button("Copy").on_press(Message::CopyPerfStat),
                            button("Close").on_press(Message::CloseDialog),
                        ]
                        .spacing(8),
                    ]
                    .spacing(10)
                    .padding(16)
                    .width(Length::Fixed(560.0)),
                )
                .style(container::bordered_box)
                .into(),
                )
            }
            Some(Dialog::PartitionType {
                selected_type,
                note,
                ..
            }) => {
                let type_choices: Element<'_, Message> = Column::with_children(
                    [
                        (0x07u8, "HPFS/NTFS"),
                        (0x0Bu8, "FAT32"),
                        (0x0Cu8, "FAT32X"),
                        (0x82u8, "Linux swap"),
                        (0x83u8, "Linux"),
                        (0x8Eu8, "Linux LVM"),
                    ]
                    .into_iter()
                    .map(|(type_id, label)| {
                        radio(
                            format!("0x{type_id:02X} {label}"),
                            type_id,
                            Some(*selected_type),
                            Message::PartitionTypePick,
                        )
                        .into()
                    })
                    .collect::<Vec<_>>(),
                )
                .into();
                Some(
                    container(
                        column![
                            text("Change Partition Type").size(18),
                            text(note).size(12),
                            scrollable(type_choices).height(Length::Fixed(200.0)),
                            row![
                                button("Apply").on_press(Message::ApplyPartitionType),
                                button("Cancel").on_press(Message::CloseDialog),
                            ]
                            .spacing(8),
                        ]
                        .spacing(10)
                        .padding(16)
                        .width(Length::Fixed(420.0)),
                    )
                    .style(container::bordered_box)
                    .into(),
                )
            }

        }
    }


    pub fn view(&self) -> Element<'_, Message> {
        let status_bar = row![
            container(text(&self.status).size(12))
                .width(Length::Fill)
                .padding([4, 8]),
            container(text(&self.clock).size(12))
                .padding([4, 8])
                .align_x(alignment::Horizontal::Right),
        ]
        .width(Length::Fill)
        .align_y(alignment::Vertical::Center);

        let detail = container(
            text_editor(&self.detail_editor)
                .height(Length::Fill)
                .on_action(Message::DetailEditor),
        )
        .width(Length::Fill)
        .height(Length::FillPortion(2))
        .padding(8)
        .style(container::bordered_box);

        let mut main = column![
            self.view_volume_table(),
            self.view_disk_table(),
        ]
        .spacing(6)
        .padding(8)
        .width(Length::Fill)
        .height(Length::Fill);
        if self.show_properties_pane {
            main = main.push(text("Properties").size(14)).push(detail);
        }

        let scrolled_main = container(main)
            .width(Length::Fill)
            .height(Length::Fill);

        let body: Element<'_, Message> = if let Some(dialog) = self.view_dialog() {
            let dialog_card = mouse_area(
                container(dialog)
                    .padding(16)
                    .style(container::bordered_box),
            )
            .on_press(Message::DialogAbsorbClick);
            let overlay = mouse_area(
                container(dialog_card)
                    .width(Length::Fill)
                    .height(Length::Fill)
                    .padding(24)
                    .center_x(Length::Fill)
                    .center_y(Length::Fill)
                    .style(|_theme| container::Style {
                        background: Some(Background::Color(Color::from_rgba(
                            0.0, 0.0, 0.0, 0.35,
                        ))),
                        ..Default::default()
                    }),
            )
            .on_press(Message::CloseDialog);
            stack![scrolled_main, overlay].height(Length::Fill).into()
        } else {
            scrolled_main.into()
        };
        column![body, status_bar]
            .spacing(0)
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }
}
