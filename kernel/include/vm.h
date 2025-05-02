#pragma once
#if __KERNEL__
#include <stdint.h>
#include "proc.h"
void
seginit(void);
void
kvmalloc(void);
uintptr_t *
setupkvm(void);
char *
uva2ka(uintptr_t *, char *);
uintptr_t
allocuvm(uintptr_t *, uintptr_t, uintptr_t);
uintptr_t
deallocuvm(uintptr_t *, uintptr_t, uintptr_t);
void
freevm(uintptr_t *);
void
inituvm(uintptr_t *, char *, uint32_t);
int
loaduvm(uintptr_t *, char *, struct inode *, uint32_t, uint32_t);
uintptr_t *
copyuvm(uintptr_t *, size_t);
void
switchuvm(struct proc *);
void
switchkvm(void);
int
copyout(uintptr_t *pgdir, uintptr_t va, void *pa, size_t len);
void
setpteu(uintptr_t *pgdir, char *uva);
void
clearpteu(uintptr_t *pgdir, char *uva);
int
mappages(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa, int perm);
void
unmap_user_page(uintptr_t *pgdir, char *user_va);
#endif
