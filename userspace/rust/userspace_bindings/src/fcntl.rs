use core::ffi;

unsafe extern "C" {
    pub fn open(path: *const i8, flags: i32) -> i32;
}
