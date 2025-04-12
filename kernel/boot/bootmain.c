// Boot loader.
//
// Part of the boot block, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.
// bootmain() loads an ELF kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.

// Note that this code only gets run when we do not use a multiboot loader.

#include <stdint.h>
#include <elf.h>
#include "x86.h"

#define SECTSIZE 512

void
readseg(uint8_t *, uint32_t, uint32_t);

void
bootmain(void)
{
	struct Elf32_Ehdr *elf;
	struct Elf32_Phdr *ph;
	const struct Elf32_Phdr *eph;
	void (*entry)(void);

	elf = (struct Elf32_Ehdr *)0x10000; // scratch space

	// Read 1st page off disk
	readseg((uint8_t *)elf, 4096, 0);

	// Is this an ELF executable?
	if (elf->magic != ELF_MAGIC_NUMBER)
		return; // let bootasm.S handle error

	// Load each program segment (ignores ph flags).
	ph = (struct Elf32_Phdr *)((uint8_t *)elf + elf->e_phoff);
	eph = ph + elf->e_phnum;
	for (uint8_t *pa; ph < eph; ph++) {
		pa = (uint8_t *)(uintptr_t)ph->p_paddr;
		readseg(pa, ph->p_filesz, ph->p_offset);
		if (ph->p_memsz > ph->p_filesz)
			stosb(pa + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
	}

	// Call the entry point from the ELF header.
	// Does not return!
	entry = (void (*)(void))(elf->e_entry);
	entry();
}

void
waitdisk(void)
{
	// Wait for disk ready.
	while ((inb(0x1F7) & 0xC0) != 0x40)
		;
}

// Read a single sector at offset into dst.
void
readsect(void *dst, uint32_t offset)
{
	// Issue command.
	waitdisk();
	outb(0x1F2, 1); // count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20); // cmd 0x20 - read sectors

	// Read data.
	waitdisk();
	insl(0x1F0, dst, SECTSIZE / 4);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked.
void
readseg(uint8_t *pa, uint32_t count, uint32_t offset)
{
	// Round down to sector boundary.
	pa -= offset % SECTSIZE;

	// Translate from bytes to sectors; kernel starts at sector 1.
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	for (uint8_t *epa = pa + count; pa < epa; pa += SECTSIZE, offset++)
		readsect(pa, offset);
}
