#pragma once
#include "../kernel/include/param.h"

// These are libc extensions.
// glibc and FreeBSD's libc both define these.

#define MAX(a, b)            \
	({                         \
		__typeof__(a) __a = (a); \
		__typeof__(b) __b = (b); \
		__a > __b ? __a : __b;   \
	})

#define MIN(a, b)            \
	({                         \
		__typeof__(a) __a = (a); \
		__typeof__(b) __b = (b); \
		__a < __b ? __a : __b;   \
	})
