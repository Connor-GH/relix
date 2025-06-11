#pragma once
#include "kernel/include/mman.h"
#include <stddef.h>
#include <sys/types.h>
#define MMAP_FAILED ((void *)-1)
void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset);
// Release memory mapping. You can do this even if the fd is closed!
int munmap(void *addr, size_t length);
