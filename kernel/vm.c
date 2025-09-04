/* vm64.c
 *
 * Copyright (c) 2013 Brian Swetland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "vm.h"
#include "boot/multiboot2.h"
#include "console.h"
#include "errno.h"
#include "fs.h"
#include "kalloc.h"
#include "kernel_ld_syms.h"
#include "memlayout.h"
#include "mmu.h"
#include "msr.h"
#include "param.h"
#include "proc.h"
#include "vga.h"
#include "x86.h"
#include <stdint.h>
#include <string.h>

uintptr_t *kpgdir; // for use in scheduler()

#define PHYSTOP 1
#define DEVSPACETOP 1
// every process's page table.
static struct kmap {
	void *virt;
	uint64_t phys_start;
	uint64_t phys_end;
	int perm;
} kmap[] = {
	{ (void *)KERNBASE, 0, EXTMEM, PTE_W }, // I/O space
	{ (void *)KERNLINK, V2P(KERNLINK), V2P(__kernel_data), 0 }, // kern
	                                                            // text+rodata
	{ (void *)__kernel_data, V2P(__kernel_data), PHYSTOP, PTE_W }, // kern
	                                                               // data+memory
	{ (void *)P2V(DEVSPACE), DEVSPACE, DEVSPACETOP, PTE_W }, // more devices
};

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(uintptr_t *pgdir, const void *va, bool alloc)
{
	uintptr_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P) {
		pgtab = (pte_t *)p2v(PTE_ADDR(*pde));
	} else {
		// Not present? We need to allocate. But if we aren't allocating,
		// this makes no sense to do.
		if (!alloc || (pgtab = (pte_t *)kpage_alloc()) == NULL) {
			return NULL;
		}
		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PGSIZE);
		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table
		// entries, if necessary.
		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
int
mappages_perm(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa,
              int perm)
{
	char *a, *last;
	pte_t *pte;

	a = (char *)PGROUNDDOWN((uintptr_t)va);
	last = (char *)PGROUNDDOWN(((uintptr_t)va) + size - 1);
	for (;;) {
		if ((pte = walkpgdir(pgdir, a, true)) == NULL) {
			return -1;
		}
		if (*pte & PTE_P) {
			uart_printf("Remap of %p from %#lx to %#lx\n", a, PTE_ADDR(*pte), pa);
			panic("remap");
		}
		*pte = pa | perm;
		if (a == last) {
			break;
		}
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

int
mappages(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa, int perm)
{
	return mappages_perm(pgdir, va, size, pa, perm | PTE_P);
}

int
mappages_cow(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa, int perm)
{
	return mappages_perm(pgdir, va, size, pa, perm | PTE_COW);
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(uintptr_t *pgdir, char *init, uint32_t sz)
{
	char *mem;

	if (sz >= PGSIZE) {
		panic("inituvm: more than a page");
	}
	mem = kpage_alloc();
	memset(mem, 0, PGSIZE);
	mappages(pgdir, NULL, PGSIZE, V2P(mem), PTE_W | PTE_U);
	memmove(mem, init, sz);
}

// Load a program segment into pgdir. addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(uintptr_t *pgdir, char *addr, struct inode *ip, off_t offset,
        uintptr_t sz)
{
	uintptr_t pa, n;
	pte_t *pte;

	if ((uintptr_t)addr % PGSIZE != 0) {
		panic("loaduvm: addr must be page aligned");
	}
	for (uintptr_t i = 0; i < sz; i += PGSIZE) {
		if ((pte = walkpgdir(pgdir, addr + i, false)) == NULL) {
			panic("loaduvm: address should exist");
		}
		pa = PTE_ADDR(*pte);
		if (sz - i < PGSIZE) {
			n = sz - i;
		} else {
			n = PGSIZE;
		}
		if (inode_read(ip, p2v(pa), offset + (off_t)i, n) != n) {
			return -1;
		}
	}
	return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uintptr_t
allocuvm(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz)
{
	char *mem;
	uintptr_t a;

	if (newsz >= KERNBASE) {
		return 0;
	}
	if (newsz < oldsz) {
		return oldsz;
	}

	a = PGROUNDUP(oldsz);
	for (; a < newsz; a += PGSIZE) {
		mem = kpage_alloc();
		if (mem == NULL) {
			cprintf("allocuvm out of memory\n");
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}
		memset(mem, 0, PGSIZE);
		if (mappages(pgdir, (char *)a, PGSIZE, V2P(mem), PTE_W | PTE_U) < 0) {
			deallocuvm(pgdir, newsz, oldsz);
			kpage_free(mem);
			return 0;
		}
	}
	return newsz;
}

int
alloc_user_bytes(uintptr_t *pgdir, const size_t size, const uintptr_t virt_addr,
                 uintptr_t *phys_addr)
{
	if (size == 0) {
		return -EINVAL;
	}
	if (pgdir == NULL) {
		return -EINVAL;
	}

	const size_t newsize = PGROUNDUP(size);
	for (size_t i = 0; i < newsize; i += PGSIZE) {
		char *mem = kmalloc(PGSIZE);
		if (mem == NULL) {
			uart_printf("alloc_user_bytes: out of memory\n");
			/* dealloc_user_bytes ... */
			return -ENOMEM;
		}
		if (i == 0 && phys_addr != NULL) {
			*phys_addr = V2P(mem);
		}
		const uintptr_t proper_virt_addr = i + PGROUNDUP(virt_addr);

		if (mappages(pgdir, (char *)proper_virt_addr, PGSIZE, V2P(mem),
		             PTE_W | PTE_U) < 0) {
			/* dealloc_user_bytes ... */
			kfree(mem);
			return -ENOMEM;
		}
	}
	return 0;
}

uintptr_t
allocuvm_cow(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz)
{
	char *mem;
	uintptr_t a;

	if (newsz >= KERNBASE) {
		return 0;
	}
	if (newsz < oldsz) {
		return oldsz;
	}

	a = PGROUNDUP(oldsz);
	for (; a < newsz; a += PGSIZE) {
		if (mappages_cow(pgdir, (char *)a, PGSIZE, 0x0, PTE_W | PTE_U) < 0) {
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}
	}
	return newsz;
}
// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uintptr_t
deallocuvm(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz)
{
	pte_t *pte;
	uintptr_t a, pa;

	if (newsz >= oldsz) {
		return oldsz;
	}

	a = PGROUNDUP(newsz);
	for (; a < oldsz; a += PGSIZE) {
		pte = walkpgdir(pgdir, (char *)a, 0);
		if (!pte) {
			// a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
			a += (uintptr_t)(NPTENTRIES - 1) * PGSIZE;
		} else if ((*pte & PTE_P) != 0) {
			pa = PTE_ADDR(*pte);
			if (pa == 0) {
				panic("kpage_free");
			}
			// Temporary hack.
			// The only set of memory above the physical address of KERNBASE
			// is a device's mmio. Since those aren't allocated, just ignore
			// them.
			if ((uintptr_t)P2V(pa) > KERNBASE) {
				char *v = p2v(pa);
				kpage_free(v);
			}
			*pte = 0;
		}
	}
	return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(uintptr_t *pgdir)
{
	if (pgdir == NULL) {
		panic("freevm: no pgdir");
	}
	deallocuvm(pgdir, /*KERNBASE*/ 0x3fa00000, 0);
	// "- 2" because of the page back pointers.
	for (uint32_t i = 0; i < NPDENTRIES - 2; i++) {
		if (pgdir[i] & PTE_P) {
			char *v = P2V(PTE_ADDR(pgdir[i]));
			kpage_free(v);
		}
	}
	kpage_free((char *)pgdir);
}

void
clear_pte_mask(uintptr_t *pgdir, char *user_virt_addr, pte_t mask)
{
	pte_t *pte = walkpgdir(pgdir, user_virt_addr, false);

	if (pte == NULL) {
		panic("set_pte_mask");
	}
	*pte &= ~mask;
}

void
set_pte_mask(uintptr_t *pgdir, char *user_virt_addr, pte_t mask)
{
	pte_t *pte = walkpgdir(pgdir, user_virt_addr, false);

	if (pte == NULL) {
		panic("set_pte_mask");
	}
	*pte |= mask;
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(uintptr_t *pgdir, char *uva)
{
	clear_pte_mask(pgdir, uva, PTE_U);
}

// Set PTE_U on a page. Used to create new pages or
// remap kernel pages to user pages.
void
setpteu(uintptr_t *pgdir, char *uva)
{
	set_pte_mask(pgdir, uva, PTE_U);
}

void
unmap_user_page(uintptr_t *pgdir, char *user_va)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, user_va, false);
	if (pte == NULL) {
		panic("unmap_user_page: no page found");
	}
	// Clear all bits in the PTE.
	*pte = 0;
}

// Given a parent process's page table, create a copy
// of it for a child.
uintptr_t *
copyuvm(uintptr_t *pgdir, size_t sz)
{
	uintptr_t *d;
	pte_t *pte;
	uintptr_t pa;
	int flags;
	char *mem;

	if ((d = setupkvm()) == NULL) {
		return NULL;
	}
	for (uintptr_t i = 0; i < sz; i += PGSIZE) {
		if ((pte = walkpgdir(pgdir, (void *)i, false)) == NULL) {
			panic("copyuvm: pte should exist");
		}
		if (!(*pte & PTE_P)) {
			panic("copyuvm: page not present");
		}
		pa = PTE_ADDR(*pte);
		flags = PTE_FLAGS(*pte);
		if ((mem = kpage_alloc()) == NULL) {
			goto bad;
		}
		memmove(mem, (char *)p2v(pa), PGSIZE);
		if (mappages(d, (void *)i, PGSIZE, V2P(mem), flags) < 0) {
			kpage_free(mem);
			goto bad;
		}
	}
	return d;

bad:
	freevm(d);
	return NULL;
}

// Map user virtual address to kernel address.
char *
uva2ka(uintptr_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if (pte == NULL) {
		return NULL;
	}
	if ((*pte & PTE_P) == 0) {
		return NULL;
	}
	if ((*pte & PTE_U) == 0) {
		return NULL;
	}
	return (char *)p2v(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(uintptr_t *pgdir, uintptr_t va, void *p, size_t len)
{
	char *buf, *pa0;
	uintptr_t n, va0;

	buf = (char *)p;
	while (len > 0) {
		va0 = (uint32_t)PGROUNDDOWN(va);
		pa0 = uva2ka(pgdir, (char *)va0);
		if (pa0 == NULL) {
			return -EFAULT;
		}
		n = PGSIZE - (va - va0);
		if (n > len) {
			n = len;
		}
		memmove(pa0 + (va - va0), buf, n);
		len -= n;
		buf += n;
		va = va0 + PGSIZE;
	}
	return 0;
}

#define kalloc() kpage_alloc()
static uintptr_t *kpml4;
static uintptr_t *kpdpt;
static uintptr_t *iopgdir;
static uintptr_t *kpgdir0;
static uintptr_t *kpgdir1;

static void
mkgate(uint32_t *idt, uint32_t n, void *kva, uint32_t pl, uint32_t trap)
{
	uint64_t addr = (uint64_t)kva;
	n *= 4;
	trap = trap ? 0x8F00 : 0x8E00; // TRAP vs INTERRUPT gate;
	idt[n + 0] = (addr & 0xFFFF) | ((SEG_KCODE << 3) << 16);
	idt[n + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
	idt[n + 2] = addr >> 32;
	idt[n + 3] = 0;
}

// Set the RSP based on the TSS. "n" is either
// 0, 1, or 2.
static void
tss_set_rsp(struct taskstate64 *tss, uint32_t n, uint64_t rsp)
{
	switch (n) {
	case 0:
		tss->rsp0 = rsp;
		break;
	case 1:
		tss->rsp1 = rsp;
		break;
	case 2:
		tss->rsp2 = rsp;
		break;
	default:
		panic("tss_set_rsp: invalid `n' value");
	}
}

extern void *vectors[];

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
	void *local;
	uint32_t *idt = (uint32_t *)kalloc();
	memset(idt, 0, PGSIZE);

	for (int n = 0; n < 256; n++) {
		mkgate(idt, n, vectors[n], 0, 0);
	}

	lidt((void *)idt, PGSIZE);

	// Create a page for cpu local storage.
	// This is just 4kiB of free space for %FS.
	local = kpage_alloc();
	memset(local, 0, PGSIZE);

	struct taskstate64 *tss = kmalloc(sizeof(*tss));
	memset(tss, 0, sizeof(*tss));

	// IO Map Base = End of TSS
	tss->iomb = sizeof(*tss);

	// Point FS to our local storage page.
	wrmsr(MSR_FS_BASE, ((uint64_t)local));

	struct cpu *c = early_init_mycpu();
	c->local = local;

	c->self = c;
	c->kernel_stack = 0;
	c->user_stack = 0;
	c->proc = NULL;

	uint64_t addr = (uint64_t)tss;
	c->gdt_bits[SEG_NULL] = 0x0000000000000000LU;
	c->gdt_bits[SEG_KCODE] = 0x0020980000000000LU; // Code, DPL=0, R/X
	c->gdt_bits[SEG_KDATA] = 0x0000920000000000LU; // Data, DPL=0, W
	c->gdt_bits[SEG_UDATA] = 0x0000F20000000000LU; // Data, DPL=3, W
	c->gdt_bits[SEG_UCODE] = 0x0020F80000000000LU; // Code, DPL=3, R/X
	c->gdt_bits[SEG_TSS + 0] =
		(0x0067) /* segment limit */ |
		((addr & 0xFFFFFF) << 16) /* base address 15:00 */ |
		(((addr >> 16LU) & 0xFFLU) << 32LU) /* base address 23:16 */ |
		/* (STS_T64A | (DPL_USER << 5) | (1 << 7)) , avl=0, granularity=0 */
		(0x00E9LL << 40) | (((addr >> 24) & 0xFF) << 56) /* address 31:24 */;
	c->gdt_bits[SEG_TSS + 1] = (addr >> 32) & 0xFFFFFFFF /* address 63:32 */;
	c->tss = tss;

	lgdt((void *)c->gdt, sizeof(c->gdt));

	ltr(SEG_TSS << 3);
	syscall_init(c);
}

// The core relix code only knows about two levels of page tables,
// so we will create all four, but only return the second level.
// because we need to find the other levels later, we'll stash
// backpointers to them in the top two entries of the level two
// table.
uintptr_t *
setupkvm(void)
{
	uintptr_t *pml4 = (uintptr_t *)kalloc();
	uintptr_t *pdpt = (uintptr_t *)kalloc();
	uintptr_t *pgdir = (uintptr_t *)kalloc();

	memset(pml4, 0, PGSIZE);
	memset(pdpt, 0, PGSIZE);
	memset(pgdir, 0, PGSIZE);
	/*
	 * This code syncs with the setup code in entry64.S
	 */
	// "PML4 -> PDPT-A"
	pml4[0] = v2p(pdpt) | PTE_P | PTE_W | PTE_U;
	// "P4ML -> PDPT-B"
	pml4[511] = v2p(kpdpt) | PTE_P | PTE_W | PTE_U;
	// "PDPT-A -> PD"
	pdpt[0] = v2p(pgdir) | PTE_P | PTE_W | PTE_U;

	// virtual backpointers
	pgdir[511] = ((uintptr_t)pml4) | PTE_P;
	pgdir[510] = ((uintptr_t)pdpt) | PTE_P;

	return pgdir;
}

void
switchkvm(void)
{
	lcr3(v2p(kpml4));
}
// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
//
// linear map the first 4GB of physical memory starting at 0xFFFFFFFF80000000
void
kvmalloc(void)
{
	int n;
	kpml4 = (uintptr_t *)kalloc();
	kpdpt = (uintptr_t *)kalloc();
	kpgdir0 = (uintptr_t *)kalloc();
	kpgdir1 = (uintptr_t *)kalloc();
	iopgdir = (uintptr_t *)kalloc();
	memset(kpml4, 0, PGSIZE);
	memset(kpdpt, 0, PGSIZE);
	memset(iopgdir, 0, PGSIZE);
	kpml4[511] = v2p(kpdpt) | PTE_P | PTE_W;
	kpdpt[511] = v2p(kpgdir1) | PTE_P | PTE_W;
	kpdpt[510] = v2p(kpgdir0) | PTE_P | PTE_W;
	kpdpt[509] = v2p(iopgdir) | PTE_P | PTE_W;
	// The boot page table used in entry64.S and entryother.S.
	// PTE_PS in a page directory entry enables 4Mbyte pages.
	// Notice, though, how cr4 does not have bit 7 set in entry64.S
	// that means that this effectively does nothing until we turn it on.
	for (n = 0; n < NPDENTRIES; n++) {
		kpgdir0[n] = (n << PDXSHIFT) | PDE_PS | PDE_P | PDE_W;
		kpgdir1[n] = ((n + 512) << PDXSHIFT) | PDE_PS | PDE_P | PDE_W;
	}
	struct multiboot_tag_framebuffer_common fb_common =
		get_multiboot_framebuffer()->common;
	uintptr_t fb_addr = fb_common.framebuffer_addr;
	for (n = 0; n < 16; n++) {
		iopgdir[n] = (fb_addr + (n << PDXSHIFT)) | PDE_PS | PDE_P | PDE_W |
		             /* Comment out PCD for now so that we can get write-combining.
		              * when VM gets redone, we should change this. */
		             PDE_PWT /*| PDE_PCD*/;
	}
	switchkvm();
}

void
switchuvm(struct proc *p)
{
	void *pml4;
	struct taskstate64 *tss;

	pushcli();
	if (p->pgdir == NULL) {
		panic("switchuvm: no pgdir");
	}
	tss = mycpu()->tss;
	tss_set_rsp(tss, 0, (uintptr_t)myproc()->kstack + KSTACKSIZE);
	// Set for when we swapgs in syscalls.
	mycpu()->kernel_stack = tss->rsp0;
	pml4 = (void *)PTE_ADDR(p->pgdir[511]);
	lcr3(v2p(pml4));
	popcli();
}
