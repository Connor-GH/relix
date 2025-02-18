#pragma once
#define SIZE_MAX ULONG_MAX

// Minimum value according to POSIX is 256.
// https://pubs.opengroup.org/onlinepubs/009696699/basedefs/limits.h.html
#define PATH_MAX 256
#define CHAR_BIT 8

#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255 // (2^8) - 1

#define CHAR_MIN SCHAR_MIN  // Typically the same as SCHAR_MIN, implementation-defined.
#define CHAR_MAX SCHAR_MAX  // Typically the same as SCHAR_MAX, implementation-defined.
#define MB_LEN_MAX  1 // Maximum number of bytes in a multibyte character.  This is not related to the integer ranges.

// Short Integer Types
#define SHRT_MIN (-32768) // -(2^15)
#define SHRT_MAX 32767 // (2^15) - 1
#define USHRT_MAX 65535U // (2^16) - 1

// Integer Types
#define INT_MIN (-INT_MAX - 1) // -(2^31)
#define INT_MAX 2147483647  // (2^31) - 1
#define UINT_MAX 4294967295U // (2^32) - 1

// Long Integer Types (64-bit)
#define LONG_MIN (-9223372036854775808L) // -(2^63)
#define LONG_MAX 9223372036854775807L  // (2^63) - 1
#define ULONG_MAX 18446744073709551615UL // (2^64) - 1
