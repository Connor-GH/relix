#![no_std]
#![feature(c_variadic)]
#![feature(ptr_metadata)]
#![feature(layout_for_ptr)]
#![allow(non_snake_case, non_upper_case_globals)]
#![allow(non_camel_case_types)]
pub mod bindings;
use self::bindings::printf;
use self::bindings::putchar;
use self::bindings::{exit, free, malloc};
use spin::Mutex;

#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {
    //println!("{info}");
    unsafe {
        printf(c"Rust panic [no other information]\n".as_ptr());
    }
    unsafe {
        exit(1);
    }
}

pub struct ConsoleWriter {}

impl core::fmt::Write for ConsoleWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.as_bytes() {
            unsafe {
                putchar((*c) as core::ffi::c_int);
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
pub mod printing {
    #[macro_export]
    #[deprecated]
    macro_rules! print {
        ($($arg:tt)*) => ($crate::print(format_args!($($arg)*)))
    }
    #[macro_export]
    #[deprecated]
    macro_rules! println {
        ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)))
    }
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
