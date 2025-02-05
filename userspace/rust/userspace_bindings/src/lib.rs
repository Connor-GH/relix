#![no_std]
#![feature(c_variadic)]
pub mod fcntl;
pub mod stdio;
pub mod stdlib;
pub mod unistd;
pub mod string;
pub mod mman;
pub mod sys_types;
pub mod ioctl;
pub mod pci;

extern crate alloc;
use alloc::ffi::CString;
use alloc::format;
use alloc::string::ToString;
use stdio::{putc, fprintf, stdout};
use stdlib::{exit, malloc, free};
use spin::Mutex;



#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {

    println!("{info}");
    unsafe {
        exit(1);
    }
}

pub struct ConsoleWriter {}

impl core::fmt::Write for ConsoleWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.as_bytes() {
            unsafe {
                putc(*c as core::ffi::c_int);
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
mod printing {
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::printing::print(format_args!($($arg)*)))
}
#[macro_export]
macro_rules! println {
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)))
}
pub use super::print;
pub use super::println;
}
use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;

pub struct UserspaceAllocator;

unsafe impl GlobalAlloc for UserspaceAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { malloc(layout.size()) as *mut u8 }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe {
            free(ptr as *mut c_void);
        }
    }
}
#[global_allocator]
static ALLOCATOR: UserspaceAllocator = UserspaceAllocator;
