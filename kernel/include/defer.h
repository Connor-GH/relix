#pragma once

#define DEFER_FUNC_IMPL(T, name, expr)           \
	static inline void __guard_func_##name(T *ptr) \
	{                                              \
		T guardval = *ptr;                           \
		expr                                         \
	}

#define defer_func(name) __attribute__((cleanup(__guard_func_##name)))
