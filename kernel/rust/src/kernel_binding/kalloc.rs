use core::ffi::{c_char, c_void};
extern "C" {
    pub fn kpage_alloc() -> *mut c_char;
    pub fn kpage_free(ptr: *mut c_char);
    pub fn kmalloc(size: usize) -> *mut c_void;
    pub fn kfree(ptr: *mut c_void);
}
