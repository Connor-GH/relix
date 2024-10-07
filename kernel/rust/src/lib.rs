#![no_std]
#![feature(c_size_t)]

use core::{
    ffi::{c_char, c_size_t, c_void, CStr},
    panic::PanicInfo,
};

unsafe extern "C" {
    pub fn cprintf(fmt: *const c_char, ...) -> c_void;
    pub fn panic(fmt: *const c_char) -> !;
    pub fn kmalloc(size: c_size_t) -> *mut c_void;
    pub fn kfree(ptr: *mut c_void) -> c_void;
}

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

#[panic_handler]
fn rs_panic(info: &PanicInfo) -> ! {
    unsafe {
        panic(c"rust panic".as_ptr());
    }
}
