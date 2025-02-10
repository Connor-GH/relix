#pragma once
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
int
allocuvm(uintptr_t *, uintptr_t, uintptr_t);
int
deallocuvm(uintptr_t *, uintptr_t, uintptr_t);
void
freevm(uintptr_t *);
void
inituvm(uintptr_t *, char *, uint32_t);
int
loaduvm(uintptr_t *, char *, struct inode *, uint32_t, uint32_t);
uintptr_t *
copyuvm(uintptr_t *, uint32_t);
void
switchuvm(struct proc *);
void
switchkvm(void);
int
copyout(uintptr_t *pgdir, uintptr_t va, void *pa, size_t len);
void
clearpteu(uintptr_t *pgdir, char *uva);
int
mappages(uintptr_t *pgdir, void *va, uintptr_t size, uintptr_t pa, int perm);
void
unmap_user_page(uintptr_t *pgdir, char *user_va);
