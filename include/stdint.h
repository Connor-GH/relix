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

#define INT8_MAX __INT8_MAX__
#define INT8_MIN (-INT8_MAX - 1)

#define INT16_MAX __INT16_MAX__
#define INT16_MIN (-INT16_MAX - 1)

#define INT32_MAX __INT32_MAX__
#define INT32_MIN (-INT32_MAX - 1)

#define INT64_MAX __INT64_MAX__
#define INT64_MIN (-INT64_MAX - 1)

#define INT_FAST8_MAX __INT_FAST8_MAX__
#define INT_FAST8_MIN (-INT_FAST8_MAX - 1)

#define INT_FAST16_MAX __INT_FAST16_MAX__
#define INT_FAST16_MIN (-INT_FAST16_MAX - 1)

#define INT_FAST32_MAX __INT_FAST32_MAX__
#define INT_FAST32_MIN (-INT_FAST32_MAX - 1)

#define INT_FAST64_MAX __INT_FAST64_MAX__
#define INT_FAST64_MIN (-INT_FAST64_MAX - 1)

#define INT_LEAST8_MAX __INT_LEAST8_MAX__
#define INT_LEAST8_MIN (-INT_LEAST8_MAX - 1)

#define INT_LEAST16_MAX __INT_LEAST16_MAX__
#define INT_LEAST16_MIN (-INT_LEAST16_MAX - 1)

#define INT_LEAST32_MAX __INT_LEAST32_MAX__
#define INT_LEAST32_MIN (-INT_LEAST32_MAX - 1)

#define INT_LEAST64_MAX __INT_LEAST64_MAX__
#define INT_LEAST64_MIN (-INT_LEAST64_MAX - 1)

#define INTPTR_MAX __INTPTR_MAX__
#define INTPTR_MIN (-INTPTR_MAX - 1)

#define INTMAX_MAX __INTMAX_MAX__
#define INTMAX_MIN (-INTMAX_MAX - 1)

#define INT64_MAX __INT64_MAX__
#define INT64_MIN (-INT64_MAX - 1)

#define UINT8_MAX __UINT8_MAX__
#define UINT16_MAX __UINT16_MAX__
#define UINT32_MAX __UINT32_MAX__
#define UINT64_MAX __UINT64_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#define UINTMAX_MAX __UINTMAX_MAX__

#define UINT_LEAST8_MAX __UINT_LEAST8_MAX__
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX__
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX__
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX__

#define UINT_FAST8_MAX __UINT_FAST8_MAX__
#define UINT_FAST16_MAX __UINT_FAST16_MAX__
#define UINT_FAST32_MAX __UINT_FAST32_MAX__
#define UINT_FAST64_MAX __UINT_FAST64_MAX__

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
