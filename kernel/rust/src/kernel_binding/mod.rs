#![no_std]
#![feature(c_variadic)]

use core::panic::PanicInfo;
pub mod console;
pub mod kalloc;
pub mod kernel_assert;
pub mod mmu;
pub mod param;
pub mod proc;
pub mod sleeplock;
pub mod spinlock;
pub mod x86;

#[panic_handler]
fn rs_panic(_info: &PanicInfo) -> ! {
    unsafe {
        console::panic(c"rust panic".as_ptr());
    }
}
