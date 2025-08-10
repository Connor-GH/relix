#pragma once
/* Exported to userspace */
#define NPROC 64 // maximum number of processes
#define KSTACKSIZE 4096 // size of per-process kernel stack
#define NCPU 128 // maximum number of CPUs
#define NFILE 100 // open files per system
#define NLINK_DEREF 31 // max amount of symlink dereferences
#define NINODE 50 // maximum number of active i-nodes
#define NDEV 10 // maximum major device number
#define ROOTDEV 1 // device number of file system root disk
#define MAXARG 32 // max exec arguments
#define MAXOPBLOCKS 10U // max # of blocks any FS op writes
#define LOGSIZE (MAXOPBLOCKS * 3LU) // max data blocks in on-disk log
#define NBUF (MAXOPBLOCKS * 3LU) // size of disk block cache
#define FSSIZE (10 * 2048LU) // size of file system in blocks
#define MAX_USERNAME 256
#define MAX_PASSWD 128
#define MAXENV 32
#define MAX_PCI_DEVICES 32
#define NMMAP 10 // maximum number of mmap()'s allowed per process
#define NTTY 128 // maximum number of TTYs.
