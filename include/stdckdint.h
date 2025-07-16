#pragma once
#define __STDC_VERSION_STDCKDINT_H__ 202311L

// Returns false on success, true on overflow.
#define ckd_add(res, a, b) __builtin_add_overflow(a, b, res)
#define ckd_sub(res, a, b) __builtin_sub_overflow(a, b, res)
#define ckd_mul(res, a, b) __builtin_mul_overflow(a, b, res)
