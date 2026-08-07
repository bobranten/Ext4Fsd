//! Ext2Srv named-pipe protocol (client side).
//!
//! Matches `Ext2Srv/Ext2Pipe.h` and the client usage in `Ext2Mgr/Ext2Pipe.cpp`.
//! Pipe name: `\\.\pipe\EXT2MGR_PSRV`.


use std::io;
use std::mem::size_of;
use std::sync::Mutex;

pub const PIPE_NAME: &str = r"\\.\pipe\EXT2MGR_PSRV";
pub const PIPE_REQ_MAGIC: u32 = 0xBAD0BAD8;
#[allow(dead_code)]
pub const REQ_BODY_SIZE: usize = 4096;

pub const CMD_QUERY_DRV: u32 = 0xBAD0_0001;
pub const CMD_DEFINE_DRV: u32 = 0xBAD0_0002;
pub const CMD_REMOVE_DRV: u32 = 0xBAD0_0003;

/// Windows DDD_* for DefineDosDevice (used in define/remove)
pub const DDD_RAW_TARGET_PATH: u32 = 0x1;
pub const DDD_REMOVE_DEFINITION: u32 = 0x2;
pub const DDD_EXACT_MATCH_ON_REMOVE: u32 = 0x4;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct PipeReqHeader {
    pub magic: u32,
    pub flag: u32,
    pub cmd: u32,
    pub len: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ReqQueryDrv {
    pub type_: u32,
    pub drive: u8,
    pub result: u8,
    pub symlink: u16,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ReqDefineDrv {
    pub pid: u32,
    pub flags: u32,
    pub drive: u8,
    pub result: u8,
    pub symlink: u16,
}

// REQ_REMOVE_DRV has same layout as REQ_DEFINE_DRV
pub type ReqRemoveDrv = ReqDefineDrv;

const HEADER_SIZE: usize = size_of::<PipeReqHeader>();

/// Result of querying a drive letter via the pipe.
#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct QueryDriveResult {
    pub drive: u8,
    pub type_: u32,
    pub result: bool,
    pub symlink: String,
}

/// Keep-alive pipe handle (Ext2Mgr-style `g_hPipe`), shared across mount ops.
static SHARED_PIPE: Mutex<Option<PipeClient>> = Mutex::new(None);

/// Run `op` on a reused Ext2Srv connection; drop and reconnect on I/O failure.
pub fn with_shared_client<T, F>(op: F) -> io::Result<T>
where
    F: FnOnce(&PipeClient) -> io::Result<T>,
{
    let mut guard = SHARED_PIPE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if guard.is_none() {
        *guard = Some(PipeClient::open()?);
    }
    let result = op(guard.as_ref().expect("pipe just opened"));
    if result.is_err() {
        *guard = None;
    }
    result
}

/// Drop the cached connection (e.g. after Ext2Srv restart).
#[allow(dead_code)]
pub fn invalidate_shared_client() {
    let mut guard = SHARED_PIPE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    *guard = None;
}

/// Cheap health check: reuse or open the pipe once.
pub fn health_check() -> io::Result<()> {
    with_shared_client(|_client| Ok(()))
}

/// Client handle to the Ext2Srv pipe.
pub struct PipeClient {
    handle: windows_sys::Win32::Foundation::HANDLE,
}

impl PipeClient {
    /// Connect to `\\.\pipe\EXT2MGR_PSRV`.
    ///
    /// Ext2Mgr waits up to 20s when the pipe is busy. That freezes a GUI on the
    /// UI thread, so this client uses a short busy-wait (2s) and fails fast when
    /// Ext2Srv is down or saturated — callers surface a MessageBox instead of hanging.
    pub fn open() -> io::Result<Self> {
        use std::os::windows::ffi::OsStrExt;
        let wide: Vec<u16> = std::ffi::OsStr::new(PIPE_NAME)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect();

        const GENERIC_READ: u32 = 0x8000_0000;
        const GENERIC_WRITE: u32 = 0x4000_0000;
        const OPEN_EXISTING: u32 = 3;
        // Busy-pipe wait; keep low so Assign/Unmount stay responsive.
        const PIPE_BUSY_WAIT_MS: u32 = 2_000;

        let handle = unsafe {
            windows_sys::Win32::Storage::FileSystem::CreateFileW(
                wide.as_ptr(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                std::ptr::null(),
                OPEN_EXISTING,
                0,
                0,
            )
        };

        if handle == windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE {
            const ERROR_PIPE_BUSY: u32 = 231;
            let code: u32 = unsafe { windows_sys::Win32::Foundation::GetLastError() };
            if code == ERROR_PIPE_BUSY {
                let wait_wide: Vec<u16> = std::ffi::OsStr::new(PIPE_NAME)
                    .encode_wide()
                    .chain(std::iter::once(0))
                    .collect();
                let ok = unsafe {
                    windows_sys::Win32::System::Pipes::WaitNamedPipeW(
                        wait_wide.as_ptr(),
                        PIPE_BUSY_WAIT_MS,
                    )
                };
                if ok != 0 {
                    return Self::open();
                }
            }
            return Err(io::Error::from_raw_os_error(code as i32));
        }

        Ok(Self { handle })
    }

    /// Low-level send request and read response. Used by query/define/remove.
    fn roundtrip(&self, req: &[u8]) -> io::Result<Vec<u8>> {
        let mut written: u32 = 0;
        let ok = unsafe {
            windows_sys::Win32::Storage::FileSystem::WriteFile(
                self.handle,
                req.as_ptr(),
                req.len() as u32,
                &mut written,
                std::ptr::null_mut(),
            )
        };
        if ok == 0 || written as usize != req.len() {
            return Err(io::Error::last_os_error());
        }

        let mut header = [0u8; HEADER_SIZE];
        let mut total = 0usize;
        while total < HEADER_SIZE {
            let mut n: u32 = 0;
            let ok = unsafe {
                windows_sys::Win32::Storage::FileSystem::ReadFile(
                    self.handle,
                    header[total..].as_mut_ptr(),
                    (HEADER_SIZE - total) as u32,
                    &mut n,
                    std::ptr::null_mut(),
                )
            };
            if ok == 0 {
                return Err(io::Error::last_os_error());
            }
            total += n as usize;
        }
        let h = unsafe { &*(header.as_ptr() as *const PipeReqHeader) };
        if h.len < HEADER_SIZE as u32 {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "pipe response header len too small",
            ));
        }
        let mut out = Vec::with_capacity(h.len as usize);
        out.extend_from_slice(&header);
        out.resize(h.len as usize, 0);
        let mut total = HEADER_SIZE;
        while total < h.len as usize {
            let mut n: u32 = 0;
            let ok = unsafe {
                windows_sys::Win32::Storage::FileSystem::ReadFile(
                    self.handle,
                    out[total..].as_mut_ptr(),
                    (h.len as usize - total) as u32,
                    &mut n,
                    std::ptr::null_mut(),
                )
            };
            if ok == 0 {
                return Err(io::Error::last_os_error());
            }
            total += n as usize;
        }
        Ok(out)
    }

    /// Query what a drive letter is mapped to. `drive` is 'A'..='Z' as u8.
    pub fn query_drive(&self, drive: u8) -> io::Result<QueryDriveResult> {
        let drive = drive.to_ascii_uppercase();
        let body_size = size_of::<ReqQueryDrv>();
        let mut buf = vec![0u8; HEADER_SIZE + body_size];
        let h = unsafe { &mut *(buf.as_mut_ptr() as *mut PipeReqHeader) };
        h.magic = PIPE_REQ_MAGIC;
        h.flag = 0;
        h.cmd = CMD_QUERY_DRV;
        h.len = buf.len() as u32;
        let q = unsafe { &mut *(buf[HEADER_SIZE..].as_mut_ptr() as *mut ReqQueryDrv) };
        q.drive = drive;

        let resp = self.roundtrip(&buf)?;
        if resp.len() < HEADER_SIZE + size_of::<ReqQueryDrv>() {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "query_drive response too short",
            ));
        }
        let q = unsafe { &*(resp[HEADER_SIZE..].as_ptr() as *const ReqQueryDrv) };
        let symlink = if resp.len() > HEADER_SIZE + size_of::<ReqQueryDrv>() {
            let name_start = HEADER_SIZE + size_of::<ReqQueryDrv>();
            let name_slice = &resp[name_start..];
            let end = name_slice.iter().position(|&b| b == 0).unwrap_or(name_slice.len());
            String::from_utf8_lossy(&name_slice[..end]).into_owned()
        } else {
            String::new()
        };
        Ok(QueryDriveResult {
            drive: q.drive,
            type_: q.type_,
            result: q.result != 0,
            symlink,
        })
    }

    /// Assign a drive letter to a symlink (e.g. `\??\Volume{...}`).
    pub fn define_drive(&self, drive: u8, symlink: &str) -> io::Result<bool> {
        let drive = drive.to_ascii_uppercase();
        let body_fixed = size_of::<ReqDefineDrv>();
        let name_bytes = symlink.as_bytes();
        let name_len = name_bytes.len() + 1;
        let total = HEADER_SIZE + body_fixed + name_len;
        let mut buf = vec![0u8; total];
        let h = unsafe { &mut *(buf.as_mut_ptr() as *mut PipeReqHeader) };
        h.magic = PIPE_REQ_MAGIC;
        h.flag = 0;
        h.cmd = CMD_DEFINE_DRV;
        h.len = total as u32;
        let q = unsafe { &mut *(buf[HEADER_SIZE..].as_mut_ptr() as *mut ReqDefineDrv) };
        q.pid = unsafe { windows_sys::Win32::System::Threading::GetCurrentProcessId() };
        // Ext2Mgr uses DDD_RAW_TARGET_PATH with NT/`\??\` volume paths.
        q.flags = DDD_RAW_TARGET_PATH;
        q.drive = drive;
        let name_dst = &mut buf[HEADER_SIZE + body_fixed..];
        name_dst[..name_bytes.len()].copy_from_slice(name_bytes);
        name_dst[name_bytes.len()] = 0;

        let resp = self.roundtrip(&buf)?;
        if resp.len() < HEADER_SIZE + size_of::<ReqDefineDrv>() {
            return Ok(false);
        }
        let q = unsafe { &*(resp[HEADER_SIZE..].as_ptr() as *const ReqDefineDrv) };
        Ok(q.result != 0)
    }

    /// Remove a drive letter mapping. `symlink` should match what was used when defining.
    pub fn remove_drive(&self, drive: u8, symlink: &str) -> io::Result<bool> {
        let drive = drive.to_ascii_uppercase();
        let body_fixed = size_of::<ReqRemoveDrv>();
        let name_bytes = symlink.as_bytes();
        let name_len = name_bytes.len() + 1;
        let total = HEADER_SIZE + body_fixed + name_len;
        let mut buf = vec![0u8; total];
        let h = unsafe { &mut *(buf.as_mut_ptr() as *mut PipeReqHeader) };
        h.magic = PIPE_REQ_MAGIC;
        h.flag = 0;
        h.cmd = CMD_REMOVE_DRV;
        h.len = total as u32;
        let q = unsafe { &mut *(buf[HEADER_SIZE..].as_mut_ptr() as *mut ReqRemoveDrv) };
        q.pid = unsafe { windows_sys::Win32::System::Threading::GetCurrentProcessId() };
        q.flags = DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE;
        q.drive = drive;
        let name_dst = &mut buf[HEADER_SIZE + body_fixed..];
        name_dst[..name_bytes.len()].copy_from_slice(name_bytes);
        name_dst[name_bytes.len()] = 0;

        let resp = self.roundtrip(&buf)?;
        if resp.len() < HEADER_SIZE + size_of::<ReqRemoveDrv>() {
            return Ok(false);
        }
        let q = unsafe { &*(resp[HEADER_SIZE..].as_ptr() as *const ReqRemoveDrv) };
        Ok(q.result != 0)
    }
}

impl Drop for PipeClient {
    fn drop(&mut self) {
        unsafe {
            windows_sys::Win32::Foundation::CloseHandle(self.handle);
        }
    }
}

// PipeClient is only used behind a Mutex; HANDLE is Send on Windows for this use.
unsafe impl Send for PipeClient {}

trait ToAsciiUpper {
    fn to_ascii_uppercase(self) -> u8;
}
impl ToAsciiUpper for u8 {
    fn to_ascii_uppercase(self) -> u8 {
        if (b'a'..=b'z').contains(&self) {
            self - 32
        } else {
            self
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn header_size() {
        assert_eq!(size_of::<PipeReqHeader>(), 16);
    }
    #[test]
    fn query_drv_size() {
        assert_eq!(size_of::<ReqQueryDrv>(), 8);
    }
    #[test]
    fn define_drv_size() {
        assert_eq!(size_of::<ReqDefineDrv>(), 12);
    }
}
