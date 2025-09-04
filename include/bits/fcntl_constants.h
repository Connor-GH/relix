#pragma once
/* Exported to userspace */
#define O_RDONLY 0x0
#define O_WRONLY 0x1
#define O_RDWR 0x2
#define O_ACCMODE 0x3
#define O_CREATE 0x4
#define O_CREAT O_CREATE
#define O_EXCL 0x8
#define O_TRUNC 0x10
#define O_APPEND 0x20
#define O_NONBLOCK 0x40
#define O_NOFOLLOW 0x80
#define O_DIRECTORY 0x100
#define O_TMPFILE (0x200 | O_DIRECTORY)
#define O_CLOEXEC 0x400
#define O_CLOFORK 0x800
#define O_NOCTTY 0x1000
#define O_DSYNC 0x2000
#define O_RSYNC 0x4000
#define O_SYNC 0x8000
#define O_EXEC 0x10000
#define O_SEARCH 0x20000

#define O_TTY_INIT 0

// These three pretty much wrap dup().
#define F_DUPFD 0x000
#define F_GETFD 0x001
#define F_SETFD 0x002 // expects arg

#define F_GETFL 0x004
#define F_SETFL 0x008 // expects arg

#define F_DUPFD_CLOEXEC 0x010
#define F_GETLK 0x020
#define F_SETLK 0x040 // expects arg
#define F_GETLKW 0x080
#define F_SETLKW 0x100 // expects arg

#define F_GETOWN 0x200
#define F_SETOWN 0x400

#define FD_CLOEXEC 0x01
#define FD_CLOFORK 0x02

#define AT_FDCWD -42

#define AT_SYMLINK_NOFOLLOW (1 << 0)
#define AT_SYMLINK_FOLLOW (1 << 0)
#define AT_REMOVEDIR (1 << 0)
#define AT_EACCESS (1 << 0)
