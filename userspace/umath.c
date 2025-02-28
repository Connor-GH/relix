#include <umath.h>
#include <stddef.h>

unsigned int
ui_pow(unsigned int base, unsigned int power)
{
	if (power == 0) {
		return 1;
	}
	if (base == 0) {
		return 0;
	}

	unsigned int ret = 1;
	for (int i = 0; i < power; i++) {
		ret *= base;
	}
	return ret;
}

static size_t
floor_log2(size_t n)
{
	return (sizeof(size_t) * 8 - 1) - __builtin_clzl(n);
}

// so satisfying.
size_t
ceil_log2(size_t n)
{
	return !!n * floor_log2(n - 1) + !!n;
}

int
abs(int j)
{
	return j < 0 ? -j : j;
}
