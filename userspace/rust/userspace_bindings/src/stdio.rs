use core::ffi::{c_char, VaList, c_int};

#[repr(C)]
pub struct FILE {
    pub write_buffer: *mut c_char,
    pub write_buffer_size: usize,
    pub write_buffer_index: usize,
    pub static_table_index: usize,
    pub fd: c_int,
    pub mode: c_int,
    pub eof: bool,
    pub error: bool,
}

unsafe extern "C" {
    pub static stdout: *mut FILE;
    pub fn printf(format: *const c_char, ...);
    pub fn fprintf(stream: *mut FILE, format: *const c_char, ...);
    pub fn vfprintf(stream: *mut FILE, format: *const c_char, arg: *mut VaList);
    pub fn vsprintf(s: *mut c_char, format: *const c_char, arg: *mut VaList);
    pub fn sprintf(s: *mut c_char, format: *const c_char, ...);
    pub fn fgets(s: *mut c_char, n: c_int, stream: *mut FILE) -> *mut c_char;
    pub fn getc(stream: *mut FILE) -> c_int;
    pub fn fileno(stream: *mut FILE) -> c_int;
    pub fn fputc(c: c_int, stream: *mut FILE) -> c_int;
    pub fn fflush(stream: *mut FILE) -> c_int;
    pub fn fdopen(fd: c_int, mode: *const c_char) -> *mut FILE;
    pub fn fopen(path: *const c_char, mode: *const c_char) -> *mut FILE;
    pub fn fclose(stream: *mut FILE) -> c_int;
}
pub unsafe fn putc(c: c_int) -> c_int {
    unsafe { fputc(c, stdout)}
}
