use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;
use kernel_bindings::bindings::{kfree, kmalloc};

pub struct KernelAllocator;

unsafe impl GlobalAlloc for KernelAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { kmalloc(layout.size()) as *mut u8 }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe {
            kfree(ptr as *mut c_void);
        }
    }
}
#[global_allocator]
static ALLOCATOR: KernelAllocator = KernelAllocator;
