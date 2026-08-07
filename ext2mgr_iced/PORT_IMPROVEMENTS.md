# Ext2Mgr Iced port — what is better than the original

Compares the Rust + [Iced](https://iced.rs/) manager in `ext2mgr_iced/` with classic MFC **Ext2 Volume Manager** (`Ext2Mgr/`). Manager/GUI only.

Shared service / classic-manager fixes that are **not** iced-only live in [`../Ext2Mgr/IMPROVEMENTS.md`](../Ext2Mgr/IMPROVEMENTS.md).

Upstream tracker for context: [bobranten/Ext4Fsd issues](https://github.com/bobranten/Ext4Fsd/issues).

---

## UX improvements

### Windowing and dialogs

| Behavior | Original (MFC) | Iced port |
|----------|----------------|-----------|
| Internal dialogs | `DoModal()` disables the parent | In-window overlays; main window stays interactive |
| Move main window while a dialog is open | Blocked | Works |
| Minimize | Hide to tray (`ShowWindow(SW_HIDE)` + notify icon) | Same tray-hide path (`win_chrome` subclass on `SC_MINIMIZE`) |
| Stability | More frequent GUI hangs/crashes in practice | Crashes much less often in day-to-day use |

### Disk / volume information

| Feature | Original | Iced port |
|---------|----------|-----------|
| Vendor / Product / Serial / Bus | On `IDD_PROPERTY_DIALOG` when storage inquiry succeeds | Same fields; inquiry stored on `DiskEntry` |
| Free space | Properties uses `FILE_FS_SIZE_INFORMATION` (`FssInfo`); often `0` when that query fails | Free space in the **properties pane and tables** from enumerated capacity/used and, when lettered, `GetDiskFreeSpaceExW` |
| Disk identity in the list | Generic disk strip; [#42](https://github.com/bobranten/Ext4Fsd/issues/42) asked for model/name | Disk header + Properties vendor/product; accent-colored DISK rule |
| Narrow columns | Text can clip awkwardly | Cell text truncates with an ellipsis (`…`) when the column is skinnier than the content |

### Theming (Windows accent)

Live Settings / DWM accent via `win_accent` applied to:

- Selection highlight
- DISK header underline
- Primary buttons

Original uses classic Win32 highlight colors.

### View menu

- Show / hide the **properties pane**
- Capacity labels: **SI** vs **binary**
- Display sizes as **bytes** or **bits**

### Mount / letters

| Feature | Original | Iced port |
|---------|----------|-----------|
| Assign Drive Letter | Shown for many unlettered volumes; mount still goes through Ext2 stack | **Grayed / omitted** unless the selection is EXT2/3/4 and has no letter (Tools, context menu, Properties Mount) — avoids a long Ext2Srv wait on NTFS/etc. |
| Device path for letter assign | `Volume->Name` = `\Device\HarddiskVolumeN` | Same — uses `physical_object` when present |
| Unmount | Letter-based remove | Tools / context / Properties when any letter is present; removes **all** letters on the selection |
| Change Drive Letters UI | Modal list + Add/Change/Remove on the right | Same layout; in-window dialog |
| Multi-letter volumes | Bitmask `DrvLetters` | Merges DefineDosDevice letters that share the same `\Device\HarddiskVolumeN` onto one row |
| Persistence modes | (1) DefineDosDevice temporary (2) Mount Manager (3) Session Manager DOS Devices registry; plus Ext2Fsd automount elsewhere | Same four options via `MountMode` enum. On startup/refresh, remounts Ext2Fsd `MountPoint=` and dormant Session Manager letters only when not already lettered / MountMgr-present (Ext2Mgr `Ext2ProcessExt2Volumes` pattern) |

**Modes are not interchangeable:** Mount Manager is `SetVolumeMountPoint`. Session Manager is the registry option. Temporary is DefineDosDevice-only. Classic Session Manager also does an immediate DefineDosDevice bind after writing the key; that bind is not the Temporary option. Device path for Temporary / Session Manager is `\Device\HarddiskVolumeN`.

Iced letter remove/assign extras vs classic: skip `DeleteVolumeMountPoint` unless Mount Manager owns the letter; Exact-match remove uses live `QueryDosDevice` (`\Device\…`); Session Manager Change same-letter is registry-only (no tear-down); automount order is Session Manager → `Volumes\{UUID}` → `EVP.DrvLetter`; Explorer notify via `WM_DEVICECHANGE` + `SHChangeNotify`.

### Remove Dead Letters

| | Original | Iced port |
|--|----------|-----------|
| Detection | Only letters with **no extent** (`drvLetter->Extent == NULL`) after subtracting live volume/part masks | (1) `QueryDosDevice` letters **not claimed** by an enumerated volume; (2) **claimed but Explorer-dead** non–Mount Manager letters (`X:\` inaccessible) — catches Session Manager / `DefineDosDevice` leftovers merged onto the same NTFS volume as a working MountMgr letter |
| Practical gap | Can **miss** real dead letters (e.g. `J:` → `\??\D:`) because that redirect still has usable geometry/extents | Lists those redirects; also lists inaccessible DosDevice siblings that volume merge would otherwise hide |
| Remove path | `DefineDosDevice` + optional MountMgr | Local `DefineDosDevice` + clear Session Manager value (no Ext2Srv round-trip); in-app Yes/No confirm (no freezing Win32 MessageBox on the iced thread) |

### Ext2Srv / errors

Temporary **EXT** mounts go through the Ext2Srv named pipe (`\\.\pipe\EXT2MGR_PSRV`).

| Behavior | Original | Iced port |
|----------|----------|-----------|
| Pipe busy / missing | `WaitNamedPipe` up to **20s**; can also try to start Ext2Srv | Busy wait capped at **2s**, then fail; MessageBox + status bar |
| Dead-letter remove | Local DOS device APIs | Local DOS device APIs (does **not** block on Ext2Srv) |
| Op failures | Mostly `AfxMessageBox` | Status bar **and** Win32 MessageBox (stop/warning), with iced confirms where modality would freeze the UI |

Pipe-server speedups (dual idle listeners, softer create retries, no `FILE_FLAG_WRITE_THROUGH`) are in **Ext2Srv** and benefit both GUIs — see [`../Ext2Mgr/IMPROVEMENTS.md`](../Ext2Mgr/IMPROVEMENTS.md).

---

## Ext2Srv / IPC — speed levers

`DefineDosDevice` inside Ext2Srv is cheap. The slow path was **getting onto the pipe on the UI thread**, then connecting again for the next op.

### Why two idle listeners (not one)

Historically Ext2Srv did: create one pipe → `ConnectNamedPipe` → hand off to a client thread → **then** create the next listener. Between accept and the next `CreateNamedPipe`, a second client (or a reconnect) could hit `ERROR_PIPE_BUSY` and wait.

| Listeners | Behavior |
|-----------|----------|
| **1** (old) | Fine for a single quiet client. Overlap → `PIPE_BUSY` / wait. |
| **2** (current) | One instance accepting while another stays pre-created; covers the handoff gap. |
| **3+** | Diminishing returns unless several clients talk at once. |

### Lever list

| # | Where | Lever | Status |
|---|--------|--------|------|
| 1 | Ext2Srv | **Dual idle listeners** | Done — second `Ext2PipeEngine` thread in `Ext2StartPipeSrv` |
| 2 | Ext2Srv | **Fail soft on create** | Done — create retries capped (3×50ms) instead of long sleep storms |
| 3 | iced | **Keep-alive client** | Done — `pipe::with_shared_client` reuses one handle |
| 4 | iced | **Off UI thread** | Partial — mount/unmount (incl. Mount Points **Remove**), dead-letter scan via `spawn_blocking`; still sync on UI: disk refresh/`enumerate_all`, Ext2 attr save, service start/stop, and modal `MessageBox` |
| 5 | iced / policy | **Local `DefineDosDevice` when elevated** | Done — try local first when elevated, else Ext2Srv |
| 6 | iced | **Letter assign device path = `\Device\HarddiskVolumeN`** | Done — use `physical_object` like Ext2Mgr `Volume->Name` for Temporary DefineDosDevice and Session Manager registry values |
| 7 | iced + Ext2Srv | **Health ping** | Done — `pipe::health_check` / Refresh status uses reused connection |
| 8 | Ext2Srv | **Drop `FILE_FLAG_WRITE_THROUGH`** | Done — removed on create; **re-measured after rebuild/reinstall** (elevated pipe probe, 2026-07-28): cold connect median **0.7 ms** (first ~21 ms), burst reconnect median **0.2 ms**, overlap-while-held median **0.2 ms**, `QUERY_DRV C:` median **0.1 ms** (first ~22 ms). Query-only probe (no letter assign/remove). |

Already done earlier: **2s** busy-pipe cap (vs original **20s**), EXT-only Assign gating, dead-letter remove without Ext2Srv.

### Other polish

- Native Win32 menu chrome with selection-synced enable/disable
- Clipboard helpers
- Persistence modes documented as distinct options (Temporary DefineDosDevice vs Session Manager registry vs Ext2Fsd automount; shared device path `\Device\HarddiskVolumeN`)
- HKCU Run autorun for the iced binary (optional; separate from original service `-service -hide`)

### Intentionally not ported

- **Format** — original dialog is largely a stub / destructive; left out on purpose

---

## Upstream issues this port may help (manager-side)

| Issue | Topic | Notes |
|-------|--------|--------|
| [#42](https://github.com/bobranten/Ext4Fsd/issues/42) | Drive model/name in Ext2Mgr | Vendor/product/serial/bus and clearer disk identity |
| [#46](https://github.com/bobranten/Ext4Fsd/issues/46) | Dead drive letter after reformat/uninstall | Dead-letter UI lists more candidates than stock Ext2Mgr (see above) |
| [#79](https://github.com/bobranten/Ext4Fsd/issues/79) | Free space display | Manager display can be more useful when Win32 size APIs work; Explorer still depends on the driver |

---

## Summary

The iced port’s main wins are **non-blocking dialogs**, **Windows accent theming**, **View menu options**, **more reliable free-space / disk-inquiry display**, **broader dead-letter detection** (including inaccessible non–Mount Manager leftovers), **EXT-only Assign gating**, **Ext2Mgr-compatible `\Device\HarddiskVolumeN` letter targets**, **startup/refresh automount with already-mounted checks**, faster fail when the Ext2Srv pipe is unavailable, and **generally better stability**.
