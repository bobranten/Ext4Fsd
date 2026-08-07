/// Types included into `app` (Selection, Dialog, Message, …).


#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Selection {
    None,
    Volume(usize),
    Disk(usize),
    Partition { disk: usize, part: usize },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum ListKind {
    Volume,
    Disk,
}

/// Subset of Ext2Mgr gCodepages[] for combo-like pickers.
const CODE_PAGES: &[&str] = &[
    "default", "utf8", "cp936", "gb2312", "cp437", "cp850", "cp852", "cp866", "cp932",
    "euc-jp", "sjis", "cp949", "euc-kr", "cp950", "big5", "iso8859-1", "iso8859-15",
];

#[derive(Debug, Clone)]
pub(crate) enum Dialog {
    LetterPicker {
        replace_letter: Option<char>,
        selected_letter: Option<char>,
        persist_mode: crate::mount::ops::MountMode,
        available: Vec<char>,
        mount_symlink: String,
        mount_win32: String,
        mount_uuid: Option<[u8; 16]>,
        mount_codepage: String,
        /// Session Manager letters already mapped to this device.
        existing_session_letters: Vec<char>,
        /// Mount Manager letters already mapped to this volume GUID.
        existing_mountmgr_letters: Vec<char>,
        /// Re-open Change Drive Letters after OK (volume_index, disk_part).
        return_mount_points: Option<(Option<usize>, Option<(usize, usize)>)>,
    },
    MountPoints {
        /// Volume list index when editing via a volume (or linked partition).
        volume_index: Option<usize>,
        /// Disk list partition when there is no linked volume row.
        disk_part: Option<(usize, usize)>,
        /// Selected row in the mountpoints list (index into `letters`).
        selected: Option<usize>,
    },
    Service,
    Ext2Attrs {
        volume_index: usize,
        readonly: bool,
        codepage: String,
        fixmount: bool,
        automount: bool,
        letter: Option<char>,
        available: Vec<char>,
        hiding_prefix: String,
        hiding_suffix: String,
        uid: String,
        gid: String,
        euid: String,
    },
    DeadLetters {
        entries: Vec<crate::mount::dead_letters::DeadLetter>,
        selected: Option<usize>,
        /// When set, also clear a permanent Mount Manager assignment on remove.
        also_remove_permanent: bool,
        pending_remove: Option<crate::mount::dead_letters::DeadLetter>,
    },
    /// Modal properties pane (IDD_PROPERTY_DIALOG fields).
    Properties,
    About {
        driver_line: String,
        program_line: String,
    },
    Donate,
    PerfStat {
        rows: Vec<(String, u32, u32)>,
    },
    PartitionType {
        disk: usize,
        part: usize,
        selected_type: u8,
        note: String,
    },
}

/// Field set for the Properties dialog (mirrors IDD_PROPERTY_DIALOG).
#[derive(Debug, Clone, Default)]
struct PropertiesSnapshot {
    disk_title: String,
    vendor: String,
    product: String,
    serial: String,
    bus_type: String,
    device_type: String,
    media_type: String,
    disk_capacity_bytes: String,
    sdev_title: String,
    status: String,
    mount_points: String,
    filesystem: String,
    capacity_bytes: String,
    free_bytes: String,
    can_change_mp: bool,
    can_mount: bool,
    can_unmount: bool,
    can_ext2: bool,
}

impl PropertiesSnapshot {
    /// Copy volume/partition (lower) fields from another snapshot onto this disk snapshot.
    fn apply_sdev(&mut self, other: &Self) {
        self.sdev_title = other.sdev_title.clone();
        self.status = other.status.clone();
        self.mount_points = other.mount_points.clone();
        self.filesystem = other.filesystem.clone();
        self.capacity_bytes = other.capacity_bytes.clone();
        self.free_bytes = other.free_bytes.clone();
        self.can_change_mp = other.can_change_mp;
        self.can_mount = other.can_mount;
        self.can_unmount = other.can_unmount;
        self.can_ext2 = other.can_ext2;
    }
}

#[derive(Debug, Clone)]
pub enum Message {
    Tick,
    Refresh,
    SelectVolume(usize),
    SelectDiskRow(usize),
    SelectPrev,
    SelectNext,
    ContextVolume(usize),
    ContextDiskRow(usize),
    MenuCopyAll,
    MenuCopyItem,
    MenuAbout,
    MenuDonate,
    MenuDocumentation,
    OpenAboutWebsite,
    OpenIcedPortWebsite,
    OpenDonateSourceForge,
    OpenDonatePayPal,
    OpenPerfStat,
    OpenPartitionType,
    PartitionTypePick(u8),
    ApplyPartitionType,
    FlushSelected,
    EnableAutorun,
    DisableAutorun,
    QuickMount,
    UnmountSelected,
    OpenMountPoints,
    OpenLetterPicker { replace_letter: Option<char> },
    MountPointsSelect(usize),
    CloseDialog,
    /// Click sink on the dialog card so backdrop dismiss does not fire underneath.
    DialogAbsorbClick,
    PickLetter(char),
    SetPersistMode(crate::mount::ops::MountMode),
    ConfirmLetterPicker,
    /// Background mount/unmount finished (keeps UI responsive during Ext2Srv I/O).
    PipeOpFinished {
        ok_message: Option<String>,
        err_message: Option<String>,
        is_error: bool,
        /// Re-open Change Drive Letters after a successful Assign from that dialog.
        restore_mount_points: Option<(Option<usize>, Option<(usize, usize)>, char, String, Option<[u8; 16]>)>,
        /// Explorer shell notify on the UI thread: `(letter, arrival)`.
        explorer_notify: Vec<(char, bool)>,
        /// When false (Mount Points Remove), skip startup automount so removed
        /// letters are not immediately rebound onto this or another volume.
        run_automount: bool,
    },
    /// Ext2Mgr `Ext2ProcessExt2Volumes` finished (after load/refresh).
    AutomountFinished(crate::mount::ops::AutomountReport),
    RemoveLetter(char),
    OpenService,
    StartService,
    StopService,
    RestartService,
    SaveService,
    ServiceStartup(u32),
    ServiceReadonly(bool),
    ServiceExt3Writable(bool),
    ServiceAutomount(bool),
    ServiceCodepage(String),
    ServicePrefix(String),
    ServiceSuffix(String),
    OpenExt2Attrs,
    Ext2Readonly(bool),
    Ext2Codepage(String),
    Ext2Fixmount(bool),
    Ext2Automount(bool),
    Ext2Letter(char),
    Ext2Prefix(String),
    Ext2Suffix(String),
    Ext2Uid(String),
    Ext2Gid(String),
    Ext2Euid(String),
    SaveExt2Attrs,
    PropertiesChangeMp,
    PropertiesQuickMount,
    PropertiesUnmount,
    PropertiesExt2Info,
    OpenDeadLetters,
    DeadLettersLoaded(Vec<crate::mount::dead_letters::DeadLetter>),
    SelectDeadLetter(usize),
    DeadAlsoRemovePermanent(bool),
    RemoveSelectedDeadLetter,
    ConfirmDeadLetterRemove,
    CancelDeadLetterRemove,
    ShowProperties,
    CopyPerfStat,
    StartResize {
        list: ListKind,
        column: usize,
        cursor_x: f32,
    },
    CursorMoved(f32),
    EndResize,
    DetailEditor(text_editor::Action),
    UseSiUnits,
    UseBinaryUnits,
    DisplayWithBytes,
    DisplayWithBits,
    TogglePropertiesPane,
}
