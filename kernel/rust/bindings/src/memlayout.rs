const DEVSPACE: usize = 0xfd000000;
#[allow(dead_code)]
const KERNBASE: usize = 0xFFFFFFFF80000000;
// First device virtual address
const DEVBASE: usize = 0xFFFFFFFF40000000;

pub fn io2v<T>(a: *mut T) -> *mut T {
    unsafe { a.offset((DEVBASE - DEVSPACE) as isize) }
}


