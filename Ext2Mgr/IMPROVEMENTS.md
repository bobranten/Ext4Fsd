# Ext2Mgr / Ext2Srv — classic stack improvements

Fixes and service changes that apply to the **regular** MFC **Ext2 Volume Manager** (`Ext2Mgr/`) and/or **Ext2Srv**, independent of the Iced port.

Rebuild Ext2Mgr / reinstall Ext2Srv (`Scripts\build.ps1`, `Scripts\install_driver.ps1`) to pick these up. Optional Iced GUI comparisons live on the `ext2mgr-iced` branch under `ext2mgr_iced/PORT_IMPROVEMENTS.md`.

---

## Ext2Mgr (`Ext2Mgr/`)

### Change Drive Letters — crash on non-EXT / Session Manager

**Symptom:** Choosing **Session Manager DOS Devices** (or temporary DOS devices) for an NTFS/FAT partition could crash Ext2Mgr.

**Cause:** `CMountPoints::AddMountPoint` in `MountPoints.cpp`:

1. On successful Session Manager registry write it called `EndDialog(0)` **without returning**, so execution continued.
2. It always ran `Ext2QueryExt2Property(Handle, EVP)` to store Ext2Fsd automount properties.
3. For a **partition-selected** native volume, `EVP` stayed **NULL** (only set from `m_Volume` or an EXT `m_Part->Volume` branch) → null dereference.

**Fix:**

- `return TRUE` immediately after a successful Session Manager assign + `EndDialog`.
- If `EVP` is NULL, **skip** the Ext2 property IOCTL path and only assign the DOS letter (`Ext2AssignDrvLetter`), then update letter masks / notify.

**Note:** Classic assign remains `DefineDosDevice` to `\Device\HarddiskVolumeN` (same path Session Manager uses). This fix is **crash safety** only.

---

## Ext2Srv (`Ext2Srv/`)

These pipe-server changes help **both** Ext2Mgr and `ext2mgr_iced` (temporary EXT letter ops via `\\.\pipe\EXT2MGR_PSRV`).

| Change | File | What |
|--------|------|------|
| Dual idle pipe listeners | `Ext2Pipe.cpp` (`Ext2StartPipeSrv`) | Second `Ext2PipeEngine` thread so reconnect / overlap rarely hits `ERROR_PIPE_BUSY` |
| Soft create retries | `Ext2Pipe.cpp` (`Ext2PipeEngine`) | Cap create failures at 3×50ms (+ short sleep) instead of long backoff storms |
| No `FILE_FLAG_WRITE_THROUGH` | `Ext2Pipe.cpp` (`Ext2CreatePipe`) | Dropped on tiny IPC messages; measured after reinstall (2026-07-28): cold connect median ~0.7 ms, burst reconnect ~0.2 ms, `QUERY_DRV` ~0.1 ms (query-only probe) |

---

## Intentionally not changed here

| Topic | Status |
|-------|--------|
| Mount Manager for NTFS in classic Ext2Mgr | Not needed for Explorer when Session Manager / DosDevice targets `\Device\HarddiskVolumeN`. Classic UI still forces DosDev (`#if TRUE` in `MountPoints.cpp`). |
| EXT-only “Assign Drive Letter” gating | Iced-only (classic still offers assign more broadly and can wait on Ext2Srv). |
| Dead-letter detection breadth | Iced lists more orphans; classic logic unchanged. |

---

## Related docs

- [`../README.md`](../README.md) — fork build/install scripts and driver changelog
- `ext2mgr_iced/` on branch `ext2mgr-iced` — optional Iced GUI port
