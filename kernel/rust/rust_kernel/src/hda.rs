use core::ffi::c_void;
use core::fmt::Debug;

use crate::pci::PCICommonHeader;
use crate::println;
use kernel_bindings::bindings::io2v;

// Reference: Intel High Definition Audio Specification, Rev. 1.0a

#[derive(Debug)]
struct Reserved<T>(T);

struct HDAControllerRegister {
    // The virtial address of where the MMIO registers start.
    address: *mut c_void,
}
impl HDAControllerRegister {
    fn with_physical_addr(addr: usize) -> Self {
        Self {
            // SAFETY: we trust the MMIO address we are given.
            address: unsafe { io2v(addr) },
        }
    }
    fn global_capabilities(&self) -> GlobalCapabilities {
        GlobalCapabilities(unsafe { *(self.address.offset(0) as *mut u16) })
    }
    fn minor_version(&self) -> u8 {
        unsafe { *(self.address.offset(2) as *mut u8) }
    }
    fn major_version(&self) -> u8 {
        unsafe { *(self.address.offset(3) as *mut u8) }
    }
}
struct GlobalCapabilities(u16);
impl GlobalCapabilities {
    fn is_64ok(&self) -> bool {
        self.0 != 0
    }
    // Number of Serial Data Out Signals.
    // Return type is really u2.
    fn num_sdo(&self) -> Result<u8, Reserved<u8>> {
        let x = ((self.0 >> 1) & 0b11) as u8;
        match x {
            0b0 => Ok(1),
            0b1 => Ok(2),
            0b10 => Ok(4),
            _ => Err(Reserved(x)),
        }
    }
    // Number of Bidirectional Streams Supported.
    // Return type is really u5.
    fn num_bss(&self) -> Result<u8, Reserved<u8>> {
        let x = ((self.0 >> 3) & 0b11111) as u8;
        match x {
            0b11111 => Err(Reserved(x)),
            _ => Ok(x),
        }
    }
    // Number of Input Streams Supported
    // Return type is really u4.
    fn num_iss(&self) -> u8 {
        ((self.0 >> 8) & 0b1111) as u8
    }
    // Number of Output Streams Supported
    // Return type is really u4.
    fn num_oss(&self) -> u8 {
        ((self.0 >> 12) & 0b1111) as u8
    }
}

impl Debug for GlobalCapabilities {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("GlobalCapabilities")
            .field("64ok", &self.is_64ok())
            .field("nsdo", &self.num_sdo())
            .field("bss", &self.num_bss())
            .field("iss", &self.num_iss())
            .field("oss", &self.num_oss())
            .finish()
    }
}

// Just a stub driver for now.
pub fn driver_init(hdr: &PCICommonHeader) {
    // The MMIO is located in BAR0.
    let controller_reg = HDAControllerRegister::with_physical_addr(hdr.bar()[0] as usize);
    println!("capabilities = {:x?}", controller_reg.global_capabilities());
    println!(
        "version {}.{}",
        controller_reg.major_version(),
        controller_reg.minor_version()
    );
}
