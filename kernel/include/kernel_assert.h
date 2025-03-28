#pragma once
#if __KERNEL__

__attribute__((noreturn)) void
kernel_assert_fail(const char *assertion, const char *file, int lineno,
									 const char *func);
// the no-op here makes clang happy.
static inline void
no_op(void){}
#define kernel_assert(expr) \
	(__builtin_expect(expr, 1) ? no_op() : kernel_assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif
