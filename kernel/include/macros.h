#pragma once
#define min(a, b) ((a) < (b) ? (a) : (b))
/* clang-format off */
#define is_same_type(type1, type2) (1 == _Generic((type2)0, type1: 1, default: 0))
/* clang-format on */
#define UNREACHABLE() kernel_assert(!"UNREACHABLE")
#define saturating_sub(x, y, sat) \
	({                              \
		__typeof__(x) ret;            \
		if (x < sat + y)                  \
			ret = sat;                  \
		else                          \
			ret = x - y;                \
		ret;                          \
	})
#define signed_saturating_add(x, y, sat) \
	({                              \
		__typeof__(x) ret;            \
		if (y < 0 && x < -y)          \
		  ret = 0;                    \
		else if (x + y > sat)         \
			ret = sat;                  \
		else                          \
			ret = x + y;                \
		ret;                          \
	})
#define _S(x) #x
#define STRINGIFY(x) _S(x)
