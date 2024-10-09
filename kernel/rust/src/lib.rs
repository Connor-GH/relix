#![no_std]
#![feature(c_size_t)]

mod kernel_binding;
use kernel_binding::console;
use crate::console::cprintf;
use core::ffi::{c_char, CStr};

fn to_cstr(s: &CStr) -> *const c_char {
    s.as_ptr()
}
fn from_str_to_cstr(s: &str) -> &'static CStr {
    let mut bytes = [0u8; 256];
    let len = s.as_bytes().len();
    bytes[..len].copy_from_slice(s.as_bytes());
    unsafe { CStr::from_ptr(bytes.as_ptr() as *const c_char) }
}

trait Equality<T> {
    fn equals(s: Self, _: T) -> bool;
}

struct KDev<'a> {
    name: &'a str,
}

impl Equality<KDev<'_>> for KDev<'_> {
    fn equals(s: Self, o: KDev) -> bool {
        s.name == o.name
    }
}

#[no_mangle]
pub extern "C" fn example_rust_binding(left: u32, right: u32) -> u32 {
    unsafe {
        cprintf(c"hello from rust!\n".as_ptr());
    }
    let s = KDev { name: "this" };
    unsafe {
        cprintf(
            c"equals test: %s\n".as_ptr(),
            if KDev::equals(s, KDev { name: "this" }) {
                to_cstr(c"true")
            } else {
                to_cstr(c"false")
            },
        )
    };
    left + right
}
