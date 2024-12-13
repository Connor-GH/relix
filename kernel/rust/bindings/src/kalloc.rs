use core::ffi::c_void;
extern "C" {
    pub fn kmalloc(size: usize) -> *mut c_void;
    pub fn kcalloc(size: usize) -> *mut c_void;
    pub fn krealloc(ptr: *mut c_void, size: usize) -> *mut c_void;
    pub fn kfree(ptr: *mut c_void);
}
