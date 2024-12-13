use bindings::kalloc::{kmalloc, krealloc, kcalloc, kfree};
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
    unsafe fn realloc(&self, ptr: *mut u8, _layout: Layout, new_size: usize) -> *mut u8 {
        unsafe {
            krealloc(ptr as *mut c_void, new_size) as *mut u8
        }
    }
    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        unsafe { kcalloc(layout.size()) as *mut u8 }
    }
}
#[global_allocator]
static ALLOCATOR: KernelAllocator = KernelAllocator;
