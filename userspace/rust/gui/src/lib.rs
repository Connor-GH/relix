#![no_std]
use core::ffi::{c_uint, c_int, c_void};
use userspace_bindings::stdio::{FILE, fclose, fflush, fopen, fprintf};
use userspace_bindings::fcntl::{open, O_RDWR};
use userspace_bindings::unistd::close;
use userspace_bindings::mman::{mmap, munmap, MMAP_FAILED, PROT_READ, PROT_WRITE, MAP_SHARED};

const LIBGUI_BUFFER_SIZE: usize = 960;
pub struct Rectangle {
    pub x: u32,
    pub y: u32,
    pub xlen: u32,
    pub ylen: u32,
}
type Point = (u32, u32);
pub struct FrameBuffer<const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize> {
    fd: c_int,
    ptr: *mut c_void,
}

impl<const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize>
    FrameBuffer<WIDTH, HEIGHT, DEPTH>
{
    fn new() -> Option<Self> {
        let fd = unsafe { open(c"/dev/fb0".as_ptr(), O_RDWR)};
        if fd == -1 {
            None
        } else {
            let ptr = unsafe { mmap(core::ptr::null_mut(),
            0, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0) };
            Some(FrameBuffer { fd, ptr })
        }
    }
    fn write_pixel(&mut self, p: Point, color: u32) {
        libgui_pixel_write_ptr(self.ptr, p.0, p.1, color);
    }
}

impl<const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize> Drop
    for FrameBuffer<WIDTH, HEIGHT, DEPTH>
{
    fn drop(&mut self) {
        unsafe {
            close(self.fd);
            munmap(self.ptr, WIDTH * HEIGHT * DEPTH);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn libgui_pixel_write_ptr(ptr: *mut c_void, x: u32, y: u32, color: u32) -> isize {
    unsafe {
        let ptr = ptr as *mut u32;
        *ptr.offset((y * 640 + x) as isize) = color;
    }
    return 0;
}

#[unsafe(no_mangle)]
pub extern "C" fn libgui_pixel_write(x: u32, y: u32, color: u32) -> isize {
    let fb = FrameBuffer::<640, 480, 32>::new();
    if fb.is_none() {
        return -1;
    }
    let mut fb = fb.unwrap();
    fb.write_pixel((x, y), color);

    return 0;
}
#[unsafe(no_mangle)]
pub extern "C" fn libgui_init(file: *const core::ffi::c_char) -> *mut c_void {
    let fd = unsafe { open(file, O_RDWR) };
    if fd == -1 {
        return core::ptr::null_mut();
    }
    /*
     * SAFETY:
     * C handles it when the pointer is null.
     */
    unsafe {
        let ptr = mmap(core::ptr::null_mut(),
            640 * 480 * 32, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        ptr
    }

}
#[unsafe(no_mangle)]
pub extern "C" fn libgui_fini(ptr: *mut c_void) {
    unsafe {
        munmap(ptr, 640 * 480 * 32);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn libgui_fill_rect(rect: *const Rectangle, hex_color: u32) {
    let fd = unsafe { open(c"/dev/fb0".as_ptr(), O_RDWR)};
    let ptr = unsafe { mmap(core::ptr::null_mut(),
        640 * 480 * 32, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0) };

    libgui_fill_rect_ptr(ptr, rect, hex_color);
    unsafe {
        close(fd);
        munmap(ptr, 640 * 480 * 32)
    };
}
#[unsafe(no_mangle)]
pub extern "C" fn libgui_fill_rect_ptr(ptr: *mut c_void, rect: *const Rectangle, hex_color: u32) {
    for x in 0..(unsafe { &*rect }).xlen {
        for y in 0..(unsafe { &*rect }).ylen {
            libgui_pixel_write_ptr(
                ptr,
                x + (unsafe { &*rect }).x,
                y + (unsafe { &*rect }).x,
                hex_color,
            );
        }
    }
}
