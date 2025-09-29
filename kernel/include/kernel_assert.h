#pragma once
#if __RELIX_KERNEL__

__attribute__((noreturn)) void kernel_assert_fail(const char *assertion,
                                                  const char *file, int lineno,
                                                  const char *func);
__attribute__((noreturn)) void
kernel_assert_fail_unlocked(const char *assertion, const char *file, int lineno,
                            const char *func);
#define kernel_assert(expr)    \
	(__builtin_expect(expr, 1) ? \
	   (void)0 :                 \
	   kernel_assert_fail(#expr, __FILE__, __LINE__, __func__))
#define kernel_assert_unlocked(expr) \
	(__builtin_expect(expr, 1) ?       \
	   (void)0 :                       \
	   kernel_assert_fail_unlocked(#expr, __FILE__, __LINE__, __func__))
#endif
