#pragma once
#include "kernel/include/syscall.h"
#undef __KERNEL__

#define __GET_MACRO(_0, _1, _2, _3, _4, _5, _6, x, ...) x
#define syscall(...)                                                       \
	__GET_MACRO(__VA_ARGS__, __syscall6, __syscall5, __syscall4, __syscall3, \
							__syscall2, __syscall1, __syscall0)                          \
	(__VA_ARGS__)
