#pragma once
#include <bits/stdint.h>

#define SIZE_MAX __SIZE_MAX__
#define SIZE_MIN __SIZE_MIN__
#define SIZE_WIDTH __SIZE_WIDTH__

#define PTRDIFF_WIDTH __PTRDIFF_WIDTH__
#define PTRDIFF_MIN __PTRDIFF_MIN__
#define PTRDIFF_MAX __PTRDIFF_MAX__

#define SIG_ATOMIC_WIDTH __SIG_ATOMIC_WIDTH__
#define SIG_ATOMIC_MIN __SIG_ATOMIC_MIN__
#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__

#define WINT_WIDTH __WINT_WIDTH__
#define WINT_MIN __WINT_MIN__
#define WINT_MAX __WINT_MAX__

#define WCHAR_WIDTH __WCHAR_WIDTH__
#define WCHAR_MIN __WCHAR_MIN__
#define WCHAR_MAX __WCHAR_MAX__

#ifndef __ASSEMBLER__
typedef __int8_t int8_t;
typedef __int_least8_t int_least8_t;
typedef __int_fast8_t int_fast8_t;

typedef __uint8_t uint8_t;
typedef __uint_least8_t uint_least8_t;
typedef __uint_fast8_t uint_fast8_t;
typedef __int_least16_t int_least16_t;

typedef __int16_t int16_t;
typedef __int_least16_t int_least16_t;
typedef __int_fast16_t int_fast16_t;

typedef __uint16_t uint16_t;
typedef __uint_least16_t uint_least16_t;
typedef __uint_fast16_t uint_fast16_t;

typedef __int32_t int32_t;
typedef __int_least32_t int_least32_t;
typedef __int_fast32_t int_fast32_t;

typedef __uint32_t uint32_t;
typedef __uint_least32_t uint_least32_t;
typedef __uint_fast32_t uint_fast32_t;

typedef __int64_t int64_t;
typedef __int_least64_t int_least64_t;
typedef __int_fast64_t int_fast64_t;
typedef __uint64_t uint64_t;
typedef __uint_least64_t uint_least64_t;
typedef __uint_fast64_t uint_fast64_t;

typedef __intptr_t intptr_t;
typedef __uintptr_t uintptr_t;

typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;

#define INT8_C(x) __INT8_C(x)
#define INT16_C(x) __INT16_C(x)
#define INT32_C(x) __INT32_C(x)
#define INT64_C(x) __INT64_C(x)
#define UINT8_C(x) __UINT8_C(x)
#define UINT16_C(x) __UINT16_C(x)
#define UINT32_C(x) __UINT32_C(x)
#define UINT64_C(x) __UINT64_C(x)
#define INTMAX_C(x) __INTMAX_C(x)
#define UINTMAX_C(x) __UINTMAX_C(x)

#endif /* !__ASSEMBLER__ */
