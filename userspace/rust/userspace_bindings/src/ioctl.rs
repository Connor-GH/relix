use core::ffi::{c_int, c_ulong};
use core::mem::size_of;

pub const _IOC_RW: u32 = 0b11;
pub const _IOC_RO: u32 = 0b01;
pub const _IOC_WO: u32 = 0b10;
pub const _IOC_NONE: u32 = 0;
// 8-bit magic, 2 bit rw, 14 bit size, 8 bit number (for that magic)
// 8 + 2 + 14 + 8 = 32 bits
// In other words, 255 different drivers, 255 different subcommands for each driver.
// In the future, this constant may change.
macro_rules! _IOC {
    ($drv_magic:expr, $rw:expr, $size:expr, $number:expr) => ((($drv_magic as u32) << 24)
    | ($number << 16) | ($size << 2) | $rw)
}

#[repr(C)]
#[derive(Debug)]
pub struct FbVarScreenInfo {
    pub xres: u32,
    pub yres: u32,
    pub bpp: u8,
}

pub const PCIIOCGETCONF: u32 = _IOC!('P', _IOC_RW, size_of::<crate::pci::PciConf>() as u32, 0);
pub const FBIOGET_VSCREENINFO: u32 = _IOC!('F', _IOC_RW, size_of::<FbVarScreenInfo>() as u32, 0);

unsafe extern "C" {
    pub fn ioctl(fd: c_int, request: c_ulong, ...) -> c_int;
}
