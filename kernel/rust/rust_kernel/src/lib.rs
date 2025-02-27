#![no_std]
#![feature(string_remove_matches)]
#![feature(ascii_char)]
#![feature(vec_into_raw_parts)]
pub mod allocator;
pub mod ansi_escape;
pub mod pci;
pub mod printing;
pub mod time;
pub mod ubsanitizer;
pub mod ksyms;
pub mod ahci;

extern crate alloc;
use alloc::ffi::CString;
use alloc::format;
use alloc::string::ToString;

#[panic_handler]
fn rs_panic(info: &core::panic::PanicInfo) -> ! {
    use bindings::console::panic;

    let s = format!("{info}").as_str().to_string();

    let info_cstr = CString::new(s).unwrap_or_default();
    unsafe {
        panic(info_cstr.as_c_str().as_ptr());
    }
}
