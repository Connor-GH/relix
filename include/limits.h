#pragma once

#define CHAR_BIT 8
#define BOOL_WIDTH 8
#define MB_LEN_MAX 16 // Maximum number of bytes in a multibyte character.

#define CHAR_WIDTH __CHAR_WIDTH__
#define CHAR_MIN __CHAR_MIN__
#define CHAR_MAX __CHAR_MAX__

#define SCHAR_WIDTH __SCHAR_WIDTH__
#define SCHAR_MIN __SCHAR_MIN__
#define SCHAR_MAX __SCHAR_MAX__

#define UCHAR_WIDTH __UCHAR_WIDTH__
#define UCHAR_MAX __UCHAR_MAX__

#define SHRT_WIDTH __SHRT_WIDTH__
#define SHRT_MIN __SHRT_MIN__
#define SHRT_MAX __SHRT_MAX__

#define USHRT_WIDTH __USHRT_WIDTH__
#define USHRT_MAX __USHRT_MAX__

#define INT_WIDTH __INT_WIDTH__
#define INT_MIN __INT_MIN__
#define INT_MAX __INT_MAX__

#define UINT_WIDTH __UINT_WIDTH__
#define UINT_MAX __UINT_MAX__

#define LONG_WIDTH __LONG_WIDTH__
#define LONG_MIN __LONG_MIN__
#define LONG_MAX __LONG_MAX__

#define ULONG_WIDTH __ULONG_WIDTH__
#define ULONG_MAX (~0UL)

#define LLONG_WIDTH __LONG_LONG_WIDTH__
#define LLONG_MIN __LONG_LONG_MIN__
#define LLONG_MAX __LONG_LONG_MAX__

#define ULLONG_WIDTH __LONG_LONG_WIDTH__
#define ULLONG_MAX (~0ULL)

// Minimum value according to POSIX is 256.
// https://pubs.opengroup.org/onlinepubs/009696699/basedefs/limits.h.html
#define _POSIX_PATH_MAX 256

#define _POSIX_PIPE_BUF 512
#define _POSIX_ARG_MAX 4096

#define _POSIX_HOST_NAME_MAX 255
#define _POSIX_LINK_MAX 8
#define _POSIX_LOGIN_NAME_MAX 9
#define _POSIX_MAX_INPUT 255
#define _XOPEN_IOV_MAX 16

#define PIPE_BUF _POSIX_PIPE_BUF
#define PATH_MAX _POSIX_PATH_MAX
// The kernel-side buffer only has room for 64 arguments total.
#define ARG_MAX 64

#define PAGESIZE 4096
#define PAGE_SIZE PAGESIZE

#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#define IOV_MAX _XOPEN_IOV_MAX

#define FILESIZEBITS 64
#define LINK_MAX _POSIX_LINK_MAX

#define MAX_INPUT _POSIX_MAX_INPUT
