use core::ffi::c_char;
use super::proc::Cpu;
#[repr(C)]
pub struct Spinlock {
    pub locked: u32,       // Is the lock held?
    pub name: *mut c_char, // Name of lock
    pub cpu: *mut Cpu,     // CPU holding the lock
    pub pcs: [u32; 10],    // Call stack (program counters) that locked the lock
}
