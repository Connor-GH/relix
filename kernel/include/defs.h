#pragma once
#if __KERNEL__
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#endif
