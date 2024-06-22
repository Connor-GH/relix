#pragma once
void
assert_fail(const char *assertion, const char *file, int lineno,
						const char *func);
static inline void
no_op(void)
{
}
#define assert(expression) \
	expression ? no_op() : assert_fail(#expression, __FILE__, __LINE__, __func__)
