#ifndef __STDARG_H
#define __STDARG_H

#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_list __builtin_va_list
#define va_arg(x, y) __builtin_va_arg(x, y)
#define va_copy __builtin_va_copy

#endif /* __STDARG_H */
