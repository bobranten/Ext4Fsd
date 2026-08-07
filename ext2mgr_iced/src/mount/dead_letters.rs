//! Dead / orphan drive letter detection and removal.
//!
//! Matches Ext2Mgr `CDelDeadLetter` / `Ext2RemoveMountPoint` / `Ext2RemoveDosSymLink`:
//! remove via `DefineDosDevice` locally — do **not** block on the Ext2Srv pipe
//! (that freezes the GUI when Ext2Srv is slow/absent, and is wrong for redirects
//! like `J:` → `\??\D:`).
//!
//! Mount Manager membership uses `GetVolumePathNamesForVolumeNameW` on the volume
//! GUID — never `GetVolumeNameForVolumeMountPoint` / `GetFileAttributes` on `X:\`,
//! which can hang for minutes on a bad DefineDosDevice bind.
//!
//! Also lists Session Manager DOS Devices letters that are not currently live in
//! `QueryDosDevice` (dormant registry leftovers beside another working letter —
//! e.g. `T:` → `\Device\HarddiskVolume5` while `F:` already serves that device).

#![cfg(windows)]

use crate::disk::enum_disk::VolumeEntry;
use std::collections::HashSet;
use std::os::windows::ffi::OsStrExt;

#[derive(Debug, Clone)]
pub struct DeadLetter {
    pub letter: char,
    pub symlink: String,
    /// True when removal might need a Mount Manager clear (orphan).
    /// False for temporary DefineDosDevice leftovers beside a live MountMgr letter,
    /// and for Session Manager registry-only dormants (registry is always cleared).
    pub may_have_permanent: bool,
}

/// Letters that QueryDosDevice resolves but are orphans or Explorer-unusable leftovers,
/// plus Session Manager registry letters with no matching live DOS device.
pub fn find_dead_letters(volumes: &[VolumeEntry]) -> Vec<DeadLetter> {
    let mut claimed = HashSet::new();
    for volume in volumes {
        for letter in &volume.letters {
            claimed.insert(letter.to_ascii_uppercase());
        }
    }

    // MountMgr letters for each Win32 volume name (GUID path) — does not open X:\.
    let mountmgr_by_volume: Vec<HashSet<char>> = volumes
        .iter()
        .map(|volume| {
            if volume.win32_volume_name.is_empty() {
                HashSet::new()
            } else {
                mountmgr_letters_for_volume(&volume.win32_volume_name)
            }
        })
        .collect();

    let mut all_mountmgr = HashSet::new();
    for set in &mountmgr_by_volume {
        all_mountmgr.extend(set.iter().copied());
    }

    let mut dead = Vec::new();
    let mut listed = HashSet::new();

    for letter_index in 0u8..26 {
        let letter = char::from(b'A' + letter_index);
        let Some(symlink) = query_dos_device(letter) else {
            continue;
        };

        // Volume enum sometimes misses a letter that already maps to a known
        // `\Device\HarddiskVolumeN` (multi-letter Ext2 DOS mounts). That is not dead —
        // treating it as an orphan falsely listed live letters like Q: beside K:/L:.
        let matches_known_device = volumes.iter().any(|volume| {
            !volume.physical_object.is_empty()
                && volume.physical_object.eq_ignore_ascii_case(&symlink)
        });
        if matches_known_device {
            claimed.insert(letter);
        }

        if !claimed.contains(&letter) {
            // Orphan DOS mapping — may still have a stale Mount Manager assignment.
            dead.push(DeadLetter {
                letter,
                symlink,
                may_have_permanent: true,
            });
            listed.insert(letter);
            continue;
        }

        if all_mountmgr.contains(&letter) {
            continue;
        }

        // Claimed via NT-device merge but not a MountMgr path for that volume —
        // typically a Temporary/Session DefineDosDevice beside a MountMgr letter.
        let Some(volume_index) = volumes.iter().position(|volume| {
            volume
                .letters
                .iter()
                .any(|entry| entry.to_ascii_uppercase() == letter)
                || (!volume.physical_object.is_empty()
                    && volume.physical_object.eq_ignore_ascii_case(&symlink))
        }) else {
            continue;
        };
        let mountmgr = &mountmgr_by_volume[volume_index];
        let has_mountmgr_sibling = volumes[volume_index].letters.iter().any(|entry| {
            let other = entry.to_ascii_uppercase();
            other != letter && mountmgr.contains(&other)
        }) || mountmgr.iter().any(|other| *other != letter);
        if has_mountmgr_sibling {
            dead.push(DeadLetter {
                letter,
                symlink,
                may_have_permanent: false,
            });
            listed.insert(letter);
        }
    }

    // Session Manager registry letters with **no** live QueryDosDevice bind
    // (dormant leftovers, e.g. T: for HarddiskVolume5 while F: is already live).
    // Do **not** flag a registry letter that still has a live DOS bind — even if the
    // registry string compare is fussy — those are working mounts (Q:/L: on Vol7).
    for (letter, device) in super::persist::registry_dos_device_entries() {
        if listed.contains(&letter) {
            continue;
        }
        if query_dos_device(letter).is_some() {
            continue;
        }
        dead.push(DeadLetter {
            letter,
            symlink: device,
            may_have_permanent: false,
        });
        listed.insert(letter);
    }

    dead.sort_by(|left, right| left.letter.cmp(&right.letter));
    dead
}

pub fn mountmgr_letters_for_volume(win32_volume_name: &str) -> HashSet<char> {
    let mut name = win32_volume_name.to_string();
    if !name.ends_with('\\') {
        name.push('\\');
    }
    let wide_name = to_wide(&name);
    let mut needed = 0u32;
    let _ = unsafe {
        windows_sys::Win32::Storage::FileSystem::GetVolumePathNamesForVolumeNameW(
            wide_name.as_ptr(),
            std::ptr::null_mut(),
            0,
            &mut needed,
        )
    };
    if needed == 0 {
        return HashSet::new();
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
        return HashSet::new();
    }

    let mut letters = HashSet::new();
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
                letters.insert(drive_letter.to_ascii_uppercase());
            }
        }
        offset = end + 1;
    }
    letters
}

pub fn remove_dead_letter(letter: char, symlink: &str, delete_mountmgr: bool) -> Result<(), String> {
    // Registry-only dormants have no live DOS device — skip DefineDosDevice remove.
    if query_dos_device(letter).is_some() {
        remove_dos_device(letter, symlink)?;
    }

    if delete_mountmgr {
        let mount_root = format!(r"{letter}:\");
        let wide = to_wide(&mount_root);
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::DeleteVolumeMountPointW(wide.as_ptr())
        };
        if ok == 0 {
            let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
            if code != 2 && code != 3 && code != 87 {
                return Err(format!(
                    "DeleteVolumeMountPoint({letter}:) failed ({code})"
                ));
            }
        }
    }
    let _ = super::persist::clear_registry_mount_point(letter);

    if query_dos_device(letter).is_some() {
        return Err(format!(
            "Failed to remove drive letter {letter}: (still present after DefineDosDevice)"
        ));
    }
    Ok(())
}

fn remove_dos_device(letter: char, symlink: &str) -> Result<(), String> {
    const DDD_RAW_TARGET_PATH: u32 = 0x0000_0001;
    const DDD_REMOVE_DEFINITION: u32 = 0x0000_0002;
    const DDD_EXACT_MATCH_ON_REMOVE: u32 = 0x0000_0004;

    let dos = format!("{letter}:");
    let dos_w = to_wide(&dos);
    let target_w = to_wide(symlink);

    let exact_ok = unsafe {
        windows_sys::Win32::Storage::FileSystem::DefineDosDeviceW(
            DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
            dos_w.as_ptr(),
            target_w.as_ptr(),
        )
    };
    if exact_ok != 0 {
        return Ok(());
    }

    let loose_ok = unsafe {
        windows_sys::Win32::Storage::FileSystem::DefineDosDeviceW(
            DDD_REMOVE_DEFINITION,
            dos_w.as_ptr(),
            std::ptr::null(),
        )
    };
    if loose_ok != 0 {
        return Ok(());
    }

    let code = unsafe { windows_sys::Win32::Foundation::GetLastError() };
    Err(format!(
        "Failed to remove drive letter {letter}:\nDefineDosDevice failed ({code})"
    ))
}

fn query_dos_device(letter: char) -> Option<String> {
    let dos = format!("{letter}:");
    let wide = to_wide(&dos);
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
        let end = target
            .iter()
            .position(|&unit| unit == 0)
            .unwrap_or(target.len());
        Some(String::from_utf16_lossy(&target[..end]))
    }
}

fn to_wide(value: &str) -> Vec<u16> {
    std::ffi::OsStr::new(value)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect()
}
