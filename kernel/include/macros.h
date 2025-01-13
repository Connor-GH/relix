#pragma once
#define min(a, b) ((a) < (b) ? (a) : (b))
/* clang-format off */
#define is_same_type(type1, type2) (1 == _Generic((type2)0, type1: 1, default: 0))
/* clang-format on */
