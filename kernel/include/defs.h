#pragma once
#if __RELIX_KERNEL__
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#endif
