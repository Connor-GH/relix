#pragma once
#if __KERNEL__
#define min(a, b) ((a) < (b) ? (a) : (b))
#define swap(a, b)              \
	do {                          \
		__typeof__(a) __temp_a = a; \
		a = b;                      \
		b = __temp_a;               \
	} while (0)

/* clang-format off */
#define is_same_type(type1, type2) (1 == _Generic((type2)0, type1: 1, default: 0))
/* clang-format on */
#define UNREACHABLE() kernel_assert(0 && "UNREACHABLE")
#define saturating_sub(x, y, sat) \
	({                              \
		__typeof__(x) ret;            \
		if (x < sat + y)              \
			ret = sat;                  \
		else                          \
			ret = x - y;                \
		ret;                          \
	})
#define signed_saturating_add(x, y, sat) \
	({                                     \
		__typeof__(x) ret;                   \
		if (y < 0 && x < -y)                 \
			ret = 0;                           \
		else if (x + y > sat)                \
			ret = sat;                         \
		else                                 \
			ret = x + y;                       \
		ret;                                 \
	})
#define _S(x) #x
#define STRINGIFY(x) _S(x)

#define BOOL_STRING(cond) (cond ? "true" : "false")

#define ROUND_UP(x, round_to) (((x + (round_to - 1)) / round_to) * round_to)
#define ROUND_DOWN(x, round_to) (x / round_to * round_to)
#endif
