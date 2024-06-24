#pragma once
#define T_DIR 1 // Directory
#define T_FILE 2 // File
#define T_DEV 3 // Device

#ifndef USE_HOST_TOOLS
#include <types.h>
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
#define S_IFMT 0170000 /* These bits determine file type */
#define S_IFDIR 0040000 /* Directory */
#define S_IFCHR 0020000 /* Character device */
#define S_IFBLK 0060000 /* Block device */
#define S_IFREG 0100000 /* Regular file */
#define S_IFIFO 0010000 /* FIFO */
#define S_IFLNK 0120000 /* sym link */
#define S_IFSOCK 0140000 /* socket */
#define S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))

#define S_ISDIR(mode) S_ISTYPE((mode), S_IFDIR)
#define S_ISCHR(mode) S_ISTYPE((mode), S_IFCHR)
#define S_ISBLK(mode) S_ISTYPE((mode), S_IFBLK)
#define S_ISREG(mode) S_ISTYPE((mode), S_IFREG)
#define S_ISFIFO(mode) S_ISTYPE((mode), S_IFIFO)
#define S_ISLNK(mode) S_ISTYPE((mode), S_IFLNK)
#define S_ISANY(mode)                                                         \
	(_Bool)(S_ISDIR(mode) || S_ISCHR(mode) || S_ISBLK(mode) || S_ISREG(mode) || \
					S_ISFIFO(mode) || S_ISLNK(mode))

#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
struct stat {
	short type; // Type of file -- TODO get rid of this struct member
	int st_dev; /* u32 or u64 */ // File system's disk device
	uint st_ino; /* u32 or u64 */ // Inode number
	short st_nlink; /* u32 or u64 */ // Number of links to file
	uint st_size; /* u32 or u64 */ // Size of file in bytes
	uint st_mode; /* u32 */
	ushort st_uid; /* u32 */
	ushort st_gid; /* u32 */
	uint st_ctime; /* u32 or u64 */ // change
	uint st_atime; /* u32 or u64 */ // access
	uint st_mtime; /* u32 or u64 */ // modification
};
#endif
// 0700
#define S_IAUSR (S_IRUSR | S_IWUSR | S_IXUSR)
// 0070
#define S_IAGRP (S_IRGRP | S_IWGRP | S_IXGRP)
// 0007
#define S_IAOTH (S_IROTH | S_IWOTH | S_IXOTH)
#define S_HASPERM(mode, mask) (((mode) & (mask)) == (mask))
// 0777
#define S_ALLPRIVS (S_IAUSR | S_IAGRP | S_IAOTH)
#define DEFAULT_GID 0
#define DEFAULT_UID 0
