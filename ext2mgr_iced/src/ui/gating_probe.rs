//! Disk/partition automount gating probe (Windows tests only).

    use std::io::Write;

    #[test]
    fn dump_disk_partition_automount_gating() {
        let out_path = std::env::temp_dir().join("ext2_gate_probe_out.txt");
        let mut out = std::fs::File::create(&out_path).expect("create probe out");
        macro_rules! gate_log {
            ($($arg:tt)*) => {{
                let line = format!($($arg)*);
                eprintln!("{line}");
                let _ = writeln!(out, "{line}");
            }};
        }

        let (disks, volumes, _rows) = crate::disk::enum_disk::enumerate_all();
        gate_log!("disk_count={} volume_count={}", disks.len(), volumes.len());

        for disk in &disks {
            gate_log!(
                "disk.index={} display_name={} part_count={}",
                disk.index,
                disk.display_name,
                disk.partitions.len()
            );
            for (partition_index, partition) in disk.partitions.iter().enumerate() {
                let linked = partition
                    .volume_index
                    .and_then(|volume_index| volumes.get(volume_index));
                let linked_fs = linked.map(|volume| volume.filesystem.as_str()).unwrap_or("");
                let linked_uuid = linked.and_then(|volume| volume.uuid);
                let uuid_hex = match linked_uuid {
                    Some(bytes) => bytes
                        .iter()
                        .map(|byte| format!("{byte:02X}"))
                        .collect::<Vec<_>>()
                        .join("-"),
                    None => "None".to_string(),
                };
                let letters = crate::disk::enum_disk::format_letters(&partition.letters);
                let is_ext_part = crate::mount::ops::is_ext_family(&partition.filesystem);
                let is_ext_linked = crate::mount::ops::is_ext_family(linked_fs);
                let can_assign = partition.letters.is_empty() && is_ext_part;
                let can_automount = linked_uuid.is_some();
                gate_log!(
                    "GATE disk.index={} part_idx={} part.number={} part.fs={:?} letters={} volume_index={:?} linked_fs={:?} linked_uuid={} is_ext_part={} is_ext_linked={} can_assign={} can_automount={} symlink={:?} win32={:?}",
                    disk.index,
                    partition_index,
                    partition.number,
                    partition.filesystem,
                    letters,
                    partition.volume_index,
                    linked_fs,
                    uuid_hex,
                    is_ext_part,
                    is_ext_linked,
                    can_assign,
                    can_automount,
                    partition.symlink,
                    partition.win32_volume_name,
                );
            }
        }

        let target = disks.iter().find_map(|disk| {
            if disk.index != 2 {
                return None;
            }
            disk.partitions
                .iter()
                .find(|partition| partition.number == 2)
                .map(|partition| (disk, partition))
        });
        match target {
            Some((disk, partition)) => {
                let linked = partition
                    .volume_index
                    .and_then(|volume_index| volumes.get(volume_index));
                gate_log!(
                    "TARGET_FOUND=1 disk.index={} display={} part.number={} part.fs={:?} volume_index={:?} linked_fs={:?} linked_uuid_is_some={} can_automount={} out={}",
                    disk.index,
                    disk.display_name,
                    partition.number,
                    partition.filesystem,
                    partition.volume_index,
                    linked.map(|volume| volume.filesystem.as_str()).unwrap_or(""),
                    linked.and_then(|volume| volume.uuid).is_some(),
                    linked.and_then(|volume| volume.uuid).is_some(),
                    out_path.display(),
                );
            }
            None => gate_log!(
                "TARGET_FOUND=0 disk.index=2 part.number=2 out={}",
                out_path.display()
            ),
        }
    }
