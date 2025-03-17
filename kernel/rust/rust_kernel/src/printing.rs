use bindings::uart::uartputc;
use core::ffi::c_int;
use spin::Mutex;
extern crate alloc;

pub struct ConsoleWriter {}

/* the following 'write' implementation
 * and printing is inspired from
 * dancrossnyc/rxv64 on github
 */
impl core::fmt::Write for ConsoleWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.as_bytes() {
            unsafe {
                uartputc(*c as c_int);
                //consputc(*c as c_int);
            }
        }
        Ok(())
    }
}
pub static CONSOLE_WRITER: Mutex<ConsoleWriter> = Mutex::new(ConsoleWriter {});

pub use core::fmt::Write;

pub fn print(args: core::fmt::Arguments) {
    CONSOLE_WRITER.lock().write_fmt(args).unwrap();
}
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::printing::print(format_args!($($arg)*)))
}
#[macro_export]
macro_rules! println {
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)))
}

pub fn debug(args: core::fmt::Arguments) {
    if cfg!(kernel_debug) {
        print(args);
    }
}

#[macro_export]
macro_rules! debug {
    ($($arg:tt)*) => ($crate::printing::debug(format_args!($($arg)*)))
}
#[macro_export]
macro_rules! debugln {
    ($($arg:tt)*) => ($crate::debug!("{}\n", format_args!($($arg)*)))
}
pub use super::print;
pub use super::println;
pub use super::debug;
pub use super::debugln;
