#pragma once
/* Exported to userspace */
#define O_RDONLY           0000
#define O_WRONLY           0001
#define O_RDWR             0002
#define O_ACCMODE          0003
#define O_CREATE           0100
#define O_CREAT O_CREATE
#define O_EXCL             0200
#define O_TRUNC           01000
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_DIRECTORY	    0200000
#define O_TMPFILE       (020000000 | O_DIRECTORY)

// These three pretty much wrap dup().
#define F_DUPFD 0x000
#define F_GETFD 0x001
#define F_SETFD 0x002 // expects arg

#define F_GETFL 0x004
#define F_SETFL 0x008 // expects arg

#define FD_CLOEXEC 0x01

#define F_OK 0
#define X_OK 1
#define W_OK (1 << 1)
#define R_OK (1 << 2)
