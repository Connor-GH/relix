/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "memlayout.h"
#include "lib/compiler_attributes.h"

__always_inline uintptr_t
v2p(void *a)
{
	return ((uintptr_t)(a)) - ((uintptr_t)KERNBASE);
}

__always_inline void *
p2v(uintptr_t a)
{
	return (void *)((a) + ((uintptr_t)KERNBASE));
}

__always_inline uintptr_t
v2io(void *a)
{
	return (((uintptr_t)(a)) - (DEVBASE - DEVSPACE));
}

__always_inline void *
io2v(uintptr_t a)
{
	return (((void *)(a)) + (DEVBASE - DEVSPACE));
}
