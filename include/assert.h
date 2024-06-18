#pragma once
void
assert_fail(const char *assertion, const char *file, int lineno,
						const char *func);
#define assert(expression) \
	expression ? 0 : assert_fail(#expression, __FILE__, __LINE__, __func__)
