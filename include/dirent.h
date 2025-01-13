#pragma once
#ifndef USE_HOST_TOOLS
#include "stdint.h"
#endif
#include "stdio.h"
struct dirent {
	uint16_t inum;
	char name[__DIRSIZ];
};
struct __DIR {
	int fd;
	struct dirent cur_ent;
	char *buffer;
	int buffer_size;
	char *nextptr;
};
typedef struct __DIR DIR;

DIR *
opendir(const char *name);
