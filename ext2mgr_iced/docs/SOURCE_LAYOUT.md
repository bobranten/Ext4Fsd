# Source layout

How `ext2mgr_iced/src` is organized. Entry point is thin; code is grouped by concern.

## Top level

```text
src/
  main.rs          Binary entry only (iced::application + window icon)
  disk/            Volume / disk enumeration and FS probes
  mount/           Letters, Ext2Srv pipe, persistence, dead letters
  service/         Ext2Fsd service + global properties
  win/             Win32 chrome helpers (menu, accent, clipboard, …)
  ui/              Iced GUI state, tables, dialogs, update loop
```

`main.rs` wires modules and starts the app. It does not own UI or mount logic.

| Module | Crate path | Role |
|--------|------------|------|
| `disk` | `crate::disk` | List volumes/disks/partitions; probe EXT and other FS signatures |
| `mount` | `crate::mount` | Assign/remove letters, pipe I/O, registry persist, dead-letter scan |
| `service` | `crate::service` | Start/stop Ext2Fsd; read/write global driver settings |
| `win` | `crate::win` | Native menu/tray/context menu, accent color, autorun, clipboard, MessageBox |
| `ui` | `crate::ui` | `Ext2MgrApp` — widgets, dialogs, keyboard/menu → `Message` handling |

Windows-only modules (`mount`, `service`, `win`) are `#[cfg(windows)]` from `main.rs`.

## `disk/`

| File | Purpose |
|------|---------|
| `enum_disk.rs` | Find volumes, disk layout, partition↔volume join, list row model |
| `fs_probe.rs` | Raw superblock / signature probes (EXT, XFS, …) |

## `mount/`

| File | Purpose |
|------|---------|
| `pipe.rs` | Ext2Srv named-pipe client (DefineDosDevice via service) |
| `ops.rs` | `MountMode`, mount/unmount, automount-on-refresh |
| `persist.rs` | Session Manager DOS Devices + Ext2Fsd `Volumes\{UUID}` registry |
| `dead_letters.rs` | Detect/remove orphan or inaccessible drive letters |

**Mount modes** (do not conflate): Temporary (`DefineDosDevice`), Mount Manager (`SetVolumeMountPoint`), Session Manager (registry DOS Devices), Ext2Fsd automount (`Volumes\{UUID}`). See root [`README.md`](../README.md).

## `service/`

| File | Purpose |
|------|---------|
| `mgr.rs` | Query/start/stop Ext2Fsd; global readonly / automount / codepage / hiding |

## `win/`

| File | Purpose |
|------|---------|
| `chrome.rs` | Native menu bar, tray, `TrackPopupMenu`, HWND helpers |
| `accent.rs` | Windows accent color for selection / primary UI |
| `autorun.rs` | HKCU Run autostart for this binary |
| `clipboard.rs` | Copy list text to clipboard |
| `msgbox.rs` | Stop/warning/confirm MessageBox (owner = main HWND) |

## `ui/`

Iced front-end. Files are pulled into **one** Rust module with `include!` so `Dialog` / `Message` fields and `impl Ext2MgrApp` methods stay visible across files (separate `mod` subcrates would force `pub` on every dialog field).

| File | Purpose |
|------|---------|
| `mod.rs` | `Ext2MgrApp` struct, `new` / title / theme / subscription; `include!`s below |
| `types.rs` | `Selection`, `Dialog`, `Message`, `PropertiesSnapshot` |
| `helpers.rs` | Reload, letter picker gating, detail text, column autofit, mount helpers |
| `view.rs` | Volume/disk tables + dialog overlays + shell layout |
| `update.rs` | `update` / message handling (keyboard, dialogs, native menu commands) |
| `gating_probe.rs` | Windows-only test probe for automount gating |

## Related docs

- [`../README.md`](../README.md) — build, persistence modes, status
- [`../PORT_IMPROVEMENTS.md`](../PORT_IMPROVEMENTS.md) — iced vs classic Ext2Mgr
- [`../../Ext2Mgr/IMPROVEMENTS.md`](../../Ext2Mgr/IMPROVEMENTS.md) — classic manager / Ext2Srv fixes
