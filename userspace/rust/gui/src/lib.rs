#![no_std]
use core::ffi::c_uint;
use userspace_bindings::stdio::{FILE, fclose, fflush, fopen, fprintf};

pub struct Rectangle {
    pub x: u32,
    pub y: u32,
    pub xlen: u32,
    pub ylen: u32,
}
type Point = (u32, u32);
pub struct FrameBuffer<'a, const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize> {
    fp: &'a mut FILE,
}

impl<const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize>
    FrameBuffer<'_, WIDTH, HEIGHT, DEPTH>
{
    fn new() -> Option<Self> {
        let ptr = &unsafe { fopen(c"/dev/fb0".as_ptr(), c"w".as_ptr()) };
        if ptr.is_null() {
            None
        } else {
            Some(FrameBuffer { fp: unsafe { &mut **ptr } })
        }
    }
    fn write_pixel(&mut self, p: Point, color: u32) {
        libgui_pixel_write_fp(self.fp, p.0, p.1, color);
    }
}

impl<const WIDTH: usize, const HEIGHT: usize, const DEPTH: usize> Drop
    for FrameBuffer<'_, WIDTH, HEIGHT, DEPTH>
{
    fn drop(&mut self) {
        unsafe { fclose(self.fp ); }
    }
}

fn libgui_pixel_write_fp(fp: *mut FILE, x: u32, y: u32, color: u32) -> isize {
    let mut buf: [u8; 13] = [0; 13]; // 4 bytes * 3 + nul
    buf[0] = x as u8;
    buf[1] = (x >> 8) as u8;
    buf[2] = (x >> 16) as u8;
    buf[3] = (x >> 24) as u8;
    buf[4] = y as u8;
    buf[5] = (y >> 8) as u8;
    buf[6] = (y >> 16) as u8;
    buf[7] = (y >> 24) as u8;
    buf[8] = color as u8;
    buf[9] = (color >> 8) as u8;
    buf[10] = (color >> 16) as u8;
    buf[11] = (color >> 24) as u8;
    // So that printf thinks it's a string and strlen works.
    // Someone is bound to treat this as a string.
    buf[12] = b'\0';
    // The kernel reads in bytes from /dev/fb0 to do pixel drawing.
    // The format is [u8; 13] because the u32's are chopped up into
    // little endian u8's.
    unsafe {
        fprintf(
            fp,
            c"%c%c%c%c%c%c%c%c%c%c%c%c".as_ptr(),
            buf[0] as c_uint,
            buf[1] as c_uint,
            buf[2] as c_uint,
            buf[3] as c_uint,
            buf[4] as c_uint,
            buf[5] as c_uint,
            buf[6] as c_uint,
            buf[7] as c_uint,
            buf[8] as c_uint,
            buf[9] as c_uint,
            buf[10] as c_uint,
            buf[11] as c_uint,
        );
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
pub extern "C" fn libgui_fill_rect(rect: *const Rectangle, hex_color: u32) {
    let fp: *mut FILE = unsafe { fopen(c"/dev/fb0".as_ptr(), c"w".as_ptr()) };
    for x in 0..(unsafe { &*rect }).xlen {
        for y in 0..(unsafe { &*rect }).ylen {
            libgui_pixel_write_fp(
                fp,
                x + (unsafe { &*rect }).x,
                y + (unsafe { &*rect }).x,
                hex_color,
            );
        }
    }
    unsafe { fclose(fp) };
}
