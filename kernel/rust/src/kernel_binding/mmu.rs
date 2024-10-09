use core::ffi::c_void;
pub const NSEGS: usize = 6;
#[repr(C)]
pub struct SegDesc {
    unused: [u8; 64], // Placeholder for segment descriptor
}

#[repr(C)]
pub struct TaskState {
    link: u32,     // Old ts selector
    esp0: u32,     // Stack pointers and segment selectors
    ss0: u16,      // After an increase in privilege level
    padding1: u16, // Padding
    esp1: *mut u32,
    ss1: u16,
    padding2: u16,
    esp2: *mut u32,
    ss2: u16,
    padding3: u16,
    cr3: *mut c_void, // Page directory base
    eip: *mut u32,    // Saved state from last task switch
    eflags: u32,
    eax: u32, // More saved state (registers)
    ecx: u32,
    edx: u32,
    ebx: u32,
    esp: *mut u32,
    ebp: *mut u32,
    esi: u32,
    edi: u32,
    es: u16, // More saved state (segment selectors)
    padding4: u16,
    cs: u16,
    padding5: u16,
    ss: u16,
    padding6: u16,
    ds: u16,
    padding7: u16,
    fs: u16,
    padding8: u16,
    gs: u16,
    padding9: u16,
    ldt: u16,
    padding10: u16,
    t: u16,    // Trap on task switch
    iomb: u16, // I/O map base address
}
