module param;
@nogc nothrow:
extern(C): __gshared:
//#pragma once
enum NPROC = 64; // maximum number of processes
enum KSTACKSIZE = 4096; // size of per-process kernel stack
enum NCPU = 128; // maximum number of CPUs
enum NOFILE = 16; // open files per process
enum NFILE = 100; // open files per system
enum NLINK_DEREF = 31; // max amount of symlink dereferences
enum NINODE = 50; // maximum number of active i-nodes
enum NDEV = 10; // maximum major device number
enum ROOTDEV = 1; // device number of file system root disk
enum MAXARG = 32; // max exec arguments
enum MAXOPBLOCKS = 10; // max # of blocks any FS op writes
enum LOGSIZE = (MAXOPBLOCKS * 3); // max data blocks in on-disk log
enum NBUF = (MAXOPBLOCKS * 3); // size of disk block cache
enum FSSIZE = (10 * 1024); // size of file system in blocks
enum MAXGROUPS = 32; // maximum groups there can be
enum MAX_USERNAME = 256;
enum MAX_PASSWD = 128;
enum MAXENV = 32;
