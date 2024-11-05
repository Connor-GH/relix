#pragma once
#include "fs.h"
#include <stdint.h>
#include <proc.h>
void
seginit(void);
void
kvmalloc(void);
uint32_t *
setupkvm(void);
char *
uva2ka(uint32_t *, char *);
int
allocuvm(uint32_t *, uint32_t, uint32_t);
int
deallocuvm(uint32_t *, uint32_t, uint32_t);
void
freevm(uint32_t *);
void
inituvm(uint32_t *, char *, uint32_t);
int
loaduvm(uint32_t *, char *, struct inode *, uint32_t, uint32_t);
uint32_t *
copyuvm(uint32_t *, uint32_t);
void
switchuvm(struct proc *);
void
switchkvm(void);
int
copyout(uint32_t *, uint32_t, void *, uint32_t);
void
clearpteu(uint32_t *pgdir, char *uva);
int
mappages(uint32_t *pgdir, void *va, uint32_t size, uint32_t pa, int perm);
