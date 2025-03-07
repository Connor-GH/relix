#[allow(dead_code)]
use core::ffi::c_void;
const KERNBASE: usize = 0xFFFFFFFF80000000;
// First device virtual address
#[allow(dead_code)]
const DEVBASE: usize = 0xFFFFFFFF40000000;

const DEVSPACE: usize = 0xFD000000; // Other devices are at high addresses


pub fn v2p(a: *const c_void) -> usize {
  return a.wrapping_sub(KERNBASE) as usize;
}

pub fn p2v(a: usize) -> *const c_void
{
  return (a + KERNBASE) as *const c_void;
}


pub fn io2v(a: usize) -> *const c_void {
    ((a) + (DEVBASE - DEVSPACE)) as *const c_void
}
pub fn v2io(a: *const c_void) -> usize {
    a.wrapping_sub(DEVBASE - DEVSPACE) as usize
}
