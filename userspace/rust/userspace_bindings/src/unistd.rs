use core::ffi::{c_char, c_int, c_void};
use crate::sys_types::off_t;

unsafe extern "C" {
    pub fn fork() -> c_int;
    pub fn _exit(status: c_int) -> !;
    pub fn pipe(pipefd: *mut c_int) -> c_int;
    pub fn write(fd: c_int, buf: *const c_void, count: usize) -> isize;
    pub fn read(fd: c_int, buf: *mut c_void, count: usize) -> isize;
    pub fn close(fd: c_int) -> c_int;
    pub fn unlink(path: *const c_char) -> c_int;
    pub fn link(oldpath: *const c_char, newpath: *const c_char) -> c_int;
    pub fn symlink(target: *const c_char, linkpath: *const c_char) -> c_int;
    pub fn readlink(path: *const c_char, buf: *mut c_char, bufsize: usize) -> isize;
    pub fn chdir(path: *const c_char) -> c_int;
    pub fn dup(fd: c_int) -> c_int;
    pub fn getpid() -> c_int;
    pub fn sbrk(increment: isize) -> *mut c_void;
    pub fn sleep(seconds: u32) -> u32;
    pub fn reboot() -> c_int;
    pub fn setuid(uid: u32) -> c_int;
    pub fn getopt(argc: c_int, argv: *const *mut c_char, shortopts: *const c_char) -> c_int;
    pub fn lseek(fd: c_int, offset: off_t, whence: c_int) -> off_t;
}
