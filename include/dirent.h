#pragma once
#ifndef USE_HOST_TOOLS
#include <bits/__DIRSIZ.h>
#include <bits/__NAME_MAX.h>
#include <bits/size_t.h>
#include <bits/types.h>
#else
#include "include/bits/__DIRSIZ.h"
#include "include/bits/__NAME_MAX.h"
#include "include/bits/size_t.h"
#include "include/bits/types.h"
#endif

typedef __ino_t ino_t;
typedef __reclen_t reclen_t;
typedef __size_t size_t;
typedef __ssize_t ssize_t;

#define DT_UNKNOWN 0 // Unknown file type.
#define DT_BLK 1 // Block special.
#define DT_CHR 2 // Character special.
#define DT_DIR 3 // Directory.
#define DT_FIFO 4 // FIFO special.
#define DT_LNK 5 // Symbolic link.
#define DT_REG 6 // Regular.
#define DT_SOCK 7 // Socket.
#define DT_MQ 8 // Message queue.
#define DT_SEM 9 // Semaphore.
#define DT_SHM 10 // Shared memory object.
#define DT_TMO 11 // Typed memory object.

// flags for posix_getdents
#define DT_FORCE_TYPE 1

// POSIX definition of dirent
struct dirent {
	__ino_t d_ino;
	char d_name[__NAME_MAX + 1];
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

// POSIX.1-2024
struct posix_dent {
	__ino_t d_ino; // Inode number.
	__reclen_t d_reclen; // Length of this entry, including padding.

	unsigned char d_type; // File type or unknown-file-type indication.
	char d_name[]; // Filename string of entry.
};

DIR *opendir(const char *name);
DIR *fdopendir(int fd);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dirp);

ssize_t posix_getdents(int fd, void *buf, size_t nbyte, int flags);
