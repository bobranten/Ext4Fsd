//! Raw filesystem probes matching Ext2Mgr `Ext2QueryVolumeFS`.

pub const SUPER_BLOCK_OFFSET: usize = 0x400;
pub const EXT4_SUPER_MAGIC: u16 = 0xEF53;
pub const EXT4_FEATURE_COMPAT_HAS_JOURNAL: u32 = 0x0004;
pub const EXT4_FEATURE_INCOMPAT_RECOVER: u32 = 0x0004;
pub const EXT4_FEATURE_INCOMPAT_JOURNAL_DEV: u32 = 0x0008;
pub const EXT4_FEATURE_INCOMPAT_EXTENTS: u32 = 0x0040;
pub const EXT4_FEATURE_INCOMPAT_SUPP: u32 = 0x0002
    | 0x0004
    | 0x0010
    | 0x0040
    | 0x0080
    | 0x0200
    | 0x2000
    | 0x20000;

pub const SWAP_MAGIC_OFFSET: usize = 0x1000 - 10;
pub const SWAP_HEADER_MAGIC_V1: &[u8; 10] = b"SWAP-SPACE";
pub const SWAP_HEADER_MAGIC_V2: &[u8; 10] = b"SWAPSPACE2";

pub const XFS_SB_MAGIC_LE: u32 = 0x4253_4658;
pub const LVM_MAGIC: &[u8; 8] = b"LABELONE";
pub const BSD_DISKMAGIC: u32 = 0x8256_4557;
pub const BSD_MAGIC_OFFSET: usize = 512;

pub const BTRFS_SUPER_BLOCK_OFFSET: u64 = 0x1_0000;
pub const BTRFS_MAGIC_OFFSET: usize = 64;
pub const BTRFS_MAGIC: u64 = 0x4D5F_5366_5248_425F;

pub const RAID_SUPER_BLOCK_OFFSET: u64 = 0x1000;
pub const RAID_MAGIC: u32 = 0xa92b_4efc;

/// Classify from the first 4096-byte page (EXT/SWAP/XFS/LVM/BSD).
/// Returns (fs name, optional EXT UUID from superblock).
pub fn probe_first_page(buffer: &[u8]) -> Option<(String, Option<[u8; 16]>)> {
    if buffer.len() < SUPER_BLOCK_OFFSET + 0x64 {
        return None;
    }

    let magic = u16::from_le_bytes([
        buffer[SUPER_BLOCK_OFFSET + 0x38],
        buffer[SUPER_BLOCK_OFFSET + 0x39],
    ]);
    if magic == EXT4_SUPER_MAGIC {
        let feature_compat = u32::from_le_bytes(
            buffer[SUPER_BLOCK_OFFSET + 0x5C..SUPER_BLOCK_OFFSET + 0x60]
                .try_into()
                .ok()?,
        );
        let feature_incompat = u32::from_le_bytes(
            buffer[SUPER_BLOCK_OFFSET + 0x60..SUPER_BLOCK_OFFSET + 0x64]
                .try_into()
                .ok()?,
        );
        let mut uuid = [0u8; 16];
        uuid.copy_from_slice(&buffer[SUPER_BLOCK_OFFSET + 0x68..SUPER_BLOCK_OFFSET + 0x78]);
        return Some((classify_ext(feature_compat, feature_incompat), Some(uuid)));
    }

    if buffer.len() >= SWAP_MAGIC_OFFSET + 10 {
        let swap_magic = &buffer[SWAP_MAGIC_OFFSET..SWAP_MAGIC_OFFSET + 10];
        if swap_magic == SWAP_HEADER_MAGIC_V1 || swap_magic == SWAP_HEADER_MAGIC_V2 {
            return Some(("SWAP".to_string(), None));
        }
    }

    if let Ok(bytes) = buffer[0..4].try_into() {
        if u32::from_le_bytes(bytes) == XFS_SB_MAGIC_LE {
            return Some(("XFS".to_string(), None));
        }
    }

    for sector in 0..4 {
        let start = sector * 512;
        if buffer.len() >= start + 8 && &buffer[start..start + 8] == LVM_MAGIC {
            return Some(("LVM".to_string(), None));
        }
    }

    if buffer.len() >= BSD_MAGIC_OFFSET + 4 {
        if let Ok(bytes) = buffer[BSD_MAGIC_OFFSET..BSD_MAGIC_OFFSET + 4].try_into() {
            if u32::from_le_bytes(bytes) == BSD_DISKMAGIC {
                return Some(("BSD".to_string(), None));
            }
        }
    }

    None
}

pub fn probe_btrfs_page(buffer: &[u8]) -> Option<String> {
    if buffer.len() < BTRFS_MAGIC_OFFSET + 8 {
        return None;
    }
    let magic = u64::from_le_bytes(
        buffer[BTRFS_MAGIC_OFFSET..BTRFS_MAGIC_OFFSET + 8]
            .try_into()
            .ok()?,
    );
    if magic == BTRFS_MAGIC {
        Some("BTRFS".to_string())
    } else {
        None
    }
}

pub fn probe_raid_page(buffer: &[u8]) -> Option<String> {
    if buffer.len() < 4 {
        return None;
    }
    let magic = u32::from_le_bytes(buffer[0..4].try_into().ok()?);
    if magic == RAID_MAGIC {
        Some("RAID".to_string())
    } else {
        None
    }
}

fn classify_ext(feature_compat: u32, feature_incompat: u32) -> String {
    if feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS != 0 {
        if feature_incompat & !EXT4_FEATURE_INCOMPAT_SUPP != 0 {
            "EXT4+".to_string()
        } else {
            "EXT4".to_string()
        }
    } else if feature_incompat
        & (EXT4_FEATURE_INCOMPAT_JOURNAL_DEV | EXT4_FEATURE_INCOMPAT_RECOVER)
        != 0
        || feature_compat & EXT4_FEATURE_COMPAT_HAS_JOURNAL != 0
    {
        "EXT3".to_string()
    } else {
        "EXT2".to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn detects_ext_magic_layout() {
        let mut buffer = vec![0u8; 4096];
        buffer[SUPER_BLOCK_OFFSET + 0x38] = 0x53;
        buffer[SUPER_BLOCK_OFFSET + 0x39] = 0xEF;
        // extents incompat
        buffer[SUPER_BLOCK_OFFSET + 0x60] = 0x40;
        assert_eq!(
            probe_first_page(&buffer).map(|(name, _)| name).as_deref(),
            Some("EXT4")
        );
    }

    #[test]
    fn detects_swap_magic() {
        let mut buffer = vec![0u8; 4096];
        buffer[SWAP_MAGIC_OFFSET..SWAP_MAGIC_OFFSET + 10]
            .copy_from_slice(SWAP_HEADER_MAGIC_V2);
        assert_eq!(
            probe_first_page(&buffer).map(|(name, _)| name).as_deref(),
            Some("SWAP")
        );
    }
}
