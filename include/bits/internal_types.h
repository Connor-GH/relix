#pragma once
#if defined(__clang__)
#define CLANG_COMPILER
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GNU_COMPILER
#endif
