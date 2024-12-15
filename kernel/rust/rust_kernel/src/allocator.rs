use bindings::kalloc::{kmalloc, kfree};
use core::ffi::c_void;
use core::alloc::{GlobalAlloc, Layout};

pub struct KernelAllocator;

unsafe impl GlobalAlloc for KernelAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { kmalloc(layout.size()) as *mut u8 }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe { kfree(ptr as *mut c_void); }
    }
}
#[global_allocator]
static ALLOCATOR: KernelAllocator = KernelAllocator;
