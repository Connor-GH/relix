#pragma once

#ifndef __ASSEMBLER__
extern int errno;
#endif
#define EPERM 1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3 /* No such process */
#define EINTR 4 /* Interrupted system call */
#define EIO 5 /* I/O error */
#define ENXIO 6 /* No such device or address */
#define E2BIG 7 /* Argument list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9 /* Bad file number */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Try again */
#define EWOULDBLOCK EAGAIN
#define ENOMEM 12 /* Out of memory */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16 /* Device or resource busy */
#define EEXIST 17 /* File exists */
#define EXDEV 18 /* Cross-device link */
#define ENODEV 19 /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21 /* Is a directory */
#define EINVAL 22 /* Invalid argument */
#define ENFILE 23 /* File table overflow */
#define EMFILE 24 /* Too many open files */
#define ENOTTY 25 /* Not a typewriter */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27 /* File too large */
#define ENOSPC 28 /* No space left on device */
#define ESPIPE 29 /* Illegal seek */
#define EROFS 30 /* Read-only file system */
#define EMLINK 31 /* Too many links */
#define EPIPE 32 /* Broken pipe */
#define EDOM 33 /* Math argument out of domain of func */
#define ERANGE 34 /* Math result not representable */
#define EDEADLK 35 /* Resource deadlock */
#define ENAMETOOLONG 36 /* Name too long */
#define ENOLCK 37 /* No record locks available */
#define ENOSYS 38 /* Functionality not supported */
#define ENOTEMPTY 39 /* Directory not empty */
#define ELOOP 40 /* Too many symbolic links */
#define EADDRINUSE 41 /* Address in use */
#define EADDRNOTAVAIL 42 /* Address not available */
#define EAFNOSUPPORT 43 /* Address family not supported */
#define EALREADY 44 /* Connection already in progress */
#define EBADMSG 45 /* Bad message */
#define ECANCELED 46 /* Operation canceled */
#define ECONNABORTED 47 /* Connection aborted */
#define ECONNREFUSED 48 /* Connection refused */
#define ECONNRESET 49 /* Connection reset */
#define EDESTADDRREQ 50 /* Destination address required */
#define EDQUOT 51 /* Reserved */
#define EHOSTUNREACH 52 /* Host is unreachable */
#define EIDRM 53 /* Identifier removed */
#define EILSEQ 54 /* Illegal byte sequence */
#define EINPROGRESS 55 /* Operation in progress */
#define EISCONN 56 /* Socket is connected */
#define EMSGSIZE 57 /* Message too large */
#define EMULTIHOP 58 /* Reserved */
#define ENETDOWN 59 /* Network is down */
#define ENETRESET 60 /* Connection aborted by network */
#define ENETUNREACH 61 /* Network unreachable */
#define ENOBUFS 62 /* No buffer space available */
#define ENOLINK 63 /* Reserved */
#define ENOMSG 64 /* No message of the desired type */
#define ENOPROTOOPT 65 /* Protocol not available */
#define ENOTCONN 66 /* The socket is not connected */
#define ENOTRECOVERABLE 67 /* State not recoverable */
#define ENOTSOCK 68 /* Not a socket */
#define ENOTSUP 69 /* Not supported (may be the same value as [EOPNOTSUPP]) */
/*
 * Operation not supported on socket (may be the same value as [ENOTSUP])
 */
#define EOPNOTSUPP ENOTSUP
#define EOVERFLOW 70 /* Value too large to be stored in data type */
#define EOWNERDEAD 71 /* Previous owner died */
#define EPROTO 72 /* Protocol error */
#define EPROTONOSUPPORT 73 /* Protocol not supported */
#define EPROTOTYPE 74 /* Protocol wrong type for socket */
#define ESOCKTNOSUPPORT 75 /* Socket type not supported */
#define ESTALE 76 /* Reserved */
#define ETIMEDOUT 77 /* Connection timed out */
