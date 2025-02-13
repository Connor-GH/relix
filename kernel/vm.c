#include <stdint.h>
#include <defs.h>
#include "drivers/memlayout.h"
#include "drivers/mmu.h"
#include "x86.h"
#include "param.h"
#include "proc.h"
#include "kalloc.h"
#include "console.h"
#include "vm.h"
#include "fs.h"
#include "spinlock.h"
#include <string.h>

extern char data[]; // defined by kernel.ld
uintptr_t *kpgdir; // for use in scheduler()

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(uintptr_t *pgdir, const void *va, int alloc)
{
	uintptr_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P) {
		pgtab = (pte_t *)p2v(PTE_ADDR(*pde));
	} else {

		// Not present? We need to allocate. But if we aren't allocating,
		// this makes no sense to do.
		if (!alloc || (pgtab = (pte_t *)kpage_alloc()) == 0)
			return 0;
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
mappages(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa, int perm)
{
	char *a, *last;
	pte_t *pte;

	a = (char *)PGROUNDDOWN((uintptr_t)va);
	last = (char *)PGROUNDDOWN(((uintptr_t)va) + size - 1);
	for (;;) {
		if ((pte = walkpgdir(pgdir, a, 1)) == 0)
			return -1;
		if (*pte & PTE_P)
			panic("remap");
		*pte = pa | perm | PTE_P;
		if (a == last)
			break;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(uintptr_t *pgdir, char *init, uint32_t sz)
{
	char *mem;

	if (sz >= PGSIZE)
		panic("inituvm: more than a page");
	mem = kpage_alloc();
	memset(mem, 0, PGSIZE);
	mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W | PTE_U);
	memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(uintptr_t *pgdir, char *addr, struct inode *ip, uint32_t offset,
				uint32_t sz)
{
	uintptr_t i, pa, n;
	pte_t *pte;

	if ((uintptr_t)addr % PGSIZE != 0)
		panic("loaduvm: addr must be page aligned");
	for (i = 0; i < sz; i += PGSIZE) {
		if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
			panic("loaduvm: address should exist");
		pa = PTE_ADDR(*pte);
		if (sz - i < PGSIZE)
			n = sz - i;
		else
			n = PGSIZE;
		if (inode_read(ip, p2v(pa), offset + i, n) != n)
			return -1;
	}
	return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz)
{
	char *mem;
	uintptr_t a;

	if (newsz >= KERNBASE)
		return 0;
	if (newsz < oldsz)
		return oldsz;

	a = PGROUNDUP(oldsz);
	for (; a < newsz; a += PGSIZE) {
		mem = kpage_alloc();
		if (mem == 0) {
			cprintf("allocuvm out of memory\n");
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}
		memset(mem, 0, PGSIZE);
		if (mappages(pgdir, (char *)a, PGSIZE, V2P(mem), PTE_W | PTE_U) < 0) {
			cprintf("allocuvm out of memory (2)\n");
			deallocuvm(pgdir, newsz, oldsz);
			kpage_free(mem);
			return 0;
		}
	}
	return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz)
{
	pte_t *pte;
	uintptr_t a, pa;

	if (newsz >= oldsz)
		return oldsz;

	a = PGROUNDUP(newsz);
	for (; a < oldsz; a += PGSIZE) {
		pte = walkpgdir(pgdir, (char *)a, 0);
		if (!pte) {
			//a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
			a += (NPTENTRIES - 1) * PGSIZE;
		} else if ((*pte & PTE_P) != 0) {
			pa = PTE_ADDR(*pte);
			if (pa == 0)
				panic("kpage_free");
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
	uint32_t i;

	if (pgdir == 0)
		panic("freevm: no pgdir");
	deallocuvm(pgdir, /*KERNBASE*/ 0x3fa00000, 0);
	for (i = 0; i < NPDENTRIES - 2; i++) {
		if (pgdir[i] & PTE_P) {
			char *v = P2V(PTE_ADDR(pgdir[i]));
			kpage_free(v);
		}
	}
	kpage_free((char *)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(uintptr_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if (pte == 0)
		panic("clearpteu");
	*pte &= ~PTE_U;
}

void
unmap_user_page(uintptr_t *pgdir, char *user_va)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, user_va, 0);
	if (pte == 0) {
		panic("unmap_user_page: no page found");
	}
	// Clear all bits in the PTE.
	*pte = 0;
}

// Given a parent process's page table, create a copy
// of it for a child.
uintptr_t *
copyuvm(uintptr_t *pgdir, uint32_t sz)
{
	uintptr_t *d;
	pte_t *pte;
	uintptr_t pa, i, flags;
	char *mem;

	if ((d = setupkvm()) == 0)
		return 0;
	for (i = 0; i < sz; i += PGSIZE) {
		if ((pte = walkpgdir(pgdir, (void *)i, 0)) == 0)
			panic("copyuvm: pte should exist");
		if (!(*pte & PTE_P))
			panic("copyuvm: page not present");
		pa = PTE_ADDR(*pte);
		flags = PTE_FLAGS(*pte);
		if ((mem = kpage_alloc()) == 0)
			goto bad;
		memmove(mem, (char *)p2v(pa), PGSIZE);
		if (mappages(d, (void *)i, PGSIZE, V2P(mem), flags) < 0) {
			kpage_free(mem);
			goto bad;
		}
	}
	return d;

bad:
	freevm(d);
	return 0;
}

// Map user virtual address to kernel address.
char *
uva2ka(uintptr_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if (pte == NULL)
		return NULL;
	if ((*pte & PTE_P) == 0)
		return 0;
	if ((*pte & PTE_U) == 0)
		return 0;
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
		if (pa0 == 0)
			return -1;
		n = PGSIZE - (va - va0);
		if (n > len)
			n = len;
		memmove(pa0 + (va - va0), buf, n);
		len -= n;
		buf += n;
		va = va0 + PGSIZE;
	}
	return 0;
}
