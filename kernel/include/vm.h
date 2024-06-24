#pragma once
#include <types.h>
#include <proc.h>
#include <file.h>
void
seginit(void);
void
kvmalloc(void);
pde_t *
setupkvm(void);
char *
uva2ka(pde_t *, char *);
int
allocuvm(pde_t *, uint, uint);
int
deallocuvm(pde_t *, uint, uint);
void
freevm(pde_t *);
void
inituvm(pde_t *, char *, uint);
int
loaduvm(pde_t *, char *, struct inode *, uint, uint);
pde_t *
copyuvm(pde_t *, uint);
void
switchuvm(struct proc *);
void
switchkvm(void);
int
copyout(pde_t *, uint, void *, uint);
void
clearpteu(pde_t *pgdir, char *uva);
