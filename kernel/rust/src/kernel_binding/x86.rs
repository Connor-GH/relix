#[repr(C)]
pub struct TrapFrame {
    pub edi: u32,
    pub esi: u32,
    pub ebp: u32,
    pub oesp: u32, // Ignored
    pub ebx: u32,
    pub edx: u32,
    pub ecx: u32,
    pub eax: u32,

    pub gs: u16,
    pub padding1: u16,
    pub fs: u16,
    pub padding2: u16,
    pub es: u16,
    pub padding3: u16,
    pub ds: u16,
    pub padding4: u16,
    pub trapno: u32,

    pub err: u32,
    pub eip: u32,
    pub cs: u16,
    pub padding5: u16,
    pub eflags: u32,
    pub esp: u32,
    pub ss: u16,
    pub padding6: u16,
}
