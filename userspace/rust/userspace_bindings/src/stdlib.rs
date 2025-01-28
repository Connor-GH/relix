use core::ffi::{c_void, c_int, c_char};

unsafe extern "C" {
    pub fn exit(status: c_int) -> !;
    pub fn malloc(size: usize) -> *mut c_void;
    pub fn calloc(nmemb: usize, size: usize) -> *mut c_void;
    pub fn realloc(ptr: *mut c_void, size: usize) -> *mut c_void;
    pub fn free(ptr: *mut c_void);
    pub fn atoi(str: *const c_char) -> c_int;
    pub fn getenv(name: *const c_char) -> *mut c_char;
}
