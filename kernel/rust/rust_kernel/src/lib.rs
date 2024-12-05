#![no_std]
#![feature(str_as_str)]
use bindings::console::panic;
pub use core::ffi::c_char;
pub use bindings::console::cprintf;
macro_rules! c_str {
    ($str:expr) => (CStr::from_bytes_with_nul(concat!($str, "\0").as_bytes()).unwrap().as_ptr())
}
#[macro_export]
macro_rules! print {
    ($fmt:expr, $($arg:tt)*) => (unsafe { cprintf(format_args!(concat!($fmt, "\0"), $($arg)*).as_str().unwrap().as_ptr() as *const c_char) });
}
#[macro_export]
macro_rules! println {
    ($fmt:expr, $($arg:tt)*) => (crate::print!(concat!($fmt, "\n"), $($arg)*));
}


#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {
    // println!("{}", info);
    unsafe {
        panic(info.message().as_str().unwrap().as_ptr() as *const c_char);
    }
}

