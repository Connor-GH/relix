#pragma once
#if __STDC_VERSION__ < 202311L
#define noreturn _Noreturn
#else
#define noreturn [[noreturn]]
#endif
