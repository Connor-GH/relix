#![no_std]
#![feature(string_remove_matches)]
#![feature(ascii_char)]
#![feature(vec_into_raw_parts)]
#![feature(debug_closure_helpers)]
#![feature(c_size_t)]
#![feature(fn_ptr_trait)]
pub mod allocator;
pub mod ansi_escape;
pub mod file;
pub mod hda;
pub mod lock;
pub mod pci;
pub mod printing;
pub mod ubsanitizer;

extern crate alloc;

use alloc::ffi::CString;
use alloc::format;
use alloc::string::ToString;
use kernel_bindings::bindings::{panic_print_after, panic_print_before, vga_cprintf};

#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {
    let s = format!("{info}").as_str().to_string();

    let info_cstr = CString::new(s).unwrap_or_default();
    unsafe {
        panic_print_before();
        vga_cprintf(info_cstr.as_c_str().as_ptr());
        panic_print_after()
    }
}
