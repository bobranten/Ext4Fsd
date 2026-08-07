//! Disk / partition / volume enumeration for the Ext2Mgr UI.

mod win_impl {
    use crate::disk::fs_probe::{
        BTRFS_SUPER_BLOCK_OFFSET, RAID_SUPER_BLOCK_OFFSET,
    };
    use std::os::windows::ffi::OsStrExt;

    const IOCTL_DISK_GET_DRIVE_GEOMETRY_EX: u32 = 0x0007_00A0;
    const IOCTL_DISK_GET_DRIVE_LAYOUT_EX: u32 = 0x0007_0050;
    const IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS: u32 = 0x0056_0000;
    const IOCTL_STORAGE_QUERY_PROPERTY: u32 = 0x002D_1400;

    const PARTITION_STYLE_MBR: u32 = 0;
    const PARTITION_STYLE_GPT: u32 = 1;
    const PARTITION_ENTRY_UNUSED: u8 = 0x00;
    const PARTITION_EXTENDED: u8 = 0x05;
    const PARTITION_XINT13_EXTENDED: u8 = 0x0F;

    /// DRIVE_LAYOUT_INFORMATION_EX before PartitionEntry[]:
    /// PartitionStyle(4) + PartitionCount(4) + union{Mbr(8),Gpt(40)} = 48 bytes.
    /// Using a shorter header misaligns PARTITION_INFORMATION_EX entries.
    const DRIVE_LAYOUT_HEADER_SIZE: usize = 48;
    /// sizeof(PARTITION_INFORMATION_EX) on Windows x64.
    const PARTITION_INFORMATION_EX_SIZE: usize = 144;

    #[derive(Debug, Clone)]
    pub struct DiskExtent {
        pub disk_number: u32,
        pub starting_offset: u64,
        pub extent_length: u64,
    }

    #[derive(Debug, Clone)]
    pub struct VolumeEntry {
        pub letters: Vec<char>,
        pub volume_kind: String,
        pub filesystem: String,
        pub total_bytes: Option<u64>,
        pub used_bytes: Option<u64>,
        pub codepage: String,
        pub physical_object: String,
        pub symlink: String,
        pub win32_volume_name: String,
        pub extents: Vec<DiskExtent>,
        pub uuid: Option<[u8; 16]>,
    }

    #[derive(Debug, Clone)]
    pub struct PartitionEntry {
        pub number: u32,
        pub style: String,
        pub partition_type: String,
        pub starting_offset: u64,
        pub length: u64,
        pub letters: Vec<char>,
        pub filesystem: String,
        pub codepage: String,
        pub total_bytes: Option<u64>,
        pub used_bytes: Option<u64>,
        pub volume_index: Option<usize>,
        pub symlink: String,
        pub win32_volume_name: String,
    }

    #[derive(Debug, Clone)]
    pub struct DiskEntry {
        pub index: u32,
        pub display_name: String,
        pub style: String,
        pub total_bytes: Option<u64>,
        pub partitions: Vec<PartitionEntry>,
        pub device_path: String,
        /// From STORAGE_DEVICE_DESCRIPTOR (Properties dialog).
        pub vendor: String,
        pub product: String,
        pub serial: String,
        pub bus_type: String,
        pub media_type: String,
        pub removable: bool,
    }

    /// Flattened disk-list rows for the UI (headers, partitions, spacers).
    #[derive(Debug, Clone)]
    pub enum DiskRow {
        DiskHeader {
            disk_index: usize,
        },
        Partition {
            disk_index: usize,
            partition_index: usize,
        },
        Spacer,
    }

    pub fn enumerate_all() -> (Vec<DiskEntry>, Vec<VolumeEntry>, Vec<DiskRow>) {
        let mut volumes = enumerate_volumes();
        let mut disks = enumerate_disks();
        link_partitions_to_volumes(&mut disks, &volumes);
        // Ext2 DefineDosDevice volumes vanish from FindFirstVolume when unlettered.
        // Probe the partition on-disk so FS type / UUID survive Remove-all-letters.
        fill_missing_partition_filesystems(&mut disks);
        let before = volumes.len();
        append_volumes_for_unlinked_ext_partitions(&disks, &mut volumes);
        if volumes.len() != before {
            // Synthesized rows are appended; re-sort so Volume7 does not sink to bottom.
            sort_volumes_by_harddisk_number(&mut volumes);
            link_partitions_to_volumes(&mut disks, &volumes);
        }
        let rows = build_disk_rows(&disks);
        (disks, volumes, rows)
    }

    pub fn format_letters(letters: &[char]) -> String {
        if letters.is_empty() {
            String::new()
        } else {
            let joined = letters
                .iter()
                .map(|letter| format!("{letter}:"))
                .collect::<Vec<_>>()
                .join(",");
            format!("({joined})")
        }
    }

    /// Ext2Mgr-style UUID: `AA-BB-CC-...-FF` (no braces).
    pub fn format_ext2_style_uuid(uuid: &[u8; 16]) -> String {
        let mut out = String::new();
        for (index, byte) in uuid.iter().enumerate() {
            if index == 0 {
                out.push_str(&format!("{byte:02X}"));
            } else {
                out.push_str(&format!("-{byte:02X}"));
            }
        }
        out
    }

    /// Prefer superblock Ext2Mgr UUID; else bare `guid` from `\\?\Volume{guid}\` (no braces, no `\\?\Volume` prefix).
    pub fn display_volume_guid(win32_volume_name: &str, uuid: Option<[u8; 16]>) -> String {
        if let Some(bytes) = uuid {
            return format_ext2_style_uuid(&bytes);
        }
        let trimmed = win32_volume_name.trim_end_matches('\\');
        if let Some(rest) = trimmed.strip_prefix(r"\\?\Volume{") {
            if let Some(guid) = rest.strip_suffix('}') {
                return guid.to_string();
            }
        }
        String::new()
    }

    /// Decimal (SI) vs binary IEC unit labels and bases for capacity display.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub enum SizeUnitStyle {
        /// 1000-based units (KB/MB/… or Kb/Mb/… when showing bits).
        #[default]
        Si,
        /// 1024-based units (KiB/MiB/… or Kib/Mib/… when showing bits).
        Binary,
    }

    /// Format a byte count for list/properties Capacity/Used/Free.
    ///
    /// Small values use a `> 10×` next-unit threshold (same idea as Ext2Mgr).
    /// Larger values step up when the whole-number value would be ≥ 1000
    /// (1000 GB → TB, 1000 TB → PB, 1000 PB → EB). Escalation stops at EB/EiB.
    /// When `as_bits` is true, the value is shown as bits (×8) with bit labels.
    pub fn format_size(bytes: Option<u64>, style: SizeUnitStyle, as_bits: bool) -> String {
        let Some(total_bytes) = bytes else {
            return String::new();
        };
        let total = if as_bits {
            total_bytes.saturating_mul(8)
        } else {
            total_bytes
        };
        match (style, as_bits) {
            (SizeUnitStyle::Si, false) => {
                format_size_scale(total, 1000, &["KB", "MB", "GB", "TB", "PB", "EB"], "B")
            }
            (SizeUnitStyle::Si, true) => {
                format_size_scale(total, 1000, &["Kb", "Mb", "Gb", "Tb", "Pb", "Eb"], "b")
            }
            (SizeUnitStyle::Binary, false) => {
                format_size_scale(total, 1024, &["KiB", "MiB", "GiB", "TiB", "PiB", "EiB"], "B")
            }
            (SizeUnitStyle::Binary, true) => {
                format_size_scale(total, 1024, &["Kib", "Mib", "Gib", "Tib", "Pib", "Eib"], "b")
            }
        }
    }

    fn format_size_scale(total: u64, base: u64, labels: &[&str; 6], atom: &str) -> String {
        let kb = base;
        let mb = kb * base;
        let gb = mb * base;
        let tb = gb * base;
        let pb = tb * base;
        let eb = pb * base;
        if total / pb >= 1000 {
            format!("{} {}", total / eb, labels[5])
        } else if total / tb >= 1000 {
            format!("{} {}", total / pb, labels[4])
        } else if total / gb >= 1000 {
            format!("{} {}", total / tb, labels[3])
        } else if total > 10 * gb {
            format!("{} {}", total / gb, labels[2])
        } else if total > 10 * mb {
            format!("{} {}", total / mb, labels[1])
        } else if total > 10 * kb {
            format!("{} {}", total / kb, labels[0])
        } else {
            format!("{total} {atom}")
        }
    }

    pub fn mbr_partition_type_name(type_id: u8) -> &'static str {
        // Mirrors Ext2Mgr enumDisk.cpp PartitionList / PartitionString.
        match type_id {
            0x00 => "Empty",
            0x01 => "FAT12",
            0x02 => "Xenix-1",
            0x03 => "Xenix-2",
            0x04 => "FAT16",
            0x05 => "Extended",
            0x06 => "FAT16 HUGE",
            0x07 => "HPFS/NTFS",
            0x0A => "OS/2",
            0x0B => "FAT32",
            0x0C => "FAT32X",
            0x0E => "XINT13",
            0x0F => "EXINT13",
            0x11 => "Hidden FAT12",
            0x14 | 0x16 => "Hidden FAT16",
            0x17 => "Hidden HPFS/NTFS",
            0x1B => "Hidden FAT32",
            0x1C => "Hidden FAT32X",
            0x41 => "OS/2",
            0x42 => "LDM",
            0x52 => "CP/M",
            0x63 => "UNIX",
            0x80 => "NTFT",
            0x81 => "Minix",
            0x82 => "Linux swap",
            0x83 => "Linux",
            0x85 => "Linux extend",
            0x8E => "Linux LVM",
            0xA5 => "FreeBSD",
            0xA6 => "OpenBSD",
            0xA8 => "Darwin UFS",
            0xA9 => "NetBSD",
            0xBE => "Solaris Boot",
            0xBF => "Solaris",
            0xC0 => "VNTFT",
            _ => "UNKNOWN",
        }
    }

    /// Map GPT PartitionType GUID (16 bytes, Windows GUID memory layout) to ASCII.
    /// Used when Gpt.Name is empty (common) so the column is still readable.
    fn gpt_partition_type_name(type_guid: &[u8]) -> Option<&'static str> {
        if type_guid.len() < 16 {
            return None;
        }
        // Compare as little-endian GUID byte pattern (same layout as on-disk / IOCTL).
        match type_guid {
            // {c12a7328-f81f-11d2-ba4b-00a0c93ec93b}
            [0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11, 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b] => {
                Some("EFI System")
            }
            // {e3c9e316-0b5c-4db8-817d-f92df00215ae}
            [0x16, 0xe3, 0xc9, 0xe3, 0x5c, 0x0b, 0xb8, 0x4d, 0x81, 0x7d, 0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae] => {
                Some("MSR")
            }
            // {ebd0a0a2-b9e5-4433-87c0-68b6b72699c7}
            [0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44, 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7] => {
                Some("Basic data")
            }
            // {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3}
            [0xaa, 0xc8, 0x08, 0x58, 0x8f, 0x7e, 0xe0, 0x42, 0x85, 0xd2, 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3] => {
                Some("LDM metadata")
            }
            // {af9b60a0-1431-4f62-bc68-3311714a69ad}
            [0xa0, 0x60, 0x9b, 0xaf, 0x31, 0x14, 0x62, 0x4f, 0xbc, 0x68, 0x33, 0x11, 0x71, 0x4a, 0x69, 0xad] => {
                Some("LDM data")
            }
            // {de94bba4-06d1-4d40-a16a-bfd50179d6ac}
            [0xa4, 0xbb, 0x94, 0xde, 0xd1, 0x06, 0x40, 0x4d, 0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac] => {
                Some("Windows recovery")
            }
            // {0fc63daf-8483-4772-8e79-3d69d8477de4}
            [0xaf, 0x3d, 0xc6, 0x0f, 0x83, 0x84, 0x72, 0x47, 0x8e, 0x79, 0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4] => {
                Some("Linux filesystem")
            }
            // {0657fd6d-a4ab-43c4-84e5-0933c84b4f4f}
            [0x6d, 0xfd, 0x57, 0x06, 0xab, 0xa4, 0xc4, 0x43, 0x84, 0xe5, 0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f] => {
                Some("Linux swap")
            }
            // {e6d6d379-f507-44c2-a23c-238f2a3df928}
            [0x79, 0xd3, 0xd6, 0xe6, 0x07, 0xf5, 0xc2, 0x44, 0xa2, 0x3c, 0x23, 0x8f, 0x2a, 0x3d, 0xf9, 0x28] => {
                Some("Linux LVM")
            }
            _ => None,
        }
    }

    fn enumerate_volumes() -> Vec<VolumeEntry> {
        let mut volumes = Vec::new();
        let mut name_buf = vec![0u16; 256];
        let find_handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::FindFirstVolumeW(
                name_buf.as_mut_ptr(),
                name_buf.len() as u32,
            )
        };
        if find_handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            return volumes;
        }

        loop {
            if let Some(entry) = volume_from_guid_name(&name_buf) {
                volumes.push(entry);
            }
            name_buf.fill(0);
            let more = unsafe {
                windows_sys::Win32::Storage::FileSystem::FindNextVolumeW(
                    find_handle,
                    name_buf.as_mut_ptr(),
                    name_buf.len() as u32,
                )
            };
            if more == 0 {
                break;
            }
        }
        unsafe {
            windows_sys::Win32::Storage::FileSystem::FindVolumeClose(find_handle);
        }

        // Ext2Mgr: match letters by mount point + comparing letter extents to volume extents
        // (GetVolumePathNames alone misses Ext2Srv/temp mounts and some fixed letters).
        attach_letters_like_ext2mgr(&mut volumes);
        // Ext2Fsd / DefineDosDevice letters often have no Win32 Volume{GUID} mount
        // point, so FindFirstVolume never saw them. Synthesize those orphans.
        synthesize_volumes_for_orphan_letters(&mut volumes);
        for volume in volumes.iter_mut() {
            // Letter-less Win32 volumes must still keep FS (after Remove all letters).
            let can_probe = !volume.letters.is_empty() || !volume.win32_volume_name.is_empty();
            if can_probe {
                let probe_name = if !volume.win32_volume_name.is_empty() {
                    volume.win32_volume_name.clone()
                } else {
                    format!(r"\\.\{}:", volume.letters[0])
                };
                let (filesystem, total_bytes, used_bytes, uuid) =
                    detect_filesystem_and_sizes(&probe_name, &volume.letters);
                if !filesystem.is_empty() {
                    volume.filesystem = filesystem;
                }
                if total_bytes.is_some() {
                    volume.total_bytes = total_bytes;
                }
                if used_bytes.is_some() {
                    volume.used_bytes = used_bytes;
                }
                if uuid.is_some() {
                    volume.uuid = uuid;
                }
                if volume.physical_object.is_empty() {
                    if let Some(path) =
                        nt_device_name_for_volume(&volume.win32_volume_name, &volume.letters)
                    {
                        volume.physical_object = path;
                    }
                }
            }
            // Codepage column: Ext2Mgr `EVP.Codepage` via IOCTL / registry.
            volume.codepage = resolve_volume_codepage(volume);
        }

        sort_volumes_by_harddisk_number(&mut volumes);
        volumes
    }

    fn sort_volumes_by_harddisk_number(volumes: &mut [VolumeEntry]) {
        volumes.sort_by(|left, right| {
            harddisk_volume_number(&left.physical_object)
                .or_else(|| harddisk_volume_number(&left.symlink))
                .cmp(
                    &harddisk_volume_number(&right.physical_object)
                        .or_else(|| harddisk_volume_number(&right.symlink)),
                )
                .then_with(|| left.letters.first().cmp(&right.letters.first()))
                .then_with(|| left.symlink.cmp(&right.symlink))
        });
    }

    /// Parse N from `\Device\HarddiskVolumeN` as an integer.
    /// String compare would order Volume10 before Volume6.
    fn harddisk_volume_number(path: &str) -> Option<u32> {
        let upper = path.to_ascii_uppercase();
        let marker = "HARDDISKVOLUME";
        let start = upper.find(marker)? + marker.len();
        let digits: String = upper[start..]
            .chars()
            .take_while(|ch| ch.is_ascii_digit())
            .collect();
        if digits.is_empty() {
            None
        } else {
            digits.parse().ok()
        }
    }

    fn enumerate_disks() -> Vec<DiskEntry> {
        let mut disks = Vec::new();
        let mut consecutive_missing = 0u32;
        for disk_index in 0u32..64 {
            match probe_disk(disk_index) {
                Ok(Some(entry)) => {
                    consecutive_missing = 0;
                    disks.push(entry);
                }
                Ok(None) => {
                    consecutive_missing += 1;
                    // PhysicalDrive numbers are usually contiguous; allow a small gap.
                    if consecutive_missing >= 2 && !disks.is_empty() {
                        break;
                    }
                    if consecutive_missing >= 2 && disks.is_empty() && disk_index > 8 {
                        break;
                    }
                }
                Err(_) => {
                    // Access denied / busy: keep scanning - do not treat as end-of-list.
                    consecutive_missing = 0;
                }
            }
        }
        disks
    }

    fn open_physical_drive(
        disk_index: u32,
    ) -> Result<Option<windows_sys::Win32::Foundation::HANDLE>, std::io::Error> {
        let path = format!(r"\\.\PhysicalDrive{disk_index}");
        let wide = to_wide(&path);
        // Prefer access=0 for geometry/layout IOCTLs; fall back to GENERIC_READ.
        // Elevated FILE_GENERIC_READ can fail on locked drives and skip disks.
        const GENERIC_READ: u32 = 0x8000_0000;
        for access in [0u32, GENERIC_READ] {
            let handle = unsafe {
                windows_sys::Win32::Storage::FileSystem::CreateFileW(
                    wide.as_ptr(),
                    access,
                    windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                        | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                    std::ptr::null(),
                    windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                    0,
                    0,
                )
            };
            if handle != windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
                return Ok(Some(handle));
            }
            let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
            if code == 2 || code == 3 {
                return Ok(None);
            }
            // try next access mode
            if access == GENERIC_READ {
                return Err(std::io::Error::from_raw_os_error(code as i32));
            }
        }
        Ok(None)
    }

    fn probe_disk(disk_index: u32) -> Result<Option<DiskEntry>, std::io::Error> {
        let Some(handle) = open_physical_drive(disk_index)? else {
            return Ok(None);
        };

        let total_bytes = ioctl_disk_size(handle);
        let media_type = ioctl_media_type(handle).unwrap_or_else(|| "Unknown".to_string());
        let inquiry = ioctl_storage_device(handle).unwrap_or_default();
        let (style, partitions) = ioctl_drive_layout(handle).unwrap_or_else(|| {
            ("RAW".to_string(), Vec::new())
        });
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }

        Ok(Some(DiskEntry {
            index: disk_index,
            display_name: format!("DISK {disk_index}"),
            style,
            total_bytes,
            partitions,
            device_path: format!(r"\Device\Harddisk{disk_index}\DR{disk_index}"),
            vendor: inquiry.vendor,
            product: inquiry.product,
            serial: inquiry.serial,
            bus_type: inquiry.bus_type,
            media_type,
            removable: inquiry.removable,
        }))
    }

    #[derive(Default)]
    struct StorageInquiry {
        vendor: String,
        product: String,
        serial: String,
        bus_type: String,
        removable: bool,
    }

    fn cstr_at(buf: &[u8], offset: u32) -> String {
        if offset == 0 || offset as usize >= buf.len() {
            return String::new();
        }
        let slice = &buf[offset as usize..];
        let end = slice.iter().position(|&byte| byte == 0).unwrap_or(slice.len());
        String::from_utf8_lossy(&slice[..end])
            .trim()
            .to_string()
    }

    fn bus_type_string(bus: u8) -> String {
        match bus {
            1 => "SCSI",
            2 => "ATAPI",
            3 => "ATA",
            4 => "1394",
            5 => "SSA",
            6 => "Fibre",
            7 => "USB",
            8 => "RAID",
            9 => "iSCSI",
            10 => "SAS",
            11 => "SATA",
            12 => "SD",
            13 => "MMC",
            14 => "Virtual",
            15 => "FileBackedVirtual",
            16 => "Spaces",
            17 => "NVMe",
            _ => "Unknown",
        }
        .to_string()
    }

    fn ioctl_storage_device(handle: windows_sys::Win32::Foundation::HANDLE) -> Option<StorageInquiry> {
        // STORAGE_PROPERTY_QUERY: PropertyId=StorageDeviceProperty(0), QueryType=PropertyStandardQuery(0)
        let mut query = [0u8; 12];
        let mut buffer = vec![0u8; 1024];
        let mut bytes_returned = 0u32;
        let ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_STORAGE_QUERY_PROPERTY,
                query.as_mut_ptr() as *mut _,
                query.len() as u32,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        if ok == 0 || bytes_returned < 28 {
            return None;
        }
        let removable = buffer[10] != 0;
        let vendor_off = u32::from_le_bytes(buffer[12..16].try_into().ok()?);
        let product_off = u32::from_le_bytes(buffer[16..20].try_into().ok()?);
        let serial_off = u32::from_le_bytes(buffer[24..28].try_into().ok()?);
        let bus = if bytes_returned as usize > 28 {
            buffer[28]
        } else {
            0
        };
        Some(StorageInquiry {
            vendor: cstr_at(&buffer, vendor_off),
            product: cstr_at(&buffer, product_off),
            serial: cstr_at(&buffer, serial_off),
            bus_type: bus_type_string(bus),
            removable,
        })
    }

    fn ioctl_media_type(handle: windows_sys::Win32::Foundation::HANDLE) -> Option<String> {
        let mut geometry_ex = [0u8; 64];
        let mut bytes_returned = 0u32;
        let ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                std::ptr::null(),
                0,
                geometry_ex.as_mut_ptr() as *mut _,
                geometry_ex.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        if ok == 0 || bytes_returned < 12 {
            return None;
        }
        // DISK_GEOMETRY.MediaType is ULONG at offset 8.
        let media = u32::from_le_bytes(geometry_ex[8..12].try_into().ok()?);
        Some(
            match media {
                11 => "Removable", // RemovableMedia
                12 => "Fixed",     // FixedMedia
                _ => "Unknown",
            }
            .to_string(),
        )
    }

    fn ioctl_disk_size(handle: windows_sys::Win32::Foundation::HANDLE) -> Option<u64> {
        let mut geometry_ex = [0u8; 64];
        let mut bytes_returned = 0u32;
        let ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                std::ptr::null(),
                0,
                geometry_ex.as_mut_ptr() as *mut _,
                geometry_ex.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        if ok != 0 && bytes_returned >= 32 {
            Some(u64::from_le_bytes(
                geometry_ex[24..32].try_into().unwrap_or([0; 8]),
            ))
        } else {
            None
        }
    }

    fn ioctl_drive_layout(
        handle: windows_sys::Win32::Foundation::HANDLE,
    ) -> Option<(String, Vec<PartitionEntry>)> {
        let mut capacity = 512usize;
        for _ in 0..8 {
            let mut buffer = vec![0u8; capacity];
            let mut bytes_returned = 0u32;
            let ok = unsafe {
                windows_sys::Win32::System::IO::DeviceIoControl(
                    handle,
                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                    std::ptr::null(),
                    0,
                    buffer.as_mut_ptr() as *mut _,
                    buffer.len() as u32,
                    &mut bytes_returned,
                    std::ptr::null_mut(),
                )
            };
            if ok == 0 {
                let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
                // ERROR_INSUFFICIENT_BUFFER = 122
                if code == 122 {
                    capacity *= 2;
                    continue;
                }
                return None;
            }
            if bytes_returned < DRIVE_LAYOUT_HEADER_SIZE as u32 {
                return None;
            }
            let partition_style = u32::from_le_bytes(buffer[0..4].try_into().ok()?);
            let partition_count = u32::from_le_bytes(buffer[4..8].try_into().ok()?) as usize;
            let style = match partition_style {
                PARTITION_STYLE_MBR => "Basic",
                PARTITION_STYLE_GPT => "GPT",
                _ => "RAW",
            }
            .to_string();

            let mut partitions = Vec::new();
            let mut data_number = 0u32;
            for entry_index in 0..partition_count {
                let offset =
                    DRIVE_LAYOUT_HEADER_SIZE + entry_index * PARTITION_INFORMATION_EX_SIZE;
                if offset + PARTITION_INFORMATION_EX_SIZE > bytes_returned as usize {
                    break;
                }
                let entry = &buffer[offset..offset + PARTITION_INFORMATION_EX_SIZE];
                let starting_offset = i64::from_le_bytes(entry[8..16].try_into().ok()?) as u64;
                let partition_length = i64::from_le_bytes(entry[16..24].try_into().ok()?) as u64;
                // Skip empty/extended MBR containers (Ext2Mgr).
                if partition_style == PARTITION_STYLE_MBR {
                    let mbr_type = entry[32];
                    if mbr_type == PARTITION_ENTRY_UNUSED
                        || mbr_type == PARTITION_EXTENDED
                        || mbr_type == PARTITION_XINT13_EXTENDED
                    {
                        continue;
                    }
                }
                if partition_length == 0 {
                    continue;
                }
                data_number += 1;
                let partition_type = if partition_style == PARTITION_STYLE_MBR {
                    mbr_partition_type_name(entry[32]).to_string()
                } else if partition_style == PARTITION_STYLE_GPT {
                    // Ext2Mgr uses Gpt.Name when set; otherwise "GPT".
                    // Name is UTF-16; keep printable ASCII only. If empty/unknown,
                    // fall back to PartitionType GUID English label.
                    let name_off = 32 + 40; // type GUID 16 + id 16 + attrs 8
                    let from_name = ascii_display_name(
                        &wide_chars_to_string(&entry[name_off..name_off + 72]),
                    );
                    if !from_name.is_empty() {
                        from_name
                    } else if let Some(type_name) = gpt_partition_type_name(&entry[32..48]) {
                        type_name.to_string()
                    } else {
                        "GPT".to_string()
                    }
                } else {
                    "RAW".to_string()
                };

                partitions.push(PartitionEntry {
                    number: data_number,
                    style: style.clone(),
                    partition_type,
                    starting_offset,
                    length: partition_length,
                    letters: Vec::new(),
                    filesystem: String::new(),
                    codepage: String::new(),
                    total_bytes: Some(partition_length),
                    used_bytes: None,
                    volume_index: None,
                    symlink: String::new(),
                    win32_volume_name: String::new(),
                });
            }
            return Some((style, partitions));
        }
        None
    }

    fn link_partitions_to_volumes(disks: &mut [DiskEntry], volumes: &[VolumeEntry]) {
        for disk in disks.iter_mut() {
            for partition in disk.partitions.iter_mut() {
                for (volume_index, volume) in volumes.iter().enumerate() {
                    if volume.extents.iter().any(|extent| {
                        extent.disk_number == disk.index
                            && extent.starting_offset == partition.starting_offset
                            && extent.extent_length == partition.length
                    }) {
                        partition.letters = volume.letters.clone();
                        partition.filesystem = volume.filesystem.clone();
                        partition.codepage = volume.codepage.clone();
                        partition.total_bytes = volume.total_bytes.or(partition.total_bytes);
                        partition.used_bytes = volume.used_bytes;
                        partition.volume_index = Some(volume_index);
                        partition.symlink = volume.symlink.clone();
                        partition.win32_volume_name = volume.win32_volume_name.clone();
                        break;
                    }
                }
            }
        }
    }

    /// Open PhysicalDrive with read access (layout IOCTLs may use access=0).
    fn open_physical_drive_readable(
        disk_index: u32,
    ) -> Option<windows_sys::Win32::Foundation::HANDLE> {
        let path = format!(r"\\.\PhysicalDrive{disk_index}");
        let wide = to_wide(&path);
        const GENERIC_READ: u32 = 0x8000_0000;
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                GENERIC_READ,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            None
        } else {
            Some(handle)
        }
    }

    fn probe_filesystem_at_disk_offset(
        disk_index: u32,
        starting_offset: u64,
    ) -> Option<(String, Option<[u8; 16]>)> {
        let handle = open_physical_drive_readable(disk_index)?;
        let mut first = vec![0u8; 4096];
        let ok = read_at(handle, starting_offset, &mut first);
        if !ok {
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            return None;
        }
        if let Some(result) = crate::disk::fs_probe::probe_first_page(&first) {
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            return Some(result);
        }
        let mut btrfs = vec![0u8; 4096];
        if read_at(handle, starting_offset + BTRFS_SUPER_BLOCK_OFFSET, &mut btrfs) {
            if let Some(name) = crate::disk::fs_probe::probe_btrfs_page(&btrfs) {
                unsafe {
                    windows_sys::Win32::Foundation::CloseHandle(handle);
                }
                return Some((name, None));
            }
        }
        let mut raid = vec![0u8; 4096];
        if read_at(handle, starting_offset + RAID_SUPER_BLOCK_OFFSET, &mut raid) {
            if let Some(name) = crate::disk::fs_probe::probe_raid_page(&raid) {
                unsafe {
                    windows_sys::Win32::Foundation::CloseHandle(handle);
                }
                return Some((name, None));
            }
        }
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        None
    }

    fn fill_missing_partition_filesystems(disks: &mut [DiskEntry]) {
        for disk in disks.iter_mut() {
            for partition in disk.partitions.iter_mut() {
                if !partition.filesystem.is_empty()
                    && !partition.filesystem.eq_ignore_ascii_case("RAW")
                {
                    continue;
                }
                let Some((filesystem, uuid)) =
                    probe_filesystem_at_disk_offset(disk.index, partition.starting_offset)
                else {
                    continue;
                };
                partition.filesystem = filesystem;
                if let Some(harddisk_volume) = find_harddisk_volume_for_extent(
                    disk.index,
                    partition.starting_offset,
                    partition.length,
                ) {
                    partition.symlink = harddisk_volume;
                } else if partition.symlink.is_empty() {
                    // Display-only fallback — never use for DefineDosDevice / Session Manager.
                    partition.symlink = format!(
                        r"\Device\Harddisk{}\Partition{}",
                        disk.index, partition.number
                    );
                }
                let _ = uuid; // applied when synthesizing the volume row
            }
        }
    }

    /// Map a partition extent to `\Device\HarddiskVolumeN` (required by Ext2Fsd /
    /// Ext2Mgr for DefineDosDevice and Session Manager). Never invent
    /// `HarddiskN\PartitionM` as a mount target — those "succeed" in registry and
    /// leave Explorer-dead letters.
    fn find_harddisk_volume_for_extent(
        disk_number: u32,
        starting_offset: u64,
        extent_length: u64,
    ) -> Option<String> {
        for index in 1u32..128 {
            let open = format!(r"\\?\GLOBALROOT\Device\HarddiskVolume{index}");
            let Some(extents) = volume_extents_from_path(&open) else {
                continue;
            };
            if extents.iter().any(|extent| {
                extent.disk_number == disk_number
                    && extent.starting_offset == starting_offset
                    && extent.extent_length == extent_length
            }) {
                return Some(format!(r"\Device\HarddiskVolume{index}"));
            }
        }
        None
    }

    /// Restore volume rows for EXT partitions that only existed via orphan letters.
    fn append_volumes_for_unlinked_ext_partitions(
        disks: &[DiskEntry],
        volumes: &mut Vec<VolumeEntry>,
    ) {
        for disk in disks {
            for partition in &disk.partitions {
                if partition.volume_index.is_some() {
                    continue;
                }
                let fs_upper = partition.filesystem.to_ascii_uppercase();
                if !(fs_upper.contains("EXT2")
                    || fs_upper.contains("EXT3")
                    || fs_upper.contains("EXT4"))
                {
                    continue;
                }
                let already = volumes.iter().any(|volume| {
                    volume.extents.iter().any(|extent| {
                        extent.disk_number == disk.index
                            && extent.starting_offset == partition.starting_offset
                            && extent.extent_length == partition.length
                    })
                });
                if already {
                    continue;
                }
                let Some(physical) = find_harddisk_volume_for_extent(
                    disk.index,
                    partition.starting_offset,
                    partition.length,
                ) else {
                    continue;
                };
                let uuid = probe_filesystem_at_disk_offset(disk.index, partition.starting_offset)
                    .and_then(|(_, uuid)| uuid);
                volumes.push(VolumeEntry {
                    letters: Vec::new(),
                    volume_kind: "Basic".to_string(),
                    filesystem: partition.filesystem.clone(),
                    total_bytes: partition.total_bytes,
                    used_bytes: None,
                    codepage: partition.codepage.clone(),
                    physical_object: physical.clone(),
                    symlink: physical,
                    win32_volume_name: partition.win32_volume_name.clone(),
                    extents: vec![DiskExtent {
                        disk_number: disk.index,
                        starting_offset: partition.starting_offset,
                        extent_length: partition.length,
                    }],
                    uuid,
                });
            }
        }
    }

    fn build_disk_rows(disks: &[DiskEntry]) -> Vec<DiskRow> {
        let mut rows = Vec::new();
        for (disk_index, disk) in disks.iter().enumerate() {
            rows.push(DiskRow::DiskHeader { disk_index });
            for partition_index in 0..disk.partitions.len() {
                rows.push(DiskRow::Partition {
                    disk_index,
                    partition_index,
                });
            }
            if disk_index + 1 < disks.len() {
                rows.push(DiskRow::Spacer);
            }
        }
        rows
    }

    fn volume_from_guid_name(name_buf: &[u16]) -> Option<VolumeEntry> {
        let win32_volume_name = wide_to_string(name_buf);
        if win32_volume_name.is_empty() {
            return None;
        }
        let symlink = win32_volume_name_to_symlink(&win32_volume_name);
        let letters = volume_drive_letters(&win32_volume_name);
        let extents = volume_extents(&win32_volume_name).unwrap_or_default();
        let (filesystem, total_bytes, used_bytes, uuid) =
            detect_filesystem_and_sizes(&win32_volume_name, &letters);
        // Physical object column: NT `\Device\HarddiskVolumeN` (Ext2Mgr volume->Name).
        // Win32 `\\?\Volume{guid}` paths belong in properties/detail, not this column.
        let physical_object =
            nt_device_name_for_volume(&win32_volume_name, &letters).unwrap_or_default();

        Some(VolumeEntry {
            letters,
            volume_kind: "Basic".to_string(),
            filesystem,
            total_bytes,
            used_bytes,
            codepage: String::new(),
            physical_object,
            symlink,
            win32_volume_name,
            extents,
            uuid,
        })
    }

    /// `\Device\HarddiskVolumeN` (or empty). Never returns `\??\Volume{guid}`.
    /// Ext2Mgr uses CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME; QueryDosDevice on the
    /// Volume{guid} DOS name is the FindFirstVolume-compatible equivalent.
    fn nt_device_name_for_volume(win32_volume_name: &str, letters: &[char]) -> Option<String> {
        if let Some(letter) = letters.first() {
            if let Some(path) = query_dos_device_name(&format!("{letter}:")) {
                if path.starts_with(r"\Device\") {
                    return Some(path);
                }
            }
        }

        // \\?\Volume{GUID}\ -> Volume{GUID}
        let trimmed = win32_volume_name.trim_end_matches('\\');
        if let Some(volume_dos) = trimmed
            .strip_prefix(r"\\?\")
            .or_else(|| trimmed.strip_prefix(r"\\\\?\\"))
        {
            if let Some(path) = query_dos_device_name(volume_dos) {
                if path.starts_with(r"\Device\") {
                    return Some(path);
                }
            }
        }

        let open_path = trimmed.to_string();
        let wide = to_wide(&open_path);
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                0,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            return None;
        }
        let mut buffer = vec![0u16; 512];
        let written = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetFinalPathNameByHandleW(
                handle,
                buffer.as_mut_ptr(),
                buffer.len() as u32,
                windows_sys::Win32::Storage::FileSystem::VOLUME_NAME_NT,
            )
        };
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        if written == 0 || written as usize >= buffer.len() {
            return None;
        }
        let path = wide_to_string(&buffer[..written as usize]);
        if path.starts_with(r"\Device\") {
            Some(path)
        } else {
            None
        }
    }

    fn win32_volume_name_to_symlink(win32_name: &str) -> String {
        let trimmed = win32_name.trim_end_matches('\\');
        if let Some(rest) = trimmed.strip_prefix(r"\\?\") {
            format!(r"\??\{rest}")
        } else {
            trimmed.to_string()
        }
    }

    fn volume_drive_letters(win32_volume_name: &str) -> Vec<char> {
        // API requires a trailing backslash on the Win32 volume name.
        let mut name = win32_volume_name.to_string();
        if !name.ends_with('\\') {
            name.push('\\');
        }
        let wide_name = to_wide(&name);
        let mut needed = 0u32;
        let probe_ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetVolumePathNamesForVolumeNameW(
                wide_name.as_ptr(),
                std::ptr::null_mut(),
                0,
                &mut needed,
            )
        };
        // First call normally fails with ERROR_MORE_DATA and sets needed.
        if needed == 0 {
            let _ = probe_ok;
            return Vec::new();
        }
        let mut paths = vec![0u16; needed as usize];
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetVolumePathNamesForVolumeNameW(
                wide_name.as_ptr(),
                paths.as_mut_ptr(),
                paths.len() as u32,
                &mut needed,
            )
        };
        if ok == 0 {
            return Vec::new();
        }
        let mut letters = Vec::new();
        let mut offset = 0usize;
        while offset < paths.len() {
            let end = paths[offset..]
                .iter()
                .position(|&unit| unit == 0)
                .map(|index| offset + index)
                .unwrap_or(paths.len());
            if end == offset {
                break;
            }
            let path = String::from_utf16_lossy(&paths[offset..end]);
            let mut chars = path.chars();
            if let (Some(drive_letter), Some(':')) = (chars.next(), chars.next()) {
                if drive_letter.is_ascii_alphabetic() {
                    letters.push(drive_letter.to_ascii_uppercase());
                } else if drive_letter.is_ascii_digit() {
                    letters.push(drive_letter);
                }
            }
            offset = end + 1;
        }
        letters.sort_unstable();
        letters.dedup();
        letters
    }

    /// Prefer GetVolumeNameForVolumeMountPoint (reliable for all lettered drives).
    /// Fall back to NT path / extent compare like Ext2QueryVolumeDrvLetters.
    fn attach_letters_like_ext2mgr(volumes: &mut [VolumeEntry]) {
        // Resolve `\Device\HarddiskVolumeN` before letter matching so DefineDosDevice
        // letters (no Win32 volume name) can join the FindFirstVolume row.
        for volume in volumes.iter_mut() {
            if volume.physical_object.is_empty() {
                if let Some(path) =
                    nt_device_name_for_volume(&volume.win32_volume_name, &volume.letters)
                {
                    volume.physical_object = path;
                }
            }
        }

        let logical_mask =
            unsafe { windows_sys::Win32::Storage::FileSystem::GetLogicalDrives() };

        for letter_index in 0u32..26 {
            if logical_mask & (1u32 << letter_index) == 0 {
                continue;
            }
            let letter = char::from(b'A' + letter_index as u8);
            let root = format!(r"{letter}:\");
            if let Some(mount_volume) = volume_name_for_mount_point(&root) {
                let mount_norm = normalize_volume_name(&mount_volume);
                for volume in volumes.iter_mut() {
                    if normalize_volume_name(&volume.win32_volume_name) == mount_norm
                        && !volume.letters.contains(&letter)
                    {
                        volume.letters.push(letter);
                    }
                }
                continue;
            }

            // Removable / Ext2Srv letters may lack a Win32 volume name mapping.
            let dos_name = format!("{letter}:");
            let Some(target) = query_dos_device_name(&dos_name) else {
                continue;
            };
            if target.is_empty() {
                continue;
            }
            let letter_extents = volume_extents_from_path(&format!(r"\\.\{letter}:"));
            for volume in volumes.iter_mut() {
                let mut matched = false;
                if !volume.physical_object.is_empty()
                    && target.eq_ignore_ascii_case(&volume.physical_object)
                {
                    matched = true;
                }
                if !matched {
                    if let (Some(letter_extents), true) =
                        (letter_extents.as_ref(), !volume.extents.is_empty())
                    {
                        if extents_equal(letter_extents, &volume.extents) {
                            matched = true;
                        }
                    }
                }
                if matched && !volume.letters.contains(&letter) {
                    volume.letters.push(letter);
                    if volume.physical_object.is_empty() && target.starts_with(r"\Device\") {
                        volume.physical_object = target.clone();
                    }
                }
            }
        }

        // Digit mounts 0-9 (Ext2Mgr).
        for digit in '0'..='9' {
            let dos_name = format!("{digit}:");
            let Some(target) = query_dos_device_name(&dos_name) else {
                continue;
            };
            let letter_extents = volume_extents_from_path(&format!(r"\\.\{digit}:"));
            for volume in volumes.iter_mut() {
                let matched = (!volume.physical_object.is_empty()
                    && target.eq_ignore_ascii_case(&volume.physical_object))
                    || letter_extents
                        .as_ref()
                        .zip(Some(&volume.extents))
                        .is_some_and(|(left, right)| {
                            !right.is_empty() && extents_equal(left, right)
                        });
                if matched && !volume.letters.contains(&digit) {
                    volume.letters.push(digit);
                    if volume.physical_object.is_empty() && target.starts_with(r"\Device\") {
                        volume.physical_object = target.clone();
                    }
                }
            }
        }

        for volume in volumes.iter_mut() {
            volume.letters.sort_unstable();
            volume.letters.dedup();
        }
    }

    /// Create volume rows for drive letters that exist in GetLogicalDrives but
    /// were never returned by FindFirstVolume (typical Ext2Fsd mounts).
    /// Multiple DefineDosDevice letters for the same `\Device\HarddiskVolumeN`
    /// are merged onto one row (Ext2Mgr `DrvLetters` bitmask behavior).
    fn synthesize_volumes_for_orphan_letters(volumes: &mut Vec<VolumeEntry>) {
        let logical_mask =
            unsafe { windows_sys::Win32::Storage::FileSystem::GetLogicalDrives() };
        for letter_index in 0u32..26 {
            if logical_mask & (1u32 << letter_index) == 0 {
                continue;
            }
            let letter = char::from(b'A' + letter_index as u8);
            if volumes.iter().any(|volume| volume.letters.contains(&letter)) {
                continue;
            }
            let dos_name = format!("{letter}:");
            let Some(physical_object) = query_dos_device_name(&dos_name) else {
                continue;
            };
            if physical_object.is_empty() || !physical_object.starts_with(r"\Device\") {
                continue;
            }

            // Same NT device as an existing volume → attach letter there.
            if let Some(volume) = volumes.iter_mut().find(|volume| {
                !volume.physical_object.is_empty()
                    && volume.physical_object.eq_ignore_ascii_case(&physical_object)
            }) {
                if !volume.letters.contains(&letter) {
                    volume.letters.push(letter);
                    volume.letters.sort_unstable();
                    volume.letters.dedup();
                }
                continue;
            }

            // Same NT device as another orphan we are about to / already did
            // synthesize in this pass — merge instead of a second row.
            if let Some(volume) = volumes.iter_mut().find(|volume| {
                volume.win32_volume_name.is_empty()
                    && volume.physical_object.eq_ignore_ascii_case(&physical_object)
            }) {
                if !volume.letters.contains(&letter) {
                    volume.letters.push(letter);
                    volume.letters.sort_unstable();
                    volume.letters.dedup();
                }
                continue;
            }

            let Some(entry) = volume_from_drive_letter(letter) else {
                continue;
            };
            volumes.push(entry);
        }

        merge_volumes_sharing_physical_object(volumes);
    }

    /// Collapse rows that share the same `\Device\...` path so multi-letter
    /// DefineDosDevice mounts appear as one volume (e.g. F: and K: → HarddiskVolume6).
    fn merge_volumes_sharing_physical_object(volumes: &mut Vec<VolumeEntry>) {
        let mut index = 0usize;
        while index < volumes.len() {
            let physical = volumes[index].physical_object.clone();
            if physical.is_empty() || !physical.starts_with(r"\Device\") {
                index += 1;
                continue;
            }
            let mut other = index + 1;
            while other < volumes.len() {
                if volumes[other]
                    .physical_object
                    .eq_ignore_ascii_case(&physical)
                {
                    let mut letters = std::mem::take(&mut volumes[other].letters);
                    volumes[index].letters.append(&mut letters);
                    volumes[index].letters.sort_unstable();
                    volumes[index].letters.dedup();
                    if volumes[index].win32_volume_name.is_empty()
                        && !volumes[other].win32_volume_name.is_empty()
                    {
                        volumes[index].win32_volume_name =
                            volumes[other].win32_volume_name.clone();
                        volumes[index].symlink = volumes[other].symlink.clone();
                    }
                    if volumes[index].extents.is_empty() && !volumes[other].extents.is_empty() {
                        volumes[index].extents = volumes[other].extents.clone();
                    }
                    if volumes[index].filesystem.is_empty()
                        && !volumes[other].filesystem.is_empty()
                    {
                        volumes[index].filesystem = volumes[other].filesystem.clone();
                    }
                    if volumes[index].total_bytes.is_none() {
                        volumes[index].total_bytes = volumes[other].total_bytes;
                    }
                    if volumes[index].used_bytes.is_none() {
                        volumes[index].used_bytes = volumes[other].used_bytes;
                    }
                    if volumes[index].uuid.is_none() {
                        volumes[index].uuid = volumes[other].uuid;
                    }
                    volumes.remove(other);
                } else {
                    other += 1;
                }
            }
            index += 1;
        }
    }

    /// Build a volume entry for a lettered drive that has no Win32 Volume{GUID} name
    /// (common for Ext2Fsd / DefineDosDevice mounts).
    fn volume_from_drive_letter(letter: char) -> Option<VolumeEntry> {
        let dos_name = format!("{letter}:");
        let physical_object = query_dos_device_name(&dos_name)?;
        if physical_object.is_empty() || !physical_object.starts_with(r"\Device\") {
            return None;
        }
        let open_path = format!(r"\\.\{letter}:");
        let extents = volume_extents_from_path(&open_path).unwrap_or_default();
        let (filesystem, total_bytes, used_bytes, uuid) =
            detect_filesystem_and_sizes(&open_path, &[letter]);
        Some(VolumeEntry {
            letters: vec![letter],
            volume_kind: "Basic".to_string(),
            filesystem,
            total_bytes,
            used_bytes,
            codepage: String::new(),
            physical_object,
            symlink: format!(r"\??\{}", open_path.trim_start_matches(r"\\.\")),
            win32_volume_name: String::new(),
            extents,
            uuid,
        })
    }

    fn normalize_volume_name(name: &str) -> String {
        name.trim_end_matches('\\').to_ascii_uppercase()
    }

    fn volume_name_for_mount_point(root_path: &str) -> Option<String> {
        let wide = to_wide(root_path);
        let mut buffer = vec![0u16; 128];
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetVolumeNameForVolumeMountPointW(
                wide.as_ptr(),
                buffer.as_mut_ptr(),
                buffer.len() as u32,
            )
        };
        if ok == 0 {
            None
        } else {
            Some(wide_to_string(&buffer))
        }
    }

    fn extents_equal(left: &[DiskExtent], right: &[DiskExtent]) -> bool {
        if left.len() != right.len() {
            return false;
        }
        left.iter().zip(right.iter()).all(|(left_extent, right_extent)| {
            left_extent.disk_number == right_extent.disk_number
                && left_extent.starting_offset == right_extent.starting_offset
                && left_extent.extent_length == right_extent.extent_length
        })
    }

    fn volume_extents(win32_volume_name: &str) -> Option<Vec<DiskExtent>> {
        let open_path = win32_volume_name.trim_end_matches('\\').to_string();
        volume_extents_from_path(&open_path)
    }

    fn volume_extents_from_path(open_path: &str) -> Option<Vec<DiskExtent>> {
        let wide = to_wide(open_path);
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                0,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            return None;
        }

        let mut capacity = 256usize;
        for _ in 0..6 {
            let mut buffer = vec![0u8; capacity];
            let mut bytes_returned = 0u32;
            let ok = unsafe {
                windows_sys::Win32::System::IO::DeviceIoControl(
                    handle,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    std::ptr::null(),
                    0,
                    buffer.as_mut_ptr() as *mut _,
                    buffer.len() as u32,
                    &mut bytes_returned,
                    std::ptr::null_mut(),
                )
            };
            if ok == 0 {
                let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
                if code == 122 {
                    capacity *= 2;
                    continue;
                }
                unsafe {
                    windows_sys::Win32::Foundation::CloseHandle(handle);
                }
                return None;
            }
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            if bytes_returned < 4 {
                return None;
            }
            let count = u32::from_le_bytes(buffer[0..4].try_into().ok()?) as usize;
            let mut extents = Vec::new();
            // VOLUME_DISK_EXTENTS: DWORD NumberOfDiskExtents; pad; DISK_EXTENT[n]
            for extent_index in 0..count {
                let offset = 8 + extent_index * 24;
                if offset + 24 > bytes_returned as usize {
                    break;
                }
                let disk_number =
                    u32::from_le_bytes(buffer[offset..offset + 4].try_into().ok()?);
                let starting_offset =
                    i64::from_le_bytes(buffer[offset + 8..offset + 16].try_into().ok()?) as u64;
                let extent_length =
                    i64::from_le_bytes(buffer[offset + 16..offset + 24].try_into().ok()?) as u64;
                extents.push(DiskExtent {
                    disk_number,
                    starting_offset,
                    extent_length,
                });
            }
            return Some(extents);
        }
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        None
    }

    fn detect_filesystem_and_sizes(
        win32_volume_name: &str,
        letters: &[char],
    ) -> (String, Option<u64>, Option<u64>, Option<[u8; 16]>) {
        let letter_open = letters
            .first()
            .map(|letter| format!(r"\\.\{letter}:"));
        if let Some(letter) = letters.first() {
            let root = format!(r"{letter}:\");
            if let Some((name, total, free)) = get_volume_information_and_space(&root) {
                if !name.eq_ignore_ascii_case("RAW") {
                    let used = total.saturating_sub(free);
                    let used = if name.eq_ignore_ascii_case("SWAP") {
                        0
                    } else {
                        used
                    };
                    // Still try raw probe for EXT UUID when Windows already mounted EXT.
                    let mut uuid = None;
                    if !win32_volume_name.is_empty() {
                        uuid = probe_volume_raw(win32_volume_name.trim_end_matches('\\'))
                            .and_then(|(_, uuid)| uuid);
                    }
                    if uuid.is_none() {
                        if let Some(ref open_path) = letter_open {
                            uuid = probe_volume_raw(open_path).and_then(|(_, uuid)| uuid);
                        }
                    }
                    return (name, Some(total), Some(used), uuid);
                }
            }
        }

        let open_path = if win32_volume_name.trim_end_matches('\\').is_empty() {
            letter_open.clone().unwrap_or_default()
        } else {
            win32_volume_name.trim_end_matches('\\').to_string()
        };
        if !open_path.is_empty() {
            if let Some((probed, uuid)) = probe_volume_raw(&open_path) {
                let used = if probed == "SWAP" { Some(0) } else { None };
                return (probed, None, used, uuid);
            }
        }
        if let Some(ref open_path) = letter_open {
            if open_path != &win32_volume_name.trim_end_matches('\\').to_string() {
                if let Some((probed, uuid)) = probe_volume_raw(open_path) {
                    let used = if probed == "SWAP" { Some(0) } else { None };
                    return (probed, None, used, uuid);
                }
            }
        }

        if !open_path.is_empty() {
            if let Some((name, total, free)) =
                get_volume_information_and_space(&format!(r"{}\", open_path))
            {
                let used = total.saturating_sub(free);
                return (name, Some(total), Some(used), None);
            }
        }

        ("RAW".to_string(), None, None, None)
    }

    fn get_volume_information_and_space(
        root_path: &str,
    ) -> Option<(String, u64, u64)> {
        let wide = to_wide(root_path);
        let mut fs_name = vec![0u16; 64];
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetVolumeInformationW(
                wide.as_ptr(),
                std::ptr::null_mut(),
                0,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                fs_name.as_mut_ptr(),
                fs_name.len() as u32,
            )
        };
        if ok == 0 {
            return None;
        }
        let name = wide_to_string(&fs_name);
        if name.is_empty() {
            return None;
        }

        let mut free_bytes_available = 0u64;
        let mut total_bytes = 0u64;
        let mut total_free_bytes = 0u64;
        let space_ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::GetDiskFreeSpaceExW(
                wide.as_ptr(),
                &mut free_bytes_available,
                &mut total_bytes,
                &mut total_free_bytes,
            )
        };
        if space_ok == 0 {
            Some((name, 0, 0))
        } else {
            Some((name, total_bytes, total_free_bytes))
        }
    }

    fn probe_volume_raw(volume_path: &str) -> Option<(String, Option<[u8; 16]>)> {
        let wide = to_wide(volume_path);
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                windows_sys::Win32::Storage::FileSystem::FILE_GENERIC_READ,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            return None;
        }

        let mut first = vec![0u8; 4096];
        if !read_at(handle, 0, &mut first) {
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            return None;
        }
        if let Some(result) = crate::disk::fs_probe::probe_first_page(&first) {
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            return Some(result);
        }

        let mut btrfs = vec![0u8; 4096];
        if read_at(handle, BTRFS_SUPER_BLOCK_OFFSET, &mut btrfs) {
            if let Some(name) = crate::disk::fs_probe::probe_btrfs_page(&btrfs) {
                unsafe {
                    windows_sys::Win32::Foundation::CloseHandle(handle);
                }
                return Some((name, None));
            }
        }

        let mut raid = vec![0u8; 4096];
        if read_at(handle, RAID_SUPER_BLOCK_OFFSET, &mut raid) {
            if let Some(name) = crate::disk::fs_probe::probe_raid_page(&raid) {
                unsafe {
                    windows_sys::Win32::Foundation::CloseHandle(handle);
                }
                return Some((name, None));
            }
        }

        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        None
    }

    fn read_at(
        handle: windows_sys::Win32::Foundation::HANDLE,
        offset: u64,
        buffer: &mut [u8],
    ) -> bool {
        let mut new_pos = 0i64;
        let seek_ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::SetFilePointerEx(
                handle,
                offset as i64,
                &mut new_pos,
                windows_sys::Win32::Storage::FileSystem::FILE_BEGIN,
            )
        };
        if seek_ok == 0 {
            return false;
        }
        let mut bytes_read = 0u32;
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::ReadFile(
                handle,
                buffer.as_mut_ptr(),
                buffer.len() as u32,
                &mut bytes_read,
                std::ptr::null_mut(),
            )
        };
        ok != 0 && (bytes_read as usize) > 0
    }

    fn query_dos_device_name(dos_name: &str) -> Option<String> {
        let wide = to_wide(dos_name);
        let mut target = vec![0u16; 512];
        let written = unsafe {
            windows_sys::Win32::Storage::FileSystem::QueryDosDeviceW(
                wide.as_ptr(),
                target.as_mut_ptr(),
                target.len() as u32,
            )
        };
        if written == 0 {
            None
        } else {
            Some(wide_to_string(&target))
        }
    }

    fn to_wide(value: &str) -> Vec<u16> {
        std::ffi::OsStr::new(value)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect()
    }

    fn wide_to_string(wide: &[u16]) -> String {
        let end = wide.iter().position(|&unit| unit == 0).unwrap_or(wide.len());
        String::from_utf16_lossy(&wide[..end])
    }

    fn wide_chars_to_string(bytes: &[u8]) -> String {
        let mut units = Vec::with_capacity(bytes.len() / 2);
        for chunk in bytes.chunks_exact(2) {
            let unit = u16::from_le_bytes([chunk[0], chunk[1]]);
            if unit == 0 {
                break;
            }
            units.push(unit);
        }
        String::from_utf16_lossy(&units)
    }

    /// Keep list/UI text ASCII printable; drop CJK and other non-ASCII.
    fn ascii_display_name(raw: &str) -> String {
        raw.chars()
            .filter(|ch| {
                let code = *ch as u32;
                (0x20..=0x7E).contains(&code)
            })
            .collect::<String>()
            .trim()
            .to_string()
    }

    /// Ext2Mgr `Ext2RefreshVLVI` / `Ext2RefreshDVPT` column: `EVP.Codepage`.
    /// Query via `IOCTL_APP_VOLUME_PROPERTY` (APP_CMD_QUERY_PROPERTY3); fall back to
    /// Ext2Fsd Volumes\{UUID} then global Parameters\CodePage for EXT volumes.
    fn resolve_volume_codepage(volume: &VolumeEntry) -> String {
        if let Some(codepage) = query_ext2_property_codepage(volume) {
            if !codepage.is_empty() {
                return codepage;
            }
        }
        if let Some(uuid) = volume.uuid {
            if let Some(codepage) = registry_volume_codepage(&uuid) {
                if !codepage.is_empty() {
                    return codepage;
                }
            }
        }
        let fs_upper = volume.filesystem.to_ascii_uppercase();
        if fs_upper.contains("EXT") {
            if let Some(codepage) = registry_global_codepage() {
                if !codepage.is_empty() {
                    return codepage;
                }
            }
            // Ext2Fsd volumes always have a codepage concept; match driver default.
            return "default".to_string();
        }
        String::new()
    }

    fn query_ext2_property_codepage(volume: &VolumeEntry) -> Option<String> {
        if !ext2fsd_device_present() {
            return None;
        }
        for open_path in volume_ext2_open_paths(volume) {
            if let Some(codepage) = ioctl_query_ext2_codepage(&open_path) {
                return Some(codepage);
            }
        }
        None
    }

    fn volume_ext2_open_paths(volume: &VolumeEntry) -> Vec<String> {
        let mut paths = Vec::new();
        if let Some(letter) = volume.letters.first() {
            paths.push(format!(r"\\.\{letter}:"));
        }
        if !volume.physical_object.is_empty() {
            // Ext2Mgr opens \Device\HarddiskVolumeN via ZwCreateFile.
            // Win32 equivalent: \\?\GLOBALROOT\Device\HarddiskVolumeN
            let trimmed = volume.physical_object.trim_start_matches('\\');
            paths.push(format!(r"\\?\GLOBALROOT\{trimmed}"));
            if let Some(rest) = volume.physical_object.strip_prefix(r"\Device\") {
                paths.push(format!(r"\\.\{rest}"));
            }
        }
        let win32 = volume.win32_volume_name.trim_end_matches('\\');
        if !win32.is_empty() {
            paths.push(win32.to_string());
        }
        paths
    }

    fn create_file_generic_read(path: &str) -> Option<windows_sys::Win32::Foundation::HANDLE> {
        const GENERIC_READ: u32 = 0x8000_0000;
        let wide = to_wide(path);
        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                GENERIC_READ,
                windows_sys::Win32::Storage::FileSystem::FILE_SHARE_READ
                    | windows_sys::Win32::Storage::FileSystem::FILE_SHARE_WRITE,
                std::ptr::null(),
                windows_sys::Win32::Storage::FileSystem::OPEN_EXISTING,
                0,
                0,
            )
        };
        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            None
        } else {
            Some(handle)
        }
    }

    fn ext2fsd_device_present() -> bool {
        // Same device Ext2Mgr opens as \DosDevices\Ext2Fsd with GENERIC_READ.
        if let Some(handle) = create_file_generic_read(r"\\.\Ext2Fsd") {
            unsafe {
                windows_sys::Win32::Foundation::CloseHandle(handle);
            }
            true
        } else {
            false
        }
    }

    fn ioctl_query_ext2_codepage(open_path: &str) -> Option<String> {
        // Ext4Fsd/include/common.h — QUERY_PROPERTY is enough to fill Codepage.
        const IOCTL_APP_VOLUME_PROPERTY: u32 = 0x0022_1F40;
        const EXT2_VOLUME_PROPERTY_MAGIC: u32 = 0x4556_504D; // 'EVPM'
        const APP_CMD_QUERY_PROPERTY: u32 = 0x0000_0002;
        const CODEPAGE_OFF: usize = 16;
        const CODEPAGE_MAXLEN: usize = 0x20;
        const PROP_SIZE: usize = 48; // sizeof(EXT2_VOLUME_PROPERTY)

        let handle = create_file_generic_read(open_path)?;
        let mut buffer = vec![0u8; PROP_SIZE];
        buffer[0..4].copy_from_slice(&EXT2_VOLUME_PROPERTY_MAGIC.to_le_bytes());
        buffer[8..12].copy_from_slice(&APP_CMD_QUERY_PROPERTY.to_le_bytes());

        let mut bytes_returned = 0u32;
        let ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_APP_VOLUME_PROPERTY,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        if ok == 0 {
            return None;
        }

        let codepage = c_string_from_bytes(&buffer[CODEPAGE_OFF..CODEPAGE_OFF + CODEPAGE_MAXLEN]);
        // Driver writes "default" when no NLS table; treat empty success as default.
        Some(if codepage.is_empty() {
            "default".to_string()
        } else {
            codepage
        })
    }

    /// Ext2Mgr `Ext2ProcessExt2Property` — `EVP.DrvLetter` (low 7 bits), if set.
    pub fn query_volume_fixed_drv_letter(volume: &VolumeEntry) -> Option<char> {
        if !ext2fsd_device_present() {
            return None;
        }
        for open_path in volume_ext2_open_paths(volume) {
            if let Some(letter) = ioctl_query_ext2_drv_letter(&open_path) {
                return Some(letter);
            }
        }
        None
    }

    fn ioctl_query_ext2_drv_letter(open_path: &str) -> Option<char> {
        // EXT2_VOLUME_PROPERTY2: base 48 + UUID[16] + DrvLetter at offset 64.
        const IOCTL_APP_VOLUME_PROPERTY: u32 = 0x0022_1F40;
        const EXT2_VOLUME_PROPERTY_MAGIC: u32 = 0x4556_504D; // 'EVPM'
        const APP_CMD_QUERY_PROPERTY2: u32 = 0x0000_0004;
        const DRV_LETTER_OFF: usize = 64;
        // sizeof(EXT2_VOLUME_PROPERTY2) ≈ 132; PROPERTY3 is larger — allocate room.
        const PROP_SIZE: usize = 256;

        let handle = create_file_generic_read(open_path)?;
        let mut buffer = vec![0u8; PROP_SIZE];
        buffer[0..4].copy_from_slice(&EXT2_VOLUME_PROPERTY_MAGIC.to_le_bytes());
        buffer[8..12].copy_from_slice(&APP_CMD_QUERY_PROPERTY2.to_le_bytes());

        let mut bytes_returned = 0u32;
        let ok = unsafe {
            windows_sys::Win32::System::IO::DeviceIoControl(
                handle,
                IOCTL_APP_VOLUME_PROPERTY,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_returned,
                std::ptr::null_mut(),
            )
        };
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(handle);
        }
        if ok == 0 || (bytes_returned as usize) <= DRV_LETTER_OFF {
            return None;
        }
        let raw = buffer[DRV_LETTER_OFF] & 0x7F;
        let letter = char::from(raw);
        if letter.is_ascii_alphabetic() {
            Some(letter.to_ascii_uppercase())
        } else {
            None
        }
    }

    fn registry_volume_codepage(uuid: &[u8; 16]) -> Option<String> {
        let value_name = format_ext2_uuid(uuid);
        let key_path = r"SYSTEM\CurrentControlSet\Services\Ext2Fsd\Volumes";
        let blob = reg_query_string(key_path, &value_name)?;
        for part in blob.split(';') {
            let part = part.trim();
            if let Some(value) = part
                .strip_prefix("CodePage=")
                .or_else(|| part.strip_prefix("codepage="))
            {
                let value = value.trim();
                if !value.is_empty() {
                    return Some(value.to_string());
                }
            }
        }
        None
    }

    fn registry_global_codepage() -> Option<String> {
        reg_query_string(
            r"SYSTEM\CurrentControlSet\Services\Ext2Fsd\Parameters",
            "CodePage",
        )
    }

    fn format_ext2_uuid(uuid: &[u8; 16]) -> String {
        let mut out = String::from("{");
        for (index, byte) in uuid.iter().enumerate() {
            if index == 0 {
                out.push_str(&format!("{byte:02X}"));
            } else if index == 15 {
                out.push_str(&format!("-{byte:02X}}}"));
            } else {
                out.push_str(&format!("-{byte:02X}"));
            }
        }
        out
    }

    fn reg_query_string(key_path: &str, value_name: &str) -> Option<String> {
        let wide_path = to_wide(key_path);
        let mut key = 0;
        let status = unsafe {
            windows_sys::Win32::System::Registry::RegOpenKeyExW(
                windows_sys::Win32::System::Registry::HKEY_LOCAL_MACHINE,
                wide_path.as_ptr(),
                0,
                windows_sys::Win32::System::Registry::KEY_READ,
                &mut key,
            )
        };
        if status != 0 {
            return None;
        }
        let wide_name = to_wide(value_name);
        let mut value_type = 0u32;
        let mut data = vec![0u8; 512];
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
        if status != 0 || data_len < 2 {
            return None;
        }
        let units: Vec<u16> = data[..data_len as usize]
            .chunks_exact(2)
            .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
            .take_while(|unit| *unit != 0)
            .collect();
        let text = String::from_utf16_lossy(&units);
        if text.is_empty() {
            None
        } else {
            Some(text)
        }
    }

    fn c_string_from_bytes(bytes: &[u8]) -> String {
        let end = bytes.iter().position(|&byte| byte == 0).unwrap_or(bytes.len());
        let raw = String::from_utf8_lossy(&bytes[..end]).into_owned();
        ascii_display_name(&raw)
    }
}

pub use win_impl::*;

