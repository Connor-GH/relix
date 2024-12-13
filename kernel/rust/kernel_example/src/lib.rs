#![no_std]
use rust_kernel::printing::*;

extern crate alloc;
use alloc::{vec, vec::Vec};

#[no_mangle]
pub extern "C" fn rust_hello_world() {

    println!("hello {} {}", "world", "from rust!");
    let v: Vec<u32> = vec![1, 2, 3];
    assert!(&1 == v.get(0).unwrap());
    assert!(&2 == v.get(1).unwrap());
    assert!(&3 == v.get(2).unwrap());
}
