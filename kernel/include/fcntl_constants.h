#pragma once
#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002
#define O_EXCL 0x004
#define O_TRUNC 0x008
#define O_CREATE 0x200
#define O_CREAT O_CREATE
#define O_APPEND 0x400
#define O_NONBLOCK 0x800

// These three pretty much wrap dup().
#define F_DUPFD 0x000
#define F_GETFD 0x001
#define F_SETFD 0x002 // expects arg

#define F_GETFL 0x004
#define F_SETFL 0x008 // expects arg

#define FD_CLOEXEC 0x01
