#pragma once

#define MB_LEN_MAX 16 // Maximum number of bytes in a multibyte character.

#define CHAR_BIT 8
#define CHAR_MIN __CHAR_MIN__
#define CHAR_MAX __CHAR_MAX__

#define SCHAR_MIN -128
#define SCHAR_MAX 127

#define UCHAR_MAX 255

#define SHRT_MIN __SHRT_MIN__
#define SHRT_MAX __SHRT_MAX__

#define USHRT_MAX __USHRT_MAX__

#define WORD_BIT __INT_WIDTH__
#define INT_MIN -2147483647
#define INT_MAX __INT_MAX__

#define UINT_MAX __UINT_MAX__

#define LONG_BIT __LONG_WIDTH__
#define LONG_MIN __LONG_MIN__
#define LONG_MAX __LONG_MAX__

#define ULONG_MAX (~0UL)

#define LLONG_MIN __LONG_LONG_MIN__
#define LLONG_MAX __LONG_LONG_MAX__

#define ULLONG_MAX (~0ULL)

// Minimum value according to POSIX is 256.
// https://pubs.opengroup.org/onlinepubs/009696699/basedefs/limits.h.html
#define _POSIX_PATH_MAX 256

#define _POSIX_PIPE_BUF 512
#define _POSIX_ARG_MAX 4096

#define _POSIX_HOST_NAME_MAX 255
#define _POSIX_LINK_MAX 8
#define _POSIX_SYMLINK_MAX 255
#define _POSIX_SYMLOOP_MAX 8
#define _POSIX_LOGIN_NAME_MAX 9
#define _POSIX_MAX_INPUT 255
#define _POSIX_OPEN_MAX 20
#define _POSIX_NGROUPS_MAX 8
#define _POSIX_NAME_MAX 14
#define _POSIX_TTY_NAME_MAX 9
#define _XOPEN_IOV_MAX 16
#define _XOPEN_NAME_MAX 255

#define PIPE_BUF _POSIX_PIPE_BUF
#define PATH_MAX _POSIX_PATH_MAX
// The kernel-side buffer only has room for 64 arguments total.
#define ARG_MAX 64

#define PAGESIZE 4096
#define PAGE_SIZE PAGESIZE

#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#define IOV_MAX _XOPEN_IOV_MAX
#define OPEN_MAX 100
#define NGROUPS_MAX _POSIX_NGROUPS_MAX

#define FILESIZEBITS 64
#define LINK_MAX _POSIX_LINK_MAX
// Maximum number of bytes in a symbolic link.
#define SYMLINK_MAX _POSIX_SYMLINK_MAX
#define SYMLOOP_MAX _POSIX_SYMLOOP_MAX
#define LOGIN_NAME_MAX 256
#define NAME_MAX 254
#define TTY_NAME_MAX NAME_MAX

#define MAX_INPUT _POSIX_MAX_INPUT
