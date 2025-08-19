#ifndef _FILE_H
#define _FILE_H
#pragma once
#include "fs.h"
#include "mman.h"
#include "param.h"
#include <stdint.h>
#include <sys/stat.h>
#if __KERNEL__
struct file {
	enum {
		FD_NONE,
		FD_PIPE,
		FD_INODE,
		FD_FIFO,
	} type;
	int flags; // Flags like O_CLOEXEC.
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
	int (*open)(short minor, int);
	int (*close)(short minor);
	ssize_t (*read)(short minor, struct inode *, char *, size_t);
	ssize_t (*write)(short minor, struct inode *, char *, size_t);
	struct mmap_info (*mmap)(short minor, size_t length, uintptr_t addr,
	                         int perm);
};
#endif

#define MINOR_TTY_SERIAL 64

enum {
	// The system console for kernel logging. /dev/console
	CONSOLE = 1,
	// /dev/null
	NULLDRV = 2,
	// The screen framebuffer(s). /dev/fb[0-9]*
	FB = 3,
	// The keyboard scancodes. /dev/kbd[0-9]*
	KBD = 4,
	// The disk device and partitions. /dev/sd[a-z]+(p[0-9]+)?
	SD = 5,
	// /dev/tty[0-9]+ (0-63) and /dev/ttyS[0-9]+ (64-127)
	TTY = 6,
	// The mouse's packets. /dev/mouse[0-9]*
	MOUSE = 7,
	__DEVSW_last,
};

_Static_assert(__DEVSW_last <= NDEV,
               "Too many devices; adjust NDEV in param.h");
#if __KERNEL__
extern struct devsw devsw[];

// Helper functions for file manipulation that
// sysfile.c might like.
struct file *filealloc(void);
int fdalloc(struct file *f);
int fdalloc2(struct file *f, int fd);
struct file *fd_to_struct_file(int fd);
char *inode_to_path(char *buf, size_t n, struct inode *ip);
struct inode *resolve_name(const char *path);
struct inode *resolve_nameat(int dirfd, const char *path);

// Generalized functions for file operations.
int fileclose(struct file *);
struct inode *filecreate(int dirfd, char *path, mode_t mode, dev_t dev);
int fileopenat(int dirfd, char *path, int flags, mode_t mode);
struct file *filedup(struct file *f, int flags);
void fileinit(void);
ssize_t fileread(struct file *file, char *buf, size_t n);
int filestat(struct file *file, struct stat *);
ssize_t filewrite(struct file *file, char *buf, size_t n);
off_t fileseek(struct file *f, off_t offset, int whence);
ssize_t filereadlinkat(int dirfd, const char *restrict pathname, char *buf,
                       size_t bufsiz);

#endif
#endif // !_FILE_H
