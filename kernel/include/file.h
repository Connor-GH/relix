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
	off_t off;
};

// table mapping major device number to
// device functions
struct devsw {
	int (*open)(int);
	int (*close)(void);
	int (*read)(struct inode *, char *, int);
	int (*write)(struct inode *, char *, int);
	struct mmap_info (*mmap)(size_t length, uintptr_t addr);
};

extern struct devsw devsw[];

enum { CONSOLE = 1, NULLDRV = 2, FB = 3, KBD = 4, };

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
ssize_t
fileread(struct file *, char *, uint64_t n);
int
filestat(struct file *, struct stat *);
ssize_t
filewrite(struct file *, char *, uint64_t n);
off_t
fileseek(struct file *f, off_t offset, int whence);
struct file *
fd_to_struct_file(int fd);
char *
inode_to_path(char *buf, size_t n, struct inode *ip);
