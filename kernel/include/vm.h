#pragma once
#if __RELIX_KERNEL__
#include "proc.h"
#include <stdint.h>
void seginit(void);
void kvmalloc(void);
uintptr_t *setupkvm(void);
char *uva2ka(uintptr_t *, char *);
uintptr_t allocuvm(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz);
uintptr_t allocuvm_cow(uintptr_t *pgdir, uintptr_t oldsz, uintptr_t newsz);
uintptr_t deallocuvm(uintptr_t *, uintptr_t, uintptr_t);
void freevm(uintptr_t *);
void inituvm(uintptr_t *, char *, uint32_t);
/// INVARIANT: offset + sz < 2^63
int loaduvm(uintptr_t *pgdir, char *addr, struct inode *ip, off_t offset,
            uintptr_t sz);
uintptr_t *copyuvm(uintptr_t *, size_t);
void switchuvm(struct proc *);
void switchkvm(void);
int copyout(uintptr_t *pgdir, uintptr_t va, void *pa, size_t len);
void setpteu(uintptr_t *pgdir, char *uva);
void clearpteu(uintptr_t *pgdir, char *uva);
int mappages(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa,
             int perm);
void unmap_user_page(uintptr_t *pgdir, char *user_va);
int alloc_user_bytes(uintptr_t *pgdir, const size_t size,
                     const uintptr_t virt_addr, uintptr_t *phys_addr);
#endif
