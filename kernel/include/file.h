#pragma once
#include <types.h>
#include "sleeplock.h"
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

// in-memory copy of an inode
struct inode {
	uint dev; // Device number
	uint inum; // Inode number
	int ref; // Reference count
	struct sleeplock lock; // protects everything below here
	int valid; // inode has been read from disk?

	short type; // copy of disk inode
	short major;
	short minor;
	short nlink;
	uint size;
	uint mode; // maybe ushort?
	ushort gid;
	ushort uid;
	uint ctime; // change
	uint atime; // access
	uint mtime; // modification
	uint addrs[NDIRECT + 1];
};

// table mapping major device number to
// device functions
struct devsw {
	int (*read)(struct inode *, char *, int);
	int (*write)(struct inode *, char *, int);
};

extern struct devsw devsw[];
#define EOF (256)

enum { CONSOLE = 1, NULLDRV };
//#define CONSOLE 1
//#define NULLDRV 2
