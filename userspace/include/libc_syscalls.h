#pragma once
#include <sys/syscall.h>

extern int errno;

#define __syscall0(num)                             \
	({                                                \
		long long __ret;                                \
		__asm__ __volatile__("syscall"                  \
												 : "=a"(__ret)              \
												 : "a"(num)                 \
												 : "rcx", "r11", "memory"); \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                          \
	})

#define __syscall1(num, a1)                            \
	({                                                   \
		long long __ret;                                   \
		register __typeof__(a1) __rdi __asm__("rdi") = a1; \
		__asm__ __volatile__("syscall"                     \
												 : "=a"(__ret)                 \
												 : "a"(num), "r"(__rdi)        \
												 : "rcx", "r11", "memory");    \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                             \
	})
#define __syscall2(num, a1, a2)                        \
	({                                                   \
		long long __ret;                                   \
		register __typeof__(a1) __rdi __asm__("rdi") = a1; \
		register __typeof__(a2) __rsi __asm__("rsi") = a2; \
		__asm__ __volatile__("syscall"                     \
												 : "=a"(__ret)                 \
												 : "a"(num), "r"(__rdi)        \
												 : "rcx", "r11", "memory");    \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                             \
	})
#define __syscall3(num, a1, a2, a3)                                     \
	({                                                                    \
		long long __ret;                                                    \
		register __typeof__(a1) __rdi __asm__("rdi") = a1;                  \
		register __typeof__(a2) __rsi __asm__("rsi") = a2;                  \
		register __typeof__(a3) __rdx __asm__("rdx") = a3;                  \
		__asm__ __volatile__("syscall"                                      \
												 : "=a"(__ret)                                  \
												 : "a"(num), "r"(__rdi), "r"(__rsi), "r"(__rdx) \
												 : "rcx", "r11", "memory");                     \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                                              \
	})
#define __syscall4(num, a1, a2, a3, a4)                                  \
	({                                                                     \
		long long __ret;                                                     \
		register __typeof__(a1) __rdi __asm__("rdi") = a1;                   \
		register __typeof__(a2) __rsi __asm__("rsi") = a2;                   \
		register __typeof__(a3) __rdx __asm__("rdx") = a3;                   \
		register __typeof__(a4) __r10 __asm__("r10") = a4;                   \
		__asm__ __volatile__("syscall"                                       \
												 : "=a"(__ret)                                   \
												 : "a"(num), "r"(__rdi), "r"(__rsi), "r"(__rdx), \
													 "r"(__r10),                                   \
												 : "rcx", "r11", "memory");                      \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                                               \
	})
#define __syscall5(num, a1, a2, a3, a4, a5)                              \
	({                                                                     \
		long long __ret;                                                     \
		register __typeof__(a1) __rdi __asm__("rdi") = a1;                   \
		register __typeof__(a2) __rsi __asm__("rsi") = a2;                   \
		register __typeof__(a3) __rdx __asm__("rdx") = a3;                   \
		register __typeof__(a4) __r10 __asm__("r10") = a4;                   \
		register __typeof__(a5) __r8 __asm__("r8") = a5;                     \
		__asm__ __volatile__("syscall"                                       \
												 : "=a"(__ret)                                   \
												 : "a"(num), "r"(__rdi), "r"(__rsi), "r"(__rdx), \
													 "r"(__r10), "r"(__r8)                         \
												 : "rcx", "r11", "memory");                      \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                                               \
	})
#define __syscall6(num, a1, a2, a3, a4, a5, a6)                          \
	({                                                                     \
		long long __ret;                                                     \
		register __typeof__(a1) __rdi __asm__("rdi") = a1;                   \
		register __typeof__(a2) __rsi __asm__("rsi") = a2;                   \
		register __typeof__(a3) __rdx __asm__("rdx") = a3;                   \
		register __typeof__(a4) __r10 __asm__("r10") = a3;                   \
		register __typeof__(a5) __r8 __asm__("r8") = a5;                     \
		register __typeof__(a6) __r9 __asm__("r9") = a6;                     \
		__asm__ __volatile__("syscall"                                       \
												 : "=a"(__ret)                                   \
												 : "a"(num), "r"(__rdi), "r"(__rsi), "r"(__rdx), \
													 "r"(__r10), "r"(__r8), "r"(__r9)              \
												 : "rcx", "r11", "memory");                      \
		if (__ret < 0) {                                \
			errno = -__ret;                               \
			__ret = -1;                                   \
		}                                               \
		__ret;                                                               \
	})
