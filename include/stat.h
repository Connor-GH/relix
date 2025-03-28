#pragma once

#ifndef USE_HOST_TOOLS
#include <stdint.h>
#include <sys/types.h>
/* fields do not begin with "st_" if they are nonstandard. */
//
// +--------->+-------+
// |+-------->| owner |
// ||+------->+-------+
// |||+------>+-------+
// ||||+----->| group |
// |||||+---->+-------+
// ||||||+--->+--------+
// |||||||+-->| public |
// ||||||||+->+--------+
// VVVVVVVVV
//trwxrwxrwx
//^^^^
//||||
//|||+->exec
//||+->write
//|+->read
//+->type
//
// in other words,
// rwx rwx rwx
// uid gid others
#define S_IFIFO  0010000 /* FIFO */
#define S_IFCHR  0020000 /* Character device */
#define S_IFDIR  0040000 /* Directory */
#define S_IFBLK  0060000 /* Block device */
#define S_IFREG  0100000 /* Regular file */
#define S_IFLNK  0120000 /* sym link */
#define S_IFSOCK 0140000 /* socket */
#define S_IFMT   0170000 /* These bits determine file type */
#define S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))

#define S_ISDIR(mode) S_ISTYPE((mode), S_IFDIR)
#define S_ISCHR(mode) S_ISTYPE((mode), S_IFCHR)
#define S_ISBLK(mode) S_ISTYPE((mode), S_IFBLK)
#define S_ISREG(mode) S_ISTYPE((mode), S_IFREG)
#define S_ISFIFO(mode) S_ISTYPE((mode), S_IFIFO)
#define S_ISLNK(mode) S_ISTYPE((mode), S_IFLNK)
#define S_ISSOCK(mode) S_ISTYPE((mode), S_IFSOCK)
#define S_ISANY(mode)                                                         \
	(_Bool)(S_ISDIR(mode) || S_ISCHR(mode) || S_ISBLK(mode) || S_ISREG(mode) || \
					S_ISFIFO(mode) || S_ISLNK(mode) || S_ISSOCK(mode))

#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

struct stat {
	dev_t st_dev;  // File system's disk device
	ino_t st_ino; // Inode number
	short st_nlink; /* u32 or u64 */ // Number of links to file
	off_t st_size; // Size of file in bytes
	mode_t st_mode;
	uid_t st_uid;
	gid_t st_gid;
	struct timespec st_ctim; // change
	struct timespec st_atim; // access
	struct timespec st_mtim; // modification
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};
#endif
// 0700
#define S_IAUSR S_IRWXU
// 0070
#define S_IAGRP S_IRWXG
// 0007
#define S_IAOTH S_IRWXO
#define S_HASPERM(mode, mask) (((mode) & (mask)) == (mask))
// 0777
#define S_ALLPRIVS (S_IAUSR | S_IAGRP | S_IAOTH)
#define DEFAULT_GID 0
#define DEFAULT_UID 0
