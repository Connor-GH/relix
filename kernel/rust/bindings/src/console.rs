use core::panic::PanicInfo;
use core::ffi::{c_void,c_char};
unsafe extern "C" {
    pub fn cprintf(fmt: *const c_char, ...) -> c_void;
    pub fn panic(fmt: *const c_char) -> !;
}
#[panic_handler]
fn rs_panic(info: &PanicInfo) -> ! {
    unsafe {
        panic(info.message().as_str().unwrap().as_ptr() as *const c_char);
    }
}

