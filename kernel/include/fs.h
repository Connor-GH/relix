#pragma once
#ifndef USE_HOST_STAT
#include <types.h>
#else
#include <../../include/types.h>
#endif
// On-disk file system format.
// Both the kernel and user programs use this header file.

#define ROOTINO 1 // root i-number
#define BSIZE 512 // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint size; // Size of file system image (blocks)
  uint nblocks; // Number of data blocks
  uint ninodes; // Number of inodes.
  uint nlog; // Number of log blocks
  uint logstart; // Block number of first log block
  uint inodestart; // Block number of first inode block
  uint bmapstart; // Block number of first free map block
};

#define NDIRECT 7
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// T_DEV -> IFBLK (block device)
// T_FILE -> IFREG (regular file)
// T_DIR -> IFDIR (directory)
// none? -> 0 (empty inode)
#define TYPE_TO_MODE(type) \
  type == T_DEV ? (S_IFBLK | S_IAUSR) \
  : (type == T_FILE ? (S_IFREG | S_IAUSR) \
  : type == T_DIR ? (S_IFDIR | S_IAUSR) : 0)

// On-disk inode structure
struct dinode {
  short type; // File type
  short major; // Major device number (T_DEV only)
  short minor; // Minor device number (T_DEV only)
  short nlink; // Number of links to inode in file system
  uint size; // Size of file (bytes)
  uint mode;
  ushort gid;
  ushort uid;
  uint ctime; // change
  uint atime; // access
  uint mtime; // modification
  uint addrs[NDIRECT + 1]; // Data block addresses
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b / BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

