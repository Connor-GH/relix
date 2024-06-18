#pragma once
#include "types.h"
#include "kernel/include/fs.h"
struct dirent {
	ushort inum;
	char name[DIRSIZ];
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
