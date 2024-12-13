#![no_std]
pub mod allocator;
use bindings::console::consputc;
use core::ffi::c_int;
use spin::Mutex;
extern crate alloc;
use alloc::ffi::CString;

pub struct ConsoleWriter {}

/* the following 'write' implementation
 * and printing is inspired from
 * dancrossnyc/rxv64 on github
 */
impl core::fmt::Write for ConsoleWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.as_bytes() {
            unsafe { consputc(*c as c_int); }
        }
        Ok(())
    }
}
pub static CONSOLE_WRITER: Mutex<ConsoleWriter> = Mutex::new(ConsoleWriter {});


pub mod printing {
    pub use core::ffi::{c_char, CStr};
    pub use bindings::console::cprintf;
    pub use core::fmt::Write;
    extern crate alloc;
    pub use alloc::format;

    pub fn print(args: core::fmt::Arguments) {
        crate::CONSOLE_WRITER.lock().write_fmt(args).unwrap();
    }
    #[macro_export]
    macro_rules! print {
        ($($arg:tt)*) => ($crate::printing::print(format_args!($($arg)*)))
    }
    #[macro_export]
    macro_rules! println {
        ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)))
    }
    pub use super::println;
    pub use super::print;
}

#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {
    use bindings::console::panic;
    let s = info.message().as_str().unwrap();
    let info_cstr = unsafe {
        CString::from_vec_unchecked(s.as_bytes().to_vec())
    };
    unsafe {
        panic(info_cstr.as_c_str().as_ptr());
    }
}

