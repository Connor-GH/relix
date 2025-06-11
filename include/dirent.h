#pragma once
#ifndef USE_HOST_TOOLS
#include "stdint.h"
#endif
#include "stdio.h"
// POSIX definition of dirent
struct dirent {
	uint16_t d_ino;
	char d_name[__DIRSIZ];
};
struct __linked_list_dirent {
	struct dirent data;
	struct __linked_list_dirent *next;
	struct __linked_list_dirent *prev;
};
struct __DIR {
	int fd;
	struct __linked_list_dirent *list;
};
typedef struct __DIR DIR;

DIR *opendir(const char *name);
DIR *fdopendir(int fd);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dirp);
