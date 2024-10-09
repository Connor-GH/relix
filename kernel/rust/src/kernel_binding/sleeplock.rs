use super::spinlock::Spinlock;
use core::ffi::c_char;
#[repr(C)]
pub struct Sleeplock {
    pub locked: u32,       // Is the lock held?
    pub lk: Spinlock,      // Spinlock protecting this sleep lock
    pub name: *mut c_char, // Name of lock
    pub pid: u32,          // Process holding lock
}
