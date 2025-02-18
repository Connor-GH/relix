#pragma once
#include <stdint.h>
#include <stat.h>
#include "fs.h"
#include "mman.h"
struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE } type;
	int ref; // reference count
	char readable;
	char writable;
	struct pipe *pipe;
	struct inode *ip;
	uint32_t off;
};

// table mapping major device number to
// device functions
struct devsw {
	int (*read)(struct inode *, char *, int);
	int (*write)(struct inode *, char *, int);
	struct mmap_info (*mmap)(size_t length, uintptr_t addr);
};

extern struct devsw devsw[];

enum { CONSOLE = 1, NULLDRV = 2, FB = 3, };

struct file *
filealloc(void);
void
fileclose(struct file *);
int
fileopen(char *path, int flags);
struct file *
filedup(struct file *);
void
fileinit(void);
int
fileread(struct file *, char *, int n);
int
filestat(struct file *, struct stat *);
int
filewrite(struct file *, char *, int n);
int
fileseek(struct file *f, int n, int whence);
struct file *
fd_to_struct_file(int fd);
char *
inode_to_path(char *buf, size_t n, struct inode *ip);
