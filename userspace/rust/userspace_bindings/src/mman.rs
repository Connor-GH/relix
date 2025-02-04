use core::ffi::{c_void, c_int};
use crate::sys_types::off_t;
pub const PROT_READ: c_int = 0x1;
pub const PROT_WRITE: c_int = 0x2;
pub const MAP_SHARED: c_int = 0x1;
pub const MMAP_FAILED: c_int = -1;

unsafe extern "C" {
    pub fn mmap(addr: *mut c_void, length: usize, prot: c_int, flags: c_int, fd: c_int, offset: off_t) -> *mut c_void;
    // Release memory mapping. You can do this even if the fd is closed!
    pub fn munmap(addr: *mut c_void, length: usize) -> c_int;
}
