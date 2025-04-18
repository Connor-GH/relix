#pragma once
// These are defined by the kernel linker script in kernel64.ld.
extern char __kernel_begin[]; // first address before kernel loaded from ELF file
extern char __kernel_end[]; // first address after kernel loaded from ELF file
extern char __kernel_data[];
extern char __kernel_edata[];
