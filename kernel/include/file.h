#pragma once
#include <types.h>
#include <stat.h>
#include "fs.h"
struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE } type;
	int ref; // reference count
	char readable;
	char writable;
	struct pipe *pipe;
	struct inode *ip;
	uint off;
};


// table mapping major device number to
// device functions
struct devsw {
	int (*read)(struct inode *, char *, int);
	int (*write)(struct inode *, char *, int);
};

extern struct devsw devsw[];

enum { CONSOLE = 1, NULLDRV };

struct file *
filealloc(void);
void
fileclose(struct file *);
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
