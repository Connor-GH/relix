#pragma once
#if __KERNEL__

__attribute__((noreturn)) void kernel_assert_fail(const char *assertion,
                                                  const char *file, int lineno,
                                                  const char *func);
#define kernel_assert(expr)    \
	(__builtin_expect(expr, 1) ? \
	   (void)0 :                 \
	   kernel_assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif
