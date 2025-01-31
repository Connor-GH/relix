use core::arch::asm;

pub unsafe fn outb(value: u8, port: u16) {
    asm!(
        "outb %al, %dx",
        in("al") value,
        in("dx") port,
        options(att_syntax));
}

pub unsafe fn outl(value: u32, port: u16) {
    asm!(
        "outl %eax, %dx",
        in("eax") value,
        in("dx") port,
        options(att_syntax));
}

pub unsafe fn inb(port: u16) -> u8 {
    let data: u8;
    asm!(
        "inb %dx, %al",
        in("dx") port,
        out("al") data,
        options(att_syntax));
    return data;
}

pub unsafe fn inl(port: u16) -> u32 {
    let data: u32;
    asm!(
        "inl %dx, %eax",
        in("dx") port,
        out("eax") data,
        options(att_syntax));
    return data;
}


