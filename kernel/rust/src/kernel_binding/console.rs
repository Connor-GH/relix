use core::ffi::c_char;
extern "C" {
    pub fn panic(msg: *const c_char) -> !;
    pub fn cprintf(fmt: *const c_char, ...);
}
