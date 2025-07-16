#ifndef _FS_H
#define _FS_H
#pragma once
// On-disk file system format.
// Both the kernel and user programs use this header file.
#include <stdint.h>
#ifndef USE_HOST_TOOLS
#include "sleeplock.h"
#include <bits/__BSIZE.h>
#include <bits/__DIRSIZ.h>
#include <bits/__MAXFILE.h>
#include <sys/types.h>
#else
#include "include/bits/__BSIZE.h"
#include "include/bits/__DIRSIZ.h"
#include "include/bits/__MAXFILE.h"
#include "include/sys/types.h"
#include <kernel/include/sleeplock.h>
#endif
#if __KERNEL__ || defined(USE_HOST_TOOLS)
#define NDIRECT __NDIRECT
#define NINDIRECT __NINDIRECT
#define MAXFILE __MAXFILE
#define NDINDIRECT_PER_ENTRY __NDINDIRECT_PER_ENTRY
#define NDINDIRECT_ENTRY __NDIRECT
#define BSIZE __BSIZE
#define DIRSIZ __DIRSIZ
// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
	uintptr_t size; // Size of file system image (blocks)
	uintptr_t nblocks; // Number of data blocks
	uintptr_t ninodes; // Number of inodes.
	uintptr_t nlog; // Number of log blocks
	uintptr_t logstart; // Block number of first log block
	uintptr_t inodestart; // Block number of first inode block
	uintptr_t bmapstart; // Block number of first free map block
};
// in-memory copy of an inode
struct inode {
	dev_t dev; // Device number
	// Write and read files for FIFO. They share a `struct pipe *`.
	struct file *rf;
	struct file *wf;
	uint32_t inum; // Inode number
	int flags; // Flags when file is opened (e.g. O_RDONLY)
	int ref; // Reference count
	struct sleeplock lock; // protects everything below here
	int valid; // inode has been read from disk?

	uint64_t ctime; // change
	uint64_t atime; // access
	uint64_t mtime; // modification
	uint64_t size; // Size of file (bytes)
	mode_t mode; // File type and permissions
	uint16_t gid;
	uint16_t uid;
	uint64_t addrs[NDIRECT + 1 + 1]; // Data block addresses
	short major; // Major device number (T_DEV only)
	short minor; // Minor device number (T_DEV only)
	short nlink; // Number of links to inode in file system
	/* 2 bytes of padding */
};
// On-disk inode structure
struct dinode {
	uint64_t ctime; // change
	uint64_t atime; // access
	uint64_t mtime; // modification
	uint64_t size; // Size of file (bytes)
	mode_t mode; // File type and permissions
	uint16_t gid;
	uint16_t uid;
	uint64_t addrs[NDIRECT + 1 + 1]; // Data block addresses
	short major; // Major device number (T_DEV only)
	short minor; // Minor device number (T_DEV only)
	short nlink; // Number of links to inode in file system
	/* 2 bytes of padding */
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + (sb).inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8LU)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b) / BPB + (sb).bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define ROOTINO 1U // root i-number

#if !defined(USE_HOST_TOOLS) || __KERNEL__
#include <sys/stat.h>
void read_superblock(dev_t dev, struct superblock *sb);
int dirlink(struct inode *, const char *, uint32_t);
struct inode *dirlookup(struct inode *, const char *, uint64_t *);
struct inode *inode_alloc(dev_t, mode_t);
struct inode *inode_dup(struct inode *);
void inode_init(dev_t dev);
void inode_lock(struct inode *ip) __acquires(&ip->lock);
void inode_put(struct inode *);
void inode_unlock(struct inode *ip) __releases(&ip->lock);
void inode_unlockput(struct inode *ip) __releases(&ip->lock);
void inode_update(struct inode *);
int namecmp(const char *, const char *);
struct inode *namei(const char *);
struct inode *nameiparent(const char *, char *);
ssize_t inode_read(struct inode *ip, char *, off_t off, uint64_t n)
	__must_hold(&ip->lock);
void inode_stat(struct inode *ip, struct stat *) __must_hold(&ip->lock);
ssize_t inode_write(struct inode *ip, char *, off_t off, uint64_t n)
	__must_hold(&ip->lock);
#endif
#endif
#endif // !_FS_H
