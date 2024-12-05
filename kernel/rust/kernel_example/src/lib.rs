#![no_std]
use rust_kernel::*;
#[no_mangle]
pub extern "C" fn rust_hello_world() {
    println!("hello {} {}", "world", "from rust!");
}
