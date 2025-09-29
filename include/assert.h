#pragma once

void __assert_fail(const char *assertion, const char *file, int lineno,
                   const char *func);

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#define assert(expression)           \
	(__builtin_expect(expression, 1) ? \
	   (void)0 :                       \
	   __assert_fail(#expression, __FILE__, __LINE__, __func__))
#endif
#if __STDC_VERSION__ < 202311L
#define static_assert _Static_assert
#endif
