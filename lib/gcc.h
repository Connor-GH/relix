#pragma once

#if __GNUC_MINOR__ >= 15
#define __counted_by(x) __attribute__((__counted_by__(x)))
#else
#define __counted_by(x)
#endif

// keep the variable no matter what.
#define __retain __attribute__((__retain__))
