#ifndef _MMU_H
#define _MMU_H
#pragma once
#ifndef __ASSEMBLER__
#include <stdint.h>
#endif
// This file contains definitions for the
// x86 memory management unit (MMU).

// Rflags register
#define FL_CARRY (1U << 0)
#define FL_PARITY (1U << 2)
#define FL_ADJUST (1U << 4)
#define FL_ZERO (1U << 6)
#define FL_SIGN (1U << 7)
#define FL_TRAP (1U << 8)
#define FL_IF (1U << 9) // Interrupt Enable
#define FL_DIRECTION (1U << 10)
#define FL_OVERFLOW (1U << 11)
#define FL_IOPL (3U << 12)

// Control Register flags
#define CR0_PE 0x00000001 // Protection Enable
#define CR0_WP 0x00010000 // Write Protect
#define CR0_PG 0x80000000 // Paging
#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_TS (1 << 3)
#define CR0_NE (1 << 5)

#define CR4_OSFXSR (1 << 9)
#define CR4_OSXMMEXCPT (1 << 10)
#define CR4_OSXSAVE (1 << 18)

#define CR4_PSE 0x00000010 // Page size extension

#if __x86_64__ || __ASSEMBLER__
// mycpu()->gdt holds these segments.

// various segment selectors.
#define SEG_NULL 0U // NULL descriptor
#define SEG_KCODE 1U // kernel code
#define SEG_KDATA 2U // kernel data+stack
#define SEG_UDATA 3U // user data+stack
#define SEG_UCODE 4U // user code
#define SEG_TSS 5U // this process's task state; takes up 2 slots.

#define NSEGS 7U

#endif
#ifndef __ASSEMBLER__
// Segment Descriptor
struct segdesc {
	uint32_t lim_15_0 : 16; // Low bits of segment limit
	uint32_t base_15_0 : 16; // Low bits of segment base address
	uint32_t base_23_16 : 8; // Middle bits of segment base address
	uint32_t type : 4; // Segment type (see STS_ constants)
	uint32_t s : 1; // 0 = system, 1 = application
	uint32_t dpl : 2; // Descriptor Privilege Level
	uint32_t p : 1; // Present
	uint32_t lim_19_16 : 4; // High bits of segment limit
	uint32_t avl : 1; // Unused (available for software use)
	uint32_t rsv1 : 1; // Reserved
	uint32_t db : 1; // 0 = 16-bit segment, 1 = 32-bit segment
	uint32_t g : 1; // Granularity: limit scaled by 4K when set
	uint32_t base_31_24 : 8; // High bits of segment base address
};

// Normal segment
#define SEG(type, base, lim, dpl)                     \
	(struct segdesc){ ((lim) >> 12) & 0xffff,           \
		                (uint32_t)(base) & 0xffff,        \
		                ((uintptr_t)(base) >> 16) & 0xff, \
		                type,                             \
		                1,                                \
		                dpl,                              \
		                1,                                \
		                (uintptr_t)(lim) >> 28,           \
		                0,                                \
		                0,                                \
		                1,                                \
		                1,                                \
		                (uintptr_t)(base) >> 24 }
#define SEG16(type, base, lim, dpl)                   \
	(struct segdesc){ (lim) & 0xffff,                   \
		                (uint32_t)(base) & 0xffff,        \
		                ((uintptr_t)(base) >> 16) & 0xff, \
		                type,                             \
		                1,                                \
		                dpl,                              \
		                1,                                \
		                (uintptr_t)(lim) >> 16,           \
		                0,                                \
		                0,                                \
		                1,                                \
		                0,                                \
		                (uintptr_t)(base) >> 24 }
#endif

#define DPL_KERNEL 0x0
#define DPL_USER 0x3 // User DPL

#define SEG_A (1 << 0) // segment accessed bit
#define SEG_R (1 << 1) // readable (code)
#define SEG_W (1 << 1) // writable (data)
#define SEG_C (1 << 2) // conforming segment (code)
#define SEG_E (1 << 2) // expand-down bit (data)
#define SEG_CODE (1 << 3) // code segment (instead of data)

// User and system segment bits.
#define SEG_S (1 << 4) // if 0, system descriptor
#define SEG_DPL(x) ((x) << 5) // descriptor privilege level (2 bits)
#define SEG_P (1 << 7) // segment present
#define SEG_AVL (1 << 8) // available for operating system use
#define SEG_L (1 << 9) // long mode
#define SEG_D (1 << 10) // default operation size 32-bit
#define SEG_G (1 << 11) // granularity

// Application segment type bits
#define STA_X 0x8 // Executable segment
#define STA_W 0x2 // Writeable (non-executable segments)
#define STA_R 0x2 // Readable (executable segments)

// System segment type bits
#if __x86_64__
#define STS_RESERVED 0
#define STS_LDT 0x2
#define STS_T64A 0x9 // Available 64-bit TSS
#define STS_T64B 0xB // Busy 64-bit TSS
#define STS_CG64 0xC // 64-bit Call Gate
#define STS_IG64 0xE // 64-bit Interrupt Gate
#define STS_TG64 0xF // 64-bit Trap Gate
#endif

// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

#define PML4X(va) (((uintptr_t)(va) >> PML4XSHIFT) & PXMASK)

#define PDPTX(va) (((uintptr_t)(va) >> PDPTXSHIFT) & PXMASK)

// page directory index
#define PDX(va) (((uintptr_t)(va) >> PDXSHIFT) & PXMASK)

// page table index
#define PTX(va) (((uintptr_t)(va) >> PTXSHIFT) & PXMASK)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uintptr_t)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#if __x86_64__
#define NPDENTRIES 512
#define NPTENTRIES 512
#define PGSIZE 4096
#define PGSHIFT 12
#define PTXSHIFT 12
#define PDXSHIFT 21
#define PDPTXSHIFT 30
#define PML4XSHIFT 39
#define PXMASK 0x1FF
#else
// Page directory and page table constants.
#define NPDENTRIES 1024 // # directory entries per page directory
#define NPTENTRIES 1024 // # PTEs per page table
#define PGSIZE 4096 // bytes mapped by a page

#define PGSHIFT 12
#define PTXSHIFT 12 // offset of PTX in a linear address
#define PDXSHIFT 22 // offset of PDX in a linear address
#define PXMASK 0x3FF

#endif
#define PGROUNDUP(sz) \
	(((sz) + (uintptr_t)PGSIZE - 1) & ~((uintptr_t)PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~((uintptr_t)PGSIZE - 1))

// Page Directory Pointer Table flags.
#define PDPT_P (1 << 0) // Present
#define PDPT_W (1 << 1) // Writeable
#define PDPT_U (1 << 2) // User
#define PDPT_PWT (1 << 3) // Write-Through
#define PDPT_PCD (1 << 4) // Cache-Disable
#define PDPT_A (1 << 5) // Accessed
#define PDPT_D (1 << 6) // Dirty
#define PDPT_PS (1 << 7) // Page Size
#define PDPT_GLOBAL (1 << 8) // Global
#define PDPT_AVL (0b111 << 9) // Available/Unused
#define PDPT_PAT (1 << 12) // Page Attribute Table

// Page table flags.
#define PTE_P (1 << 0) // Present
#define PTE_W (1 << 1) // Writeable
#define PTE_U (1 << 2) // User
#define PTE_PWT (1 << 3) // Write-Through
#define PTE_PCD (1 << 4) // Cache-Disable
#define PTE_A (1 << 5) // Accessed
#define PTE_D (1 << 6) // Dirty
#define PTE_PAT (1 << 7) // Page Attribute Table
#define PTE_GLOBAL (1 << 8) // Global
#define PTE_AVL (0b111 << 9) // Available/Unused
// Custom PTE flags (contained within PTE_AVL).
#define PTE_COW (1 << 9)
#define PTE_UNUSED10 (1 << 10)
#define PTE_UNUSED11 (1 << 11)

// Page directory entry flags.
#define PDE_P (1 << 0) // Present
#define PDE_W (1 << 1) // Writeable
#define PDE_U (1 << 2) // User
#define PDE_PWT (1 << 3) // Write-Through
#define PDE_PCD (1 << 4) // Cache-Disable
#define PDE_A (1 << 5) // Accessed
#define PDE_D (1 << 6) // Dirty
#define PDE_PS (1 << 7) // Page Size
#define PDE_GLOBAL (1 << 8) // Global
#define PDE_AVL (0b111 << 9) // Available/Unused
#define PDE_PAT (1 << 12) // Page Attribute Table

// Address in page table or page directory entry
#define PTE_ADDR(pte) ((uintptr_t)(pte) & ~0xFFF)
#define PTE_FLAGS(pte) ((int)(pte) & 0xFFF)

#ifndef __ASSEMBLER__
typedef uintptr_t pte_t;
typedef uintptr_t pde_t;
typedef uintptr_t pdpt_t;

struct taskstate64 {
	uint32_t __reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint32_t __reserved1[2];
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint32_t __reserved2[2];
	uint16_t __reserved3;
	uint16_t iomb;
} __attribute__((packed));

_Static_assert(sizeof(struct taskstate64) == 104, "");

// Task state segment format (32-bit).
struct taskstate32 {
	uint32_t link; // Old ts selector
	uint32_t rsp0; // Stack pointers and segment selectors
	uint16_t ss0; //   after an increase in privilege level
	uint16_t padding1;
	uint32_t *rsp1;
	uint16_t ss1;
	uint16_t padding2;
	uint32_t *rsp2;
	uint16_t ss2;
	uint16_t padding3;
	void *cr3; // Page directory base
	uint32_t *rip; // Saved state from last task switch
	uint32_t rflags;
	uint32_t rax; // More saved state (registers)
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t *rsp;
	uint32_t *rbp;
	uint32_t esi;
	uint32_t edi;
	uint16_t es; // Even more saved state (segment selectors)
	uint16_t padding4;
	uint16_t cs;
	uint16_t padding5;
	uint16_t ss;
	uint16_t padding6;
	uint16_t ds;
	uint16_t padding7;
	uint16_t fs;
	uint16_t padding8;
	uint16_t gs;
	uint16_t padding9;
	uint16_t ldt;
	uint16_t padding10;
	uint16_t t; // Trap on task switch
	uint16_t iomb; // I/O map base address
};

// Gate descriptors for interrupts and traps
struct gatedesc {
	uint32_t off_15_0 : 16; // low 16 bits of offset in segment
	uint32_t cs : 16; // code segment selector
	uint32_t args : 5; // # args, 0 for interrupt/trap gates
	uint32_t rsv1 : 3; // reserved(should be zero I guess)
	uint32_t type : 4; // type(STS_{IG32,TG32})
	uint32_t s : 1; // must be 0 (system)
	uint32_t dpl : 2; // descriptor(meaning new) privilege level
	uint32_t p : 1; // Present
	uint32_t off_31_16 : 16; // high bits of offset in segment
};

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)        \
	{                                               \
		(gate).off_15_0 = (uint32_t)(off) & 0xffff;   \
		(gate).cs = (sel);                            \
		(gate).args = 0;                              \
		(gate).rsv1 = 0;                              \
		(gate).type = (istrap) ? STS_TG32 : STS_IG32; \
		(gate).s = 0;                                 \
		(gate).dpl = (d);                             \
		(gate).p = 1;                                 \
		(gate).off_31_16 = (uint32_t)(off) >> 16;     \
	}

#endif
#endif // !_MMU_H
