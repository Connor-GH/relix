use core::ffi::{c_void,c_char};
unsafe extern "C" {
    pub fn cprintf(fmt: *const c_char, ...) -> c_void;
    pub fn panic(fmt: *const c_char) -> !;
}
