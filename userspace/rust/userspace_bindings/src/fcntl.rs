use core::ffi::{c_int, c_char};

pub const O_RDWR: i32 = 0x2;
unsafe extern "C" {
    pub fn open(path: *const c_char, flags: c_int) -> c_int;
}
